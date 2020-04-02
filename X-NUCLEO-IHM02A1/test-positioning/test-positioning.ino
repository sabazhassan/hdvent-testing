/********************************************//**
 *  Basic script to run a single stepper motor with the
 *  X-NUCLEO-IHM02A1 driver board stacked on an Arduino Uno.
 *  Make sure to hardwire D3 to D13 pin and jumper SB41
 *  instead of SB40.
 *
 *  This script was adapted from the SparkFunAutoDriver lib's
 *  gantry.ino example script.
 *
 *  David Grimshandl, 2020/04/01
 *
 *  Requires:
 *  SparkFunAutoDriver
 *
 ***********************************************/

#include <Arduino.h>
#include <SparkFunAutoDriver.h>
#include <SPI.h>

// Need this line for some reason?!
AutoDriver Stepperdummy(1, A2, 4);  // Nr, CS, Reset => 0 , D16/A2 (PA4), D4 (PB5) for IHM02A1 board
AutoDriver Stepper(0, A2, 4);  // Nr, CS, Reset => 0 , D16/A2 (PA4), D4 (PB5) for IHM02A1 board
AutoDriver *boardIndex ;

unsigned int speed = 200;
byte spi_test();
void ConfigureBoards();
int getBoardConfigurations();
int getBoardStatus();
void SingleMotorCycle();
void UpdateMotorCurveParameters(float respiratoryRate, float pathRatio, float IERatio);
void PrintMotorCurveParameters();
bool isBusy();

// motor curve for volume control operation
// inspiratory phase
float const ACC_IN = 200; // steps/s/s
float speedIn = 100; // steps/s
float const DEC_IN = 200; // steps/s/s
float timeIn=1;

// hold time after inspiratory phase
float const TIME_HOLD_PLATEAU = 0.200; // seconds
int const stepDivider = STEP_FS_4;


// exspiratory phase
float const ACC_EX = 200; // steps/s/s
float const SPEED_EX = 100; //steps/s
float const DEC_EX = 200; // steps/s/s
float timeEx=1;

//float time_hold_ex = 4; // seconds

//
unsigned int stepsFullRange = 100*4;
unsigned int stepsInterval = 100*4;

// user-set parameters (later by poti)
float respiratoryRate = 15;
float pathRatio = 0.7;
float IERatio = 0.5;

void setup()
{
    Serial.begin(115200);

// Start by setting up the SPI port and pins. The Autodriver library does not do this for you!
    pinMode(3, INPUT);     // D3 = SPI SCK, wired-routed to D13(SCK), (D3 setup as input - tri-state)
    pinMode(4, OUTPUT);    // D4 = nReset
    pinMode(MOSI, OUTPUT); // Serial IN
    pinMode(MISO, INPUT);  // Serial OUT
    pinMode(13, OUTPUT);   // serial clock -> routed to D3 by wire
    pinMode(A2, OUTPUT);   // CS signal
    digitalWrite(A2, HIGH);   // nCS set High
    digitalWrite(4, LOW);     // toggle nReset
    //delay(100);
    digitalWrite(4, HIGH);
    SPI.begin();             // start
    boardIndex = &Stepper;

    // Configure the boards to the settings we wish to use
    //  for them. See below for more information about the
    //  settings applied.
    spi_test();
    ConfigureBoards();

}


void loop(){
    UpdateMotorCurveParameters(respiratoryRate, pathRatio, IERatio);
    //Stepper.move(0,1);
    //delay(5000);
    PrintMotorCurveParameters();
    SingleMotorCycle();
    getBoardStatus();
}


//! take user-set parameters and calculate the parameters for the motor curve,
//!
//! \param respiratoryRate - respiratory rate in breaths per minute
//! \param pathRatio - float between 0-1, setting the portion of the total path that will be driven
//! \param IERatio - (inhalation time)/(exhalation time)
void UpdateMotorCurveParameters(float respiratoryRate, float pathRatio, float IERatio) {
    float t_total = 60 / respiratoryRate;
    timeEx = t_total / (IERatio + 1);
    timeIn = t_total - timeEx ;
    int steps = stepsFullRange * pathRatio;
    //Serial.print("timeIn: "); Serial.println(timeIn);
    //Serial.print("timeEx: "); Serial.println(timeEx);

    float discriminant = sq(timeIn) - 2 * (1. / ACC_IN + 1. / DEC_IN) * steps;
    //Serial.print("discriminant: "); Serial.println(discriminant);
    if (discriminant > 0) {
        speedIn = (-timeIn + sqrtf(discriminant)) / -(1. / ACC_IN + 1. / DEC_IN);
        //time_hold_ex =  timeEx - ((1. / ACC_EX + 1. / DEC_EX)*sq(SPEED_EX)/2 + steps)/SPEED_EX;
        stepsInterval = steps;
    }
}

void PrintMotorCurveParameters(){
    Serial.print("speedIn: "); Serial.println(speedIn);
    //Serial.print("time_hold_ex "); Serial.println(time_hold_ex);
    Serial.print("stepsInterval: "); Serial.println(stepsInterval);
}

void SingleMotorCycle(){
    boardIndex->setMaxSpeed(speedIn);
    boardIndex->setAcc(ACC_IN);
    boardIndex->setDec(DEC_IN);


    Stepper.move(0,stepsInterval);

    // ensure inspiration phase lasts timeIn seconds
    unsigned long startTime = millis();
    unsigned long elapsedTime = millis()-startTime;
    while( isBusy() || elapsedTime < (timeIn-TIME_HOLD_PLATEAU)*1000){
        elapsedTime = millis()-startTime;
    };
    delay(1000*TIME_HOLD_PLATEAU);

    boardIndex->setMaxSpeed(SPEED_EX);
    boardIndex->setAcc(ACC_EX);
    boardIndex->setDec(DEC_EX);
    Stepper.move(1, stepsInterval);

    // ensure exspiration phase lasts timeEx seconds
    startTime = millis();
    elapsedTime = millis()-startTime;
    while( isBusy() || elapsedTime < timeEx*1000){
        elapsedTime = millis()-startTime;
    };

}

bool isBusy()
{
    byte status=0;
    bool busy=0;
    status = boardIndex->getStatus();
    busy = !((status>>1)&0x01);
    return(busy);
}

// Test Serial chain: shift out ox55 pattern and read back : should be 2 for  X-NUCLEO-IHM02A1 BOARD
// Important: L6470 Serial SPI only works if Vcc (Chip voltage) AND Vss (motor Voltage) is applied!
byte spi_test()
{
    byte val,t,u=0;
    digitalWrite(A2, LOW);
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3));
    Serial.print("\n SPI test: ");
    for(t=0x55;t<0x6D;++t)
    {
        val=SPI.transfer(t);
        Serial.print(" Tx:"); Serial.print(t,HEX);Serial.print("/Rx:"); Serial.print(val,HEX);
        if (val==0x55) {u=t-0x55;t=0x6D;}
        delay(100);
    }
    Serial.print("\n Chain="); Serial.println(u);Serial.print("\n");
    return(u);
}



//  For ease of reading, we're just going to configure all the boards to the same settings.
void ConfigureBoards()
{
    int paramValue;
    Serial.println("Configuring boards...");

    // Before we do anything, we need to tell each device which SPI port we're using.
    boardIndex->SPIPortConnect(&SPI);

    // reset device //
    boardIndex->resetDev();

    // Set the Overcurrent Threshold to 6A(max). The OC detect circuit is quite sensitive; even if the current is only momentarily
    //  exceeded during acceleration or deceleration, the driver will shutdown. This is a per channel value; it's useful to
    //  consider worst case, which is startup.
    boardIndex->setOCThreshold(OCD_TH_6000mA);

    // KVAL is a modifier that sets the effective voltage applied to the motor. KVAL/255 * Vsupply = effective motor voltage.
    //  This lets us hammer the motor harder during some phases  than others, and to use a higher voltage to achieve better
    //  torqure performance even if a motor isn't rated for such a high current.
    // This IHM02A1 BOARD has 12V motors and a 12V supply.
    boardIndex->setRunKVAL(240);  // 220/255 * 12V = 6V
    boardIndex->setAccKVAL(240);  // 220/255 * 12V = 6V
    boardIndex->setDecKVAL(240);  // /255 * 12V = 3V
    boardIndex->setHoldKVAL(180);  // 132/255 * 12V = 1.5V  // low voltage, almost free turn

    // The dSPIN chip supports microstepping for a smoother ride. This function provides an easy front end for changing the microstepping mode.
    // once in full speed, it will step up to half-step
    boardIndex->configStepMode(stepDivider); // Full step

    // When a move command is issued, max speed is the speed the Motor tops out at while completing the move, in steps/s
    boardIndex->setMaxSpeed(100);
    boardIndex->setMinSpeed(0);

    // Acceleration and deceleration in steps/s/s. Increasing this value makes the motor reach its full speed more quickly, at the cost of possibly causing it to slip and miss steps.
    boardIndex->setAcc(100);
    boardIndex->setDec(100);




}

int getBoardStatus()
{
    int paramValue;
    Serial.print("\n Status ");

    // See Datasheet page 55 : Status register
    // MOT_STAT: 00 = stopped / 01=Acc  / 10=Dec / 11 = const speed
    // OCD = Over current Flag (Active low)
    paramValue = boardIndex->getStatus();
    Serial.print("[hex]"); Serial.println(paramValue, HEX);
    Serial.println("0-HIZ | 1-BUSY | 2-SW_F | 3-SW_EVN | 4-DIR |  5-MOT_STAT  | NOPR_CMD");
    Serial.print("   ");Serial.print(paramValue&0x01);
    Serial.print("  |  "); Serial.print((paramValue>>1)&0x01);
    Serial.print("     |  "); Serial.print((paramValue>>2)&0x01);
    Serial.print("     |  "); Serial.print((paramValue>>3)&0x01);
    Serial.print("       |  "); Serial.print((paramValue>>4)&0x01);
    Serial.print("    |    "); Serial.print((paramValue>>5)&0x03);
    Serial.print("         |  "); Serial.println((paramValue>>7)&0x01);
    Serial.println("-------------------------------------------------------------------");
    Serial.println("8-WRNG_CMD | 9-UVLO | 10-TH_WRN | 11-TH_SD | 12-OCD | 13-STLOS_A | 14-STLOS_B | 15-SCK_MOD");
    Serial.print("   "); Serial.print((paramValue>>8)&0x01);
    Serial.print("       |  "); Serial.print((paramValue>>9)&0x01);
    Serial.print("     |  "); Serial.print((paramValue>>10)&0x01);
    Serial.print("        |  "); Serial.print((paramValue>>11)&0x01);
    Serial.print("       |  "); Serial.print((paramValue>>12)&0x01);
    Serial.print("     |  "); Serial.print((paramValue>>13)&0x01);
    Serial.print("         |  "); Serial.print((paramValue>>14)&0x01);
    Serial.print("         |  "); Serial.println((paramValue>>15)&0x01);

    return(paramValue);
}

