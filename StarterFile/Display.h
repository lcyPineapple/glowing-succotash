#ifdef __cplusplus  
extern "C" { 
#endif 

#ifndef DISPLAY_H_
#define DISPLAY_H_


#include <stdlib.h>
#include <stdbool.h>
#include <Arduino.h>
#include <Elegoo_GFX.h>
#include <Elegoo_TFTLCD.h>
#include <pin_magic.h>
#include <registers.h>
#include <TouchScreen.h>


/* Tags for the current screen displayed*/
#define MEASURE 0x00
#define ALARM 0x01
#define BATTERY 0x02

/* Assign human-readable names to some common 16-bit color values*/
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

/*Buttons Sizing*/ 
#define BUTTON_X 50
#define BUTTON_Y 250
#define BUTTON_W 70
#define BUTTON_H 30
#define BUTTON1_SPACING_X 43
#define BUTTON2_SPACING_X 77
#define BUTTON_TEXTSIZE 1

#define BATTERY_BUTTON_X 50
#define BATTERY_BUTTON_Y 100
#define BATTERY_BUTTON_W 80
#define BATTERY_BUTTON_H 50

/*Button adjustment spacing*/
#define ONE 1

/*TFT LCD Screen Configuration*/
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 120
#define TS_MAXX 900

#define TS_MINY 70
#define TS_MAXY 920

/*Touch Screen Sensitivity*/
#define MINPRESSURE 10
#define MAXPRESSURE 1000


typedef struct displayTaskData {      // Data structure for the display task, 
                                      // Stores the HVIL, and Contactor input/output pin number
                                      // Stores the current contactor state, Open or closed
    const byte* hvilPin;
    bool* contactorState;
    int* contactorLED;
    
} displayData;


void displayTask (void*);             // Function header, updates the TFT LCD screens
  

#endif

#ifdef __cplusplus 
} 
#endif 
