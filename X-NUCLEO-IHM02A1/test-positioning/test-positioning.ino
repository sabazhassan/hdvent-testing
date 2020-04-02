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
void configureBoards();
int getBoardConfigurations();
int getBoardStatus();
void singleMotorCycle();
void updateMotorCurveParameters(float respiratory_rate, float path_ratio, float ie_ratio);
void printMotorCurveParameters();
bool is_busy();

// motor curve for volume control operation
// inspiratory phase
float acc_in = 300; // steps/s/s
float speed_in = 100; // steps/s
float dec_in = 300; // steps/s/s
float t_in=1;
// hold time after inspiratory phase
float time_hold_plateau = 0.200; // seconds

// expiratory phase
float acc_ex = 300; // steps/s/s
float speed_ex = 100; //steps/s
float dec_ex = 300; // steps/s/s
float t_ex=1;

//float time_hold_ex = 4; // seconds
unsigned int steps_full_range = 100;
unsigned int steps_interval = 100;

// user-set parameters (later by poti)
float respiratory_rate = 15;
float path_ratio = 0.7;
float ie_ratio = 0.5;

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
    configureBoards();

}

void loop(){
    updateMotorCurveParameters(respiratory_rate, path_ratio, ie_ratio);
    printMotorCurveParameters();
    singleMotorCycle();
}

//! take user-set parameters and calculate the parameters for the motor curve,
//!
//! \param respiratory_rate - respiratory rate in breaths per minute
//! \param path_ratio - float between 0-1, setting the portion of the total path that will be driven
//! \param ie_ratio - (inhalation time)/(exhalation time)
void updateMotorCurveParameters(float respiratory_rate, float path_ratio, float ie_ratio) {
    float t_total = 60 / respiratory_rate;
    t_ex = t_total / (ie_ratio + 1);
    t_in = t_total - t_ex ;
    int steps = steps_full_range * path_ratio;
    //Serial.print("t_in: "); Serial.println(t_in);
    //Serial.print("t_ex: "); Serial.println(t_ex);

    float discriminant = sq(t_in) - 2 * (1. / acc_in + 1. / dec_in) * steps;
    //Serial.print("discriminant: "); Serial.println(discriminant);
    if (discriminant > 0) {
        speed_in = (-t_in + sqrtf(discriminant)) / -(1. / acc_in + 1. / dec_in);
        //time_hold_ex =  t_ex - ((1. / acc_ex + 1. / dec_ex)*sq(speed_ex)/2 + steps)/speed_ex;
        steps_interval = steps;
    }
}

void printMotorCurveParameters(){
    Serial.print("speed_in: "); Serial.println(speed_in);
    //Serial.print("time_hold_ex "); Serial.println(time_hold_ex);
    Serial.print("steps_interval: "); Serial.println(steps_interval);
}

void singleMotorCycle(){
    boardIndex->setMaxSpeed(speed_in);
    boardIndex->setAcc(acc_in);
    boardIndex->setDec(dec_in);

    Stepper.move(0,steps_interval);

    // ensure inspiration phase lasts t_in seconds
    unsigned long startTime = millis();
    unsigned long elapsedTime = millis()-startTime;
    while( is_busy() || elapsedTime < (t_in-time_hold_plateau)*1000){
        elapsedTime = millis()-startTime;
    };
    delay(1000*time_hold_plateau);

    boardIndex->setMaxSpeed(speed_ex);
    boardIndex->setAcc(acc_ex);
    boardIndex->setDec(dec_ex);
    Stepper.move(1, steps_interval);

    // ensure exspiration phase lasts t_ex seconds
    startTime = millis();
    elapsedTime = millis()-startTime;
    while( is_busy() || elapsedTime < t_ex*1000){
        elapsedTime = millis()-startTime;
    };

}

bool is_busy()
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
void configureBoards()
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
    boardIndex->setRunKVAL(140);  // 128/255 * 12V = 6V
    boardIndex->setAccKVAL(160);  // 192/255 * 12V = 6V
    boardIndex->setDecKVAL(140);  // 128/255 * 12V = 3V
    boardIndex->setHoldKVAL(80);  // 32/255 * 12V = 1.5V  // low voltage, almost free turn

    // The dSPIN chip supports microstepping for a smoother ride. This function provides an easy front end for changing the microstepping mode.
    // once in full speed, it will step up to half-step
    boardIndex->configStepMode(STEP_FS); // Full step

    // When a move command is issued, max speed is the speed the Motor tops out at while completing the move, in steps/s
    boardIndex->setMaxSpeed(100);
    boardIndex->setMinSpeed(0);

    // Acceleration and deceleration in steps/s/s. Increasing this value makes the motor reach its full speed more quickly, at the cost of possibly causing it to slip and miss steps.
    boardIndex->setAcc(100);
    boardIndex->setDec(100);




}

