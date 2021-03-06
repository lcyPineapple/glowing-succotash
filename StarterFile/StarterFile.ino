#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Measurement.h"
#include "TaskControlBlock.h"
#include "StateOfCharge.h"
#include "Contactor.h"
#include "Display.h"
#include "Alarm.h"


#include <pin_magic.h>
#include <registers.h>
#include <Elegoo_GFX.h>         // Core graphics library
#include <Elegoo_TFTLCD.h>      // Hardware-specific library


/* The control pins for the LCD can be assigned to any digital or
 * analog pins...but we'll use the analog pins as this allows us to
 * double up the pins with the touch screen (see the TFT paint example).*/

#define LCD_CS A3     // Chip Select goes to Analog 3
#define LCD_CD A2     // Command/Data goes to Analog 2
#define LCD_WR A1     // LCD Write goes to Analog 1
#define LCD_RD A0     // LCD Read goes to Analog 0

#define LCD_RESET A4  // Can alternately just connect to Arduino's reset pin

#define YP A3         // must be an analog pin, use "An" notation!
#define XM A2         // must be an analog pin, use "An" notation!
#define YM 9          // can be a digital pin
#define XP 8          // can be a digital pin


#define MEASURE 0x00  // Used to keep track of which screen is displayed: Measurement screen
#define ALARM 0x01    // Used to keep track of which screen is displayed: Alarm screen
#define BATTERY 0x02  // Used to keep track of which screen is displayed: Battery screen

#define SOC 0                   // Constant SOC value
                                // Task Control Blocks
TCB measurementTCB;             // Declare measurement TCB
TCB stateOfChargeTCB;           // Declare state of charge TCB
TCB contactorTCB;               // Declare contactor TCB
TCB alarmTCB;                   // Declare alarm TCB
TCB displayTCB;                 // Declare display TCB   [Display should be last task done each cycle]

                                // Measurement Data
measurementData measure;        // Declare measurement data structure - defined in Measurement.h
float hvCurrent     = 0;        // Stores the measured current in the HVIL
float hvVoltage     = 0;        // Stores the measured voltage in the HVIL
float temperature   = 0;        // Stores the measured temperature of the system
bool hVIL           = 0;        // Stores whether or not the HVIL is closed(1) or open(0)
const byte hvilPin  = 22;       // Stores the input pin number for HVIL

                                // Alarm Data
alarmData alarmStatus;          // Declare an Alarm data structure - defined in Alarm.h
byte hVoltInterlock;            // Store the alarm status for the HVIL alarm
byte overCurrent;               // Store the overcurretn alarm status
byte hVoltOutofRange;           // Store alarm status for HV out of range

                                // State Of Charge Data
stateOfChargeData chargeState;  // Declare charge state data structure
float stateOfCharge = SOC;        

                                // Contactor Data
contactorData contactState;
bool contactorState = 0;        
int contactorLED = 53;          // Store the output pin for the contactor
bool contactorLocal = contactorState; // initialize local to be same as state
bool contactorAck;


displayData displayUpdates;                                     // Display Data structure
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);   // LCD touchscreen
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);              // Touch screen input object

byte clockTick = 0;                                             // Keep track real time, seconds between 0 and 18

                                                                                           
int taskNumber = 5;                                                                             
TCB* tasks[5]  = {&measurementTCB, &stateOfChargeTCB, &contactorTCB, &alarmTCB, &displayTCB};   // Make an array of 5 TCB tasks


Elegoo_GFX_Button buttons[3];                                 // Create an array of button objects for the display
char buttonlabels[3][9]   = {"Measures", "Alarms", "Battery"};  
uint16_t buttoncolors[3]  = {CYAN, CYAN, CYAN};
bool measureButton = 0;                                      // Flag is true when the measuremnt screen button is pushed 
bool batteryButton = 0;                                      // Flag is true when the battery screen button is pushed
bool alarmButton = 0;                                        // Flag is true when the alarm screen button is pushed
byte currentScreen = 0;                                      // Stores which screen the user is on, 0 for measurement, 1 for alarm, 2 for battery

/*Battery Screen buttons*/
Elegoo_GFX_Button batteryButtons[2];                         // Creates an array of buttons for the battery ON, OFF buttons
char batteryButtonLabels[2][4] = {"OFF", "ON"};

unsigned long time_1 = 0;

/***********************************************************************************************************************
  * Function name: loop
  * Function inputs: Sensor data, touch input
  * Function outputs: Display data and lights indicating alarm status, contactor status, sensor data, & state of charge
  * Function description: This is a round robin scheduler to run a series of tasks
  *                       that create a user interface to a battery management system
  * Author(s): Leonard Shin, Leika Yamada
  **********************************************************************************************************************/
void loop() {
    while( 1 ){
        unsigned long time_2 = millis();                                                              // Measures task start time

        if(time_2 - time_1 > 1000){
          time_1 = time_2;
          for( int i = 0; i < taskNumber/* - 1*/; i++ )                                                           
          {
            tasks[i]->task(tasks[i]->taskDataPtr);                                                              // Call all 5 tasks, measurement, SOC, contactor, alarm, display
          }
        clockTick = ( clockTick + 1 ) % 18;                                                           // Get clock tick 0 - 18 to keep system in real time
        }
        // tasks[4]->task(tasks[4]->taskDataPtr); 
        /*serialMonitor();*/                                                                          // Uncomment this line for debugging
        //unsigned long time_2 = millis();                                                            // Measures task end time
        //unsigned long time_3 = 1000 - ( time_2 - time_1 );                                          // Calculates how much to sleep in millisec, for tasks to execute in 1 sec. intervals
        //delay(time_3);                                                         
    }
}

/******************************************************************************
  * Function name:    serialMonitor
  * Function inputs:  void
  * Function outputs: void
  * Function description: This displays all of our processes to the serial
  *                       display. Used for purposes of debugging the system.
  * Author(s): Leonard Shin, Leika Yamada
  ******************************************************************************/
void serialMonitor()
{
  
      Serial.print("My State of Charge is: ");
      Serial.println(SOC, DEC);
      Serial.print("My clock is: ");
      Serial.print(clockTick, DEC);
      Serial.print("\n");

      Serial.print("My HVIL_alarm is: ");
      Serial.println(hVoltInterlock, DEC);
      Serial.print("My Overcurrent_alarm is: ");
      Serial.println(overCurrent, DEC);
      Serial.print("My HVOutofRange_alarm is: ");
      Serial.println(hVoltOutofRange, DEC);

      Serial.print("My Temperature is: ");
      Serial.println(temperature, DEC);
      Serial.print("My Current is: ");
      Serial.println(hvCurrent, DEC);
      Serial.print("My Voltage is: ");
      Serial.println(hvVoltage, DEC);
      Serial.print("My HVIL input is: ");
      Serial.println(hVIL, DEC);
}


/********************************************************************
  * Function name: setup
  * Function inputs: void
  * Function outputs: void
  * Function description: Initializes all  TCBs and the scheduler.
  *                       Sets up the TFT LCD screen to display and 
  *                       receive real time data. 
  * Author(s): Leonard Shin, Leika Yamada
  *******************************************************************/
void setup() {  

       
    /* Initialize Measurement & Sensors*/
    measure = {&hVIL, &hvilPin, &temperature, &hvCurrent, &hvVoltage};  // Initailize measure data struct with data
    measurementTCB.task = &measurementTask;                             // Store a pointer to the measurementTask update function in the TCB
    measurementTCB.taskDataPtr = &measure;                                            
    measurementTCB.next = NULL;
    measurementTCB.prev = NULL;

   
    /*Initialize Display*/
    displayUpdates = {&hvilPin, &contactorState, &contactorLED};        // Initialize display data struct with data    
    displayTCB.task = &displayTask;                                     // Store a pointer to the displayTask update function in the TCB
    displayTCB.taskDataPtr = &displayUpdates;
    displayTCB.next = NULL;
    displayTCB.prev = NULL;

 
    /*Initialize Touch Input*/
    measureButton = 1;                                                  // Initalize the measure button as pressed to start display with measure screen
    batteryButton = 0;                                                  // Battery button initialized as not pressed
    alarmButton = 0;                                                    // Alarm screen button initialized as not pressed
    currentScreen = MEASURE;                                            // Initialize start screen as measurement screen

    
    /*Initialize Contactor*/
    contactState = {&contactorState, &contactorLocal,                   // Initialize contactor data struct with contactor data
                    &contactorAck, &contactorLED};                    
    contactorTCB.task = &contactorTask;                                 // Store a pointer to the contactor task update function in the TCB                             
    contactorTCB.taskDataPtr = &contactState;
    contactorTCB.next = NULL;
    contactorTCB.prev = NULL;


    /*Initialize Alarm */
    alarmStatus = {&hVoltInterlock, &overCurrent, &hVoltOutofRange};    // Initialize alarm data struct with alarm data
    alarmTCB.task = &alarmTask;                                         // Store a pointer to the alarm task update function in the TCB
    alarmTCB.taskDataPtr = &alarmStatus;
    alarmTCB.next = NULL;
    alarmTCB.prev = NULL;

    
    /*Initialize SOC*/
    chargeState = {};                                               // Initialize state of charge data struct with state of charge boolean
    stateOfChargeTCB.task = &stateOfChargeTask;                         // Store a pointer to the soc task update function in the TCB
    stateOfChargeTCB.taskDataPtr = &chargeState;
    stateOfChargeTCB.next = NULL;
    stateOfChargeTCB.prev = NULL;


    /*Initailize input and output pins*/
    pinMode(hvilPin, INPUT);
    pinMode(contactorLED, OUTPUT);


    /*Initialize serial communication*/
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial1.setTimeout(1000);

    /*Initialize the TFT LCD screen and prepare it for display*/
    /*Identifier finder from project 1d, given in class*/
    tft.reset();                                                                                             
    
    uint16_t identifier = tft.readID();
    if(identifier == 0x9325) {
        //Serial.println(F("Found ILI9325 LCD driver"));
    } else if(identifier == 0x9328) {
        //Serial.println(F("Found ILI9328 LCD driver"));
    } else if(identifier == 0x4535) {
        //Serial.println(F("Found LGDP4535 LCD driver"));
    }else if(identifier == 0x7575) {
        //Serial.println(F("Found HX8347G LCD driver"));
    } else if(identifier == 0x9341) {
        //Serial.println(F("Found ILI9341 LCD driver"));
    } else if(identifier == 0x8357) {
        //Serial.println(F("Found HX8357D LCD driver"));
    } else if(identifier==0x0101)
    {     
        identifier=0x9341;
        //Serial.println(F("Found 0x9341 LCD driver"));
    }else {
    //Serial.print(F("Unknown LCD driver chip: "));
    //Serial.println(identifier, HEX);
    //Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    //Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    //Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    //Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    //Serial.println(F("Also if using the breakout, double-check that all wiring"));
    //Serial.println(F("matches the tutorial."));
    identifier=0x9341; 
    }
    
    tft.begin(identifier);
    tft.setRotation(2); 
    tft.fillScreen(BLACK);         
    
    unsigned long time_1 = millis();                                                                             

   /*Create scroll buttons for measurement, alarm, and battery screens*/
  for (uint8_t row=0; row<3; row++) {                                                         // Measures Screen Button button coordinates start from the center of the button
      buttons[row].initButton(&tft, BUTTON1_SPACING_X + row*BUTTON2_SPACING_X, BUTTON_Y,
                 BUTTON_W, BUTTON_H, WHITE, buttoncolors[row], BLACK,
                 buttonlabels[row], BUTTON_TEXTSIZE); 
      buttons[row].drawButton();
  }
}
