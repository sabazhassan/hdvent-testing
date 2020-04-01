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
    delay(100);
    digitalWrite(4, HIGH);
    SPI.begin();             // start
    boardIndex = &Stepper;

    // Configure the boards to the settings we wish to use
    //  for them. See below for more information about the
    //  settings applied.
    spi_test();
    configureBoards();
    getBoardStatus();

}

// loop() is going to wait to receive a character from the
//  host, then do something based on that character.
void loop()
{
    char rxChar = 0;
    if (Serial.available())
    {
        rxChar = Serial.read();
        // This switch statement handles the various cases that may come from the host via the serial port.
        //  Do note that not all terminal programs will encode e.g. Page Up as 0x0B. I'm using CoolTerm
        //  in Windows and this is what CoolTerm sends when I strike these respective keys.
        //  only Stepper control (one motor) is implemented here as example
        switch(rxChar)
        {
            case 'x':  // x Arrow : speed 0
                Stepper.run(0,0);
                break;
            case 'd':  // right Arrow:  turn right
                Stepper.run(1,speed);
                break;
            case 'a':  // left Arrow:  turn left
                Stepper.run(0,speed);
                break;
            case 'w':  // Up Arrow : speed up 10
                speed = constrain(speed+10, 25, 1500);
                Serial.print("Spd:");Serial.println(speed);
                break;
            case 's':  // Down Arrow : speed down 10
                speed = constrain(speed-10, 25, 1500);
                Serial.print("Spd:");Serial.println(speed);
                break;
            case 'g':
                getBoardStatus();
                break;
            case 'h':
                getBoardConfigurations();
                break;
            case 'r':
                Stepper.resetDev();
                break;
            case 'c':
                break;
            default:
                Serial.println(".");
        }
    }
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
    // These MY5401 motors have a 4.3 ohm static winding resistance; at 12V, the current is est 3A at start
    // so we need to use the KVAL settings (see below) to trim that value back to a safe level. >>3A
    boardIndex->setOCThreshold(OCD_TH_6000mA);

    // KVAL is a modifier that sets the effective voltage applied to the motor. KVAL/255 * Vsupply = effective motor voltage.
    //  This lets us hammer the motor harder during some phases  than others, and to use a higher voltage to achieve better
    //  torqure performance even if a motor isn't rated for such a high current.
    // This IHM02A1 BOARD has 12V motors and a 12V supply.
    boardIndex->setRunKVAL(128);  // 128/255 * 12V = 6V
    boardIndex->setAccKVAL(128);  // 192/255 * 12V = 6V
    boardIndex->setDecKVAL(64);  // 128/255 * 12V = 3V
    boardIndex->setHoldKVAL(32);  // 32/255 * 12V = 1.5V  // low voltage, almost free turn

    // The dSPIN chip supports microstepping for a smoother ride. This function provides an easy front end for changing the microstepping mode.
    // once in full speed, it will step up to half-step
    boardIndex->configStepMode(STEP_FS_64);

    // When a move command is issued, max speed is the speed the Motor tops out at while completing the move, in steps/s
    boardIndex->setMaxSpeed(1500);
    boardIndex->setMinSpeed(50);

    // Acceleration and deceleration in steps/s/s. Increasing this value makes the motor reach its full speed more quickly, at the cost of possibly causing it to slip and miss steps.
    boardIndex->setAcc(500);
    boardIndex->setDec(500);




}

// Reads back the "config" register from board [i] in the series.
//  This should be 0x2e88 after a reset of the Autodriver board, or of the control board (as it resets the Autodriver at startup).
//  A reading of 0x0000 means something is wrong with your hardware.
int getBoardConfigurations()
{
    int paramValue;
    Serial.print("\n Board configurations: ");

    // It's nice to know if our board is connected okay. We can read
    //  the config register and print the result; it should be 0x2e88
    //  on startup.
    paramValue = boardIndex->getParam(CONFIG);
    Serial.print(" Config:");
    Serial.println(paramValue, HEX);
    return(paramValue);
}

// Reads back the "status" register from board [i] in the series.

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
