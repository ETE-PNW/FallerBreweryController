#ifndef _DEFAULTS_H
#define _DEFAULTS_H
  
#define DEVICE_VERSION "1.0.0"
#define RELEASE

#define SD_CS 5

#define TICK_IN_MILLIS    500
#define MIN_TO_TICKS(x)   (x*60*1000/TICK_IN_MILLIS)
#define HR_TO_TICKS(x)    (x*3600*1000/TICK_IN_MILLIS)
#define SEC_TO_TICKS(x)   (x*1000/TICK_IN_MILLIS)
#define HALF_SECOND       TICK_IN_MILLIS      

#define WDT_TIMEOUT       15000 //time in mS for the WDT

// CLI Defs
#define CLI_LINE_BUF_SIZE  70   //Maximum input string length
#define CLI_MAX_NUM_ARGS   10   //Maximum number of arguments

// CAN
#define MAX_CAN_COMMAND 10

#endif