#ifndef _DEFAULTS_H
#define _DEFAULTS_H
  
#define DEVICE_VERSION "1.0.0"    //Only when compiling from the IDE and downloading directly to the device
#define RELEASE

#define SD_CS 5

#define RELAY_PIN 4         //ToDo: Check if this is the right pin for the relay

typedef enum { KEY_A = 11, KEY_B, KEY_C } KEYS; //ToDo: adjust to actual hardware

#define TICK_IN_MILLIS    1000
#define MIN_TO_TICKS(x)   (x*60*1000/TICK_IN_MILLIS)
#define HR_TO_TICKS(x)    (x*3600*1000/TICK_IN_MILLIS)

#define WDT_TIMEOUT       15000 //time in mS for the WDT

// CLI Defs
#define CLI_LINE_BUF_SIZE  70   //Maximum input string length
#define CLI_MAX_NUM_ARGS   10   //Maximum number of arguments

#endif