#include <stdlib.h>
#include <stdbool.h>
#include <Arduino.h>
#include <Elegoo_GFX.h>
#include <Elegoo_TFTLCD.h>
#include <pin_magic.h>
#include <registers.h>
#include <TouchScreen.h>
#include "Display.h" 


/*Global Varibles to update the display screen*/
extern Elegoo_TFTLCD tft;
extern TouchScreen ts;
extern byte currentScreen;

extern bool measureButton;
extern bool alarmButton;
extern bool batteryButton;

extern Elegoo_GFX_Button buttons[3];
extern char buttonlabels[3][9];
extern uint16_t buttoncolors[3];
extern Elegoo_GFX_Button batteryButtons[2];
extern char batteryButtonLabels[2][4];

/*Measurement, Alarm, HVIL, and Contactor data*/
extern float hvCurrent;
extern float hvVoltage;
extern float temperature;
extern bool hVIL;
extern const byte hvilPin;
extern byte hVoltInterlock;
extern byte overCurrent;
extern byte hVoltOutofRange;
extern float stateOfCharge;
extern bool contactorState;
extern int contactorLED;
extern bool contactorAck;

/*Local Copies of global data to keep track of updated values*/
/*Measurement Data*/
float localHVCurrent     = 0;
float localHVVoltage     = 0;
float localTemperature   = 0;
bool localHVIL           = 0;
const byte localHvilPin  = 22;

/*Alarm Data*/
byte localHVoltInterlock = 0;
byte localOverCurrent = 0;
byte localHVoltOutofRange = 0;

/*State Of Charge Data*/
float localStateOfCharge = 1;

/*Contactor Data*/
bool localContactorState = 0;
int localContactorLED = 53;


/*********************************************************************************
    * Function name: batteryButtonDisplay 
    * Function inputs: void
    * Function outputs: void
    * Function description: Draws two buttons, ON and OFF for the battery screen. 
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void batteryButtonDisplay (){
  
    for ( uint8_t row=0; row<2; row++ ) {
        batteryButtons[row].initButton(&tft, (row + ONE)*BATTERY_BUTTON_X + row*BATTERY_BUTTON_X, BATTERY_BUTTON_Y,     // Takes input: X,Y,Width,Height,Outline Color, Color, TextColor,
            BUTTON_W, BUTTON_H, WHITE, buttoncolors[row], BLACK,                                                        // label, and size
            batteryButtonLabels[row], BUTTON_TEXTSIZE); 
      
        batteryButtons[row].drawButton();
    }
    
    return;
    
  }

/*********************************************************************************
    * Function name: displayMeasurementScreen
    * Function inputs: void
    * Function outputs: void
    * Function description: Draws the measurement labels on the measurement screen. 
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/  
void displayMeasurementScreen (){
  
    currentScreen = MEASURE;                      // Set the current screen to measurement screen
    
    tft.fillRect(0, 0, 240, 200, BLACK);          // Set background to black
    tft.setCursor(50, 0);  
                                                                                       
    tft.setTextColor(CYAN); 
    tft.setTextSize(2);
                                                              
    tft.print("Measurements"); 
    
    tft.setTextSize(1.5);
    tft.setCursor(0, 40);
    tft.print("State of Charge: "); 
                                                                        
    tft.setCursor(0, 60);
    tft.print("Temperature: ");
    
    tft.setCursor(0, 80);
    tft.print("HV Current: ");
    
    tft.setCursor(0, 100);
    tft.print("HV Voltage: ");
    
    tft.setCursor(0, 120);
    tft.print("HVIL Status: ");
    tft.setCursor(160, 120);
    if( hVIL ){                              // Check if the dip switch is open or closed and print status
        tft.print("OPEN");
    }else{
        tft.print("CLOSED");
    }
    
    return; 
}

/*********************************************************************************
    * Function name: displayAlarmScreen
    * Function inputs: void
    * Function outputs: void
    * Function description: Draws the alarm labels on the alarm screen. 
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void displayAlarmScreen (){
  
    currentScreen = ALARM;
    tft.fillRect(0, 0, 240, 200, BLACK);
    
    tft.setCursor(75, 0);                                                                                      
    tft.setTextColor(CYAN); tft.setTextSize(2); 
                                                                  
    tft.print("Alarms");                                                                                 
    tft.setTextSize(1.5);
    
    tft.setCursor(0, 40);
    tft.print("HVIL Alarm: ");
    
    tft.setCursor(120, 40);
    tft.print("NOT ACTIVE");
    
    tft.setCursor(0, 60);
    tft.print("HV Out of Range: ");
    
    tft.setCursor(120, 60);
    tft.print("NOT ACTIVE");
    
    tft.setCursor(0, 80);
    tft.print("Overcurrent status: ");
    
    tft.setCursor(120, 80);
    tft.print("NOT ACTIVE");
    
    return;
}

/*********************************************************************************
    * Function name: displayBatteryScreen
    * Function inputs: void
    * Function outputs: void
    * Function description: Draws the battery labels on the battery screen. 
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void displayBatteryScreen (){
    
    currentScreen = BATTERY;
    tft.fillRect(0, 0, 240, 200, BLACK);
    batteryButtonDisplay();
    
    tft.setCursor(25, 0);                                                                                    
    tft.setTextColor(CYAN); tft.setTextSize(2);                                                               
    tft.print("Battery ON/OFF");                                                                                
    
    tft.setTextSize(1.5);
    tft.setCursor(0, 40);
    tft.print("Current Battery State: ");
    
    tft.setCursor(160, 40);
    tft.fillRect(160, 40, 40, 20, BLACK);
    
    if(localContactorState) {                      // Checks contactor to see if the button is pressed, and contactor is closed ON, open OFF
        tft.print("ON");                           //  check if contactor change is acknowledged: if so turn it off; if not, do nothing
        if(contactorAck){
            contactorAck = 0;
        }
        
    }
    else { 
        tft.print("OFF");
        if(contactorAck){
            contactorAck = 0;
        }
    }
    return; 
}

/*********************************************************************************
    * Function name: updateMeasurementDisplay
    * Function inputs: void
    * Function outputs: void
    * Function description: Updates the values displayed on the measurement screen. 
    *                       Checks if the local copy is updated to the global copy
    *                       if false, update the value on the screen, do nothing if
    *                       already updated.
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void updateMeasurementDisplay () {
    
    tft.setTextSize(1.5);

   // if( stateOfCharge != localStateOfCharge ) {       // Update SOC on screen if not already updated
      
        localStateOfCharge = stateOfCharge;
        tft.fillRect(160, 40, 40, 20, BLACK);
        tft.setCursor(160, 40); 
        tft.print(localStateOfCharge);
        
   // }
    if( temperature != localTemperature ){            // Update temperature on screen if not already updated
      
        localTemperature = temperature;
        tft.fillRect(160, 60, 40, 20, BLACK);
        tft.setCursor(160, 60); 
        tft.print(localTemperature);
      
    }
    if( hvCurrent != localHVCurrent ){                // Update HIVL current on screen if not already updated
      
        localHVCurrent = hvCurrent;
        tft.fillRect(160, 80, 40, 20, BLACK);
        tft.setCursor(160, 80); 
        tft.print(localHVCurrent);
      
    }
    if( hvVoltage != localHVVoltage ){                // Update HVVoltage on screen if not already updated
      
        localHVVoltage = hvVoltage;
        tft.fillRect(160, 100, 40, 20, BLACK);
        tft.setCursor(160, 100); 
        tft.print(localHVVoltage);
      
    }
    if( hVIL != localHVIL ){                          // Update HIVL status on screen if not already updated
        
        localHVIL = hVIL;
        tft.fillRect(160, 120, 40, 20, BLACK);
        tft.setCursor(160, 120);
        tft.setTextColor(CYAN);
        if( localHVIL ){                              // Check if the dip switch is open or closed and print status
            tft.print("OPEN");
        }else{
            tft.print("CLOSED");
        }
      
    }
    return;
}

/*********************************************************************************
    * Function name: updateBatteryDisplay
    * Function inputs: bool* contactorState
    * Function outputs: void
    * Function description: Updates the values displayed on the battery screen. 
    *                       Checks if the local copy is updated to the global copy
    *                       if false, update the value on the screen, do nothing if
    *                       already updated.
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void updateBatteryDisplay ( bool* contactorState ) {
  
     tft.setTextSize(1.5);
     
    if( *contactorState != localContactorState ) {    // If the contactor is not updated, update screen
        
        localContactorState = *contactorState;
        tft.fillRect(160, 40, 40, 20, BLACK);
        tft.setCursor(160, 40);
        
        if(*contactorState){                          
            tft.print("ON");                          // If contactor is closed print ON
        }
        else{ 
            tft.print("OFF");         
        }
    }
    return;
}

/*********************************************************************************
    * Function name: updateAlarmDisplay
    * Function inputs: void
    * Function outputs: void
    * Function description: Updates the values displayed on the alarm screen. 
    *                       Checks if the local copy is updated to the global copy
    *                       if false, update the value on the screen, do nothing if
    *                       already updated.
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void updateAlarmDisplay () {
    
    tft.setTextSize(1.5);
    tft.setTextColor(CYAN);
    
    if( hVoltInterlock != localHVoltInterlock ){            // Check HVIL status for updates, if 0 alarm is not active, if 1 active not acknowledged,
                                                            // if 2 active and acknowledged.
        localHVoltInterlock = hVoltInterlock;
      
        if(localHVoltInterlock == 0x00){
            tft.fillRect(120, 40, 90, 20, BLACK);
            tft.setCursor(120, 40);
            tft.print("NOT ACTIVE");
        }
        else if(localHVoltInterlock == 0x01){
            tft.fillRect(120, 40, 90, 20, BLACK);
            tft.setCursor(120, 40);
            tft.print("ACTIVE NOT ACK.");
        }
        else{
            tft.fillRect(120, 40, 90, 20, BLACK);
            tft.setCursor(120, 40);
            tft.print("ACTIVE ACK.");
        } 
    }
    
    if( hVoltOutofRange != localHVoltOutofRange ){           // Check HV out of range for updates, if 0 alarm is not active, if 1 active not acknowledged,
                                                             // if 2 active and acknowledged.
        localHVoltOutofRange = hVoltOutofRange;
        tft.fillRect(120, 60, 90, 20, BLACK);
        tft.setCursor(120, 60);
        
        if(localHVoltOutofRange == 0){
            tft.print("NOT ACTIVE");
        }
        else if(localHVoltOutofRange == 1){
            tft.print("ACTIVE NOT ACK.");
        }
        else{
            tft.print("ACTIVE ACK.");
        } 
    }
    
    if( overCurrent != localOverCurrent ){                // Check HV out of range for updates, if 0 alarm is not active, if 1 active not acknowledged,
                                                          // if 2 active and acknowledged
        localOverCurrent = overCurrent;
        tft.fillRect(120, 80, 90, 20, BLACK);
        tft.setCursor(120, 80);
      
        if(localOverCurrent == 0){
            tft.print("NOT ACTIVE");
        }
        else if(localOverCurrent == 1){
          tft.print("ACTIVE NOT ACK.");
        }
        else{
          tft.print("ACTIVE ACK.");
        } 
    }
    
    return;
}

/*********************************************************************************
    * Function name: updateDisplay
    * Function inputs: void
    * Function outputs: void
    * Function description: Gather touch screen information and update button
    *                       status.If a button is pressed update the button flag
    *                       variables so that the screen is switched to the desired
    *                       screen. If a battery ON/OFF button is pressed update 
    *                       the contactorState variable.
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void updateDisplay (){
  
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();                                                        // Capture touchscreen x, y, z pressure coordinates
    digitalWrite(13, LOW);
    
    pinMode(XM, OUTPUT);                                                              // Configure LCD screen pins
    pinMode(YP, OUTPUT);
                                                                                      // Code is based on Arduino phonecal.ino example file
    if ( p.z > MINPRESSURE && p.z < MAXPRESSURE ) {                                   // Check if sufficient pressure is applied, if it is get coordinates.
        p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
        p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
    }
    
    if( currentScreen == BATTERY ){
                                                                                      // Check if buttons are pressed
    for (uint8_t b=0; b<2; b++) {
      
        if ( batteryButtons[b].contains(p.x, p.y)) {
            
            batteryButtons[b].press(true);                                            // Button is pressed
        } else {
            batteryButtons[b].press(false);                                           // Button not pressed
        }
    }

    for ( uint8_t b=0; b<2; b++ ) {
        if ( batteryButtons[b].justReleased() ) { 
            /*buttons[b].drawButton(true); Uncomment to Draw inverted button*/        // Re-draw normal version of button if it was inverted
        }
    
        if (batteryButtons[b].justPressed()) {
            //batteryButtons[b].drawButton(true);                                     // Draw inverted version of button to indicate it was pressed

                                                                                      // OFF button is pressed,  update contactor to open
            if (b == 0) {
                contactorState = 0;
            }
        
                                                                                      // ON button is pressed, update contactor to closed
            if (b == 1) {
                contactorState = 1;
            }
        
        /*delay(100); uncomment to debounce UI*/
        }
      }
    }
   
                                                                                        // Check if measurement, alarm, or battery button is pressed
    for ( uint8_t b=0; b<3; b++ ) {
      
        if (buttons[b].contains(p.x, p.y)) {
          
            buttons[b].press(true);                                                     // Button is pressed
        } else {
            buttons[b].press(false);                                                    // Button is not pressed
        }
    }

    for ( uint8_t b=0; b<3; b++ ) {
      
        if (buttons[b].justReleased()) {
      
            /*buttons[b].drawButton(); Revert to regular button*/  
        }
    
        if (buttons[b].justPressed()) {
            /*buttons[b].drawButton(true); Uncomment to Draw inverted button*/ 

                                                                                        // measurement button is pressed, flag measurement screen
            if (b == 0) {
                measureButton = true;
            }
        
                                                                                        // alarm button is pressed, flag alarm screen
            if (b == 1) {
                alarmButton = true;
            }
                                                                                        // battery button is pressed,  flag battery screen
            if (b == 2) {
                batteryButton = true;
            }
        
        /*delay(100); uncomment for UI debounce*/
        }
    }  
}
/*********************************************************************************
    * Function name: displayTask
    * Function inputs: void* dispData
    * Function outputs: void
    * Function description: Update the screen that is displayed based on buttons
    *                       pushed on the TFT screen. Check which screen the user
    *                       is currently on. Then call functions to check if the 
    *                       values on the displayed screen needs to be updated.
    * Author(s): Leonard Shin, Leika Yamada
    ******************************************************************************/
void displayTask ( void* dispData ) {
   
    displayData* data = (displayData*) dispData;                                          // Display correct screen on button press
    updateDisplay();                                                                      // Print the main display page                                                                   
                                                                                          // Check if any buttons are pressed, then display the cooresponding screen
    if ( measureButton == true ){
      
        displayMeasurementScreen();
                                                                                          // Reset measure button to be false, so code does not repeatedly execute
        measureButton = false;  
    }
    else if ( alarmButton == true ){
      
        displayAlarmScreen();
                                                                                          // Reset alarm button to be false, so code does not repeatedly execute
        alarmButton = false;  
    }
    else if ( batteryButton == true ){
      
        displayBatteryScreen();
                                                                                          // Reset measure button to be false, so code does not repeatedly execute
        batteryButton = false;  
    }
                                                                                          // Check the current screen, then update the values on those screens
    if( currentScreen == MEASURE ){
      
      updateMeasurementDisplay();  
    }
    else if( currentScreen == ALARM ){
      
      updateAlarmDisplay();
    }
    else{
      
      updateBatteryDisplay(data->contactorState);
    } 
    
  return;
}
