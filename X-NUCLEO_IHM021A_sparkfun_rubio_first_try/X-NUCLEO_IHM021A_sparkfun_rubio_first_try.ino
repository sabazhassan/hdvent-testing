/****************************************************************************
 * X-NUCLEO_IHM021A_sparkfun_rubio_first_try.ino
 * Example for Autodriver WITH st X-NUCLEO-IHM02A1 BOARD (2x L6470 Stepper drivers)
 * DRIVERS & EXAMPLES :
 * https://github.com/sparkfun/SparkFun_AutoDriver_Arduino_Library
 * 
 * This file demonstrates the use of two Autodriver boards in one sketch, to
 * control one step motor. 
 *
 * The X-NUCLEO-IHM02A1 BOARD must be modified in order to work with the arduino uno
 * board. Remove SMD jumper SB40 and connect SMD jumper SB41 to select 5V.
 * Connect the pin D3 with pin D13 in to work the SPI interface.
 *
 * Now connect the motor to ST1, first phase to pin 2A and 1A, the second phase
 * to pin 1B and 2B. The used motor to test the functionality was 17PM-K077BP01CN 
 * from Minebea
 *
 * The software get how many L6474 chip are in the chain. After that all chip (2)
 * are initialize with the defined default parameters. At the end the arduino wait
 * for commands over the serial port (115200 baud).
 *
 * Serial por commands:
 * x -> stop motor
 * r -> turn right the motor 
 * l -> turn left the motor
 * + -> increment the speed
 * - -> decrement the speed 
 * s -> get board status
 * c -> get board configuration
 *
 * Development environment specifics:
 * Arduino 1.8.9
 * SparkFun Arduino Pro board, Autodriver V13
 * 
 * ****************************************************************************/

#include <SparkFunAutoDriver.h>
#include <SPI.h>

#define NUM_BOARDS 2

// MOTOR DEFAULT PARAMETERS
#define MAX_SPEED 1000
#define MIN_SPEED 25
#define ACCELE 25
#define DECELE 25
#define START_SPEED 500
#define SPEED_STEP 100

// The gantry we're using has five autodriver boards, but 2 drivers are on the X-NUCLEO-IHM02A1 BOARD, daisy chained
// Numbering starts from the board farthest from the
AutoDriver YAxis(0, A2, 4);  // Nr, CS, Reset => 0 , D16/A2 (PA4), D4 (PB5) for IHM02A1 board
AutoDriver XAxis(1, A2, 4);  // Nr, CS, Reset => 1 , D16/A2 (PA4), D4 (PB5) for IHM02A1 board
unsigned int xspeed = START_SPEED;
int8_t dir = -1;

// It may be useful to index over the drivers, say, to set
//  the values that should be all the same across all the
//  boards. Let's make an array of pointers to AutoDriver
//  objects so we can do just that.
AutoDriver *boardIndex[NUM_BOARDS];

void setup() 
{
  Serial.begin(115200);
  Serial.println("Hello world!");
  
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

  // Set the pointers in our global AutoDriver array to
  //  the objects we've created.
  boardIndex[1] = &YAxis;
  boardIndex[0] = &XAxis;

  // Configure the boards to the settings we wish to use
  //  for them. See below for more information about the
  //  settings applied.
  spi_test();
  configureBoards();
  
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
    //  only Yaxis control (one motor) is implemented here as example
    switch(rxChar)
    {
      case 'x':  // x Arrow : speed 0
        XAxis.run(dir,0);
        Serial.print("Stoping");
        break;
      case 'r':  // turn right
        dir = 1;
        XAxis.run(dir,xspeed);
        Serial.print("Turn right, speed: "); Serial.print(xspeed);Serial.print(" step/s");Serial.println("");
        break;
      case 'l':  //  turn left
        dir = 0;
        XAxis.run(dir,xspeed);
        Serial.print("Turn right, speed: "); Serial.print(xspeed);Serial.print(" step/s");Serial.println("");
        break;
      case '+':  // speed up
        if (dir != -1)
        {
          xspeed = constrain(xspeed+SPEED_STEP, MIN_SPEED, MAX_SPEED);
          Serial.print("Speed:");Serial.println(xspeed);
          XAxis.run(dir,xspeed);
        }
        break;
      case '-':  // speed down
        if (dir != -1)
        {
          xspeed = constrain(xspeed-SPEED_STEP, MIN_SPEED, MAX_SPEED);
          Serial.print("Speed:");Serial.println(xspeed);
          XAxis.run(dir,xspeed);
        }
        break;
      case 's':
        getBoardStatus(0);
        break;
      case 'c':
        getBoardConfigurations(0);        
        break;
      case 'y':        
          XAxis.resetDev();  
        break;
      case 'p':
          XAxis.move(0,200);
        break;   
      case '\n':
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
  Serial.print(" Tx:"); 
  Serial.print(t,HEX);
  Serial.print("/Rx:"); 
  Serial.print(val,HEX);
  if (val==0x55) 
  {
    u=t-0x55;
    t=0x6D;
  }
  delay(100);   
  }
  Serial.print("\n Chain="); 
  Serial.println(u);
  Serial.print("\n");
  return(u);
}


//  For ease of reading, we're just going to configure all the boards to the same settings. 
void configureBoards()
{
  int paramValue;
  Serial.println("Configuring boards...");
  for (int i = 0; i < NUM_BOARDS; i++)
  {
    // Before we do anything, we need to tell each device which SPI port we're using. 
    boardIndex[i]->SPIPortConnect(&SPI);

    // reset device //
    boardIndex[i]->resetDev();

    // Set the Overcurrent Threshold to 6A(max). The OC detect circuit is quite sensitive; even if the current is only momentarily
    //  exceeded during acceleration or deceleration, the driver will shutdown. This is a per channel value; it's useful to
    //  consider worst case, which is startup.
    // These MY5401 motors have a 4.3 ohm static winding resistance; at 12V, the current is est 3A at start
    // so we need to use the KVAL settings (see below) to trim that value back to a safe level. >>3A
    boardIndex[i]->setOCThreshold(OCD_TH_6000mA);
    
    // KVAL is a modifier that sets the effective voltage applied to the motor. KVAL/255 * Vsupply = effective motor voltage.
    //  This lets us hammer the motor harder during some phases  than others, and to use a higher voltage to achieve better
    //  torqure performance even if a motor isn't rated for such a high current. 
    // This IHM02A1 BOARD has 12V motors and a 12V supply.
   boardIndex[i]->setRunKVAL(255);  // 128/255 * 12V = 6V
   boardIndex[i]->setAccKVAL(255);  // 192/255 * 12V = 6V
   boardIndex[i]->setDecKVAL(255);  // 128/255 * 12V = 3V
   boardIndex[i]->setHoldKVAL(64);  // 32/255 * 12V = 1.5V  // low voltage, almost free turn

   // The dSPIN chip supports microstepping for a smoother ride. This function provides an easy front end for changing the microstepping mode.
   // once in full speed, it will step up to half-step
    boardIndex[i]->configStepMode(STEP_FS);//STEP_FS_64
    
    // When a move command is issued, max speed is the speed the Motor tops out at while completing the move, in steps/s
    boardIndex[i]->setMaxSpeed(MAX_SPEED);  
    boardIndex[i]->setMinSpeed(MIN_SPEED);
    
    // Acceleration and deceleration in steps/s/s. Increasing this value makes the motor reach its full speed more quickly, at the cost of possibly causing it to slip and miss steps.
    boardIndex[i]->setAcc(ACCELE);
    boardIndex[i]->setDec(DECELE);
    


  }
  Serial.println("Configuring boards done");
}

// Reads back the "config" register from board [i] in the series.
//  This should be 0x2e88 after a reset of the Autodriver board, or of the control board (as it resets the Autodriver at startup).
//  A reading of 0x0000 means something is wrong with your hardware.
int getBoardConfigurations(int i)
{
  int paramValue;
  Serial.print("\n Board configurations: ");
  
  // It's nice to know if our board is connected okay. We can read
  //  the config register and print the result; it should be 0x2e88
  //  on startup.
  paramValue = boardIndex[i]->getParam(CONFIG);
  Serial.print(" Config:");
  Serial.println(paramValue, HEX);
  return(paramValue);
}

// Reads back the "status" register from board [i] in the series.

int getBoardStatus(int i)
{
  int paramValue;
  Serial.print("\n Status ");

    // See Datasheet page 55 : Status register
    // MOT_STAT: 00 = stopped / 01=Acc  / 10=Dec / 11 = const speed
    // OCD = Over current Flag (Active low)
    paramValue = boardIndex[i]->getStatus();
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
