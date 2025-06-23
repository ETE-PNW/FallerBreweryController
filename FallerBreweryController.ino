#include "defaults.h"

#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SDU.h>

#include "Logger.h"
#include "Dispatcher.h"
#include "Actions.h"
#include "CliDevice.h"
#include "CBUS.h"
#include "Keys.h"
#include "Relay.h"
#include "AudioBoard.h"
#include "CBUSConfig.h"

ConsoleLogger trace("DEBUG");
ConsoleLogger info("INFO ");
FileLogger    error("ERROR", 1);   //1: Verbose

// Core modules
Actions actions;
Dispatcher<Actions> dispatcher(&actions);
Keys keys;
CBUS cbus;
CBUSConfig config;
Relay relay;
AudioBoard audio;

/*
  keepAlive is a callback for long running functions that might need to notify the WDT
  that they are still working. We could have called WDT directly, but we want a level
  of indirection in case things change.
*/
void keepAlive(){
  Watchdog.reset();
}

static CliContext context = {
  .relay = &relay,
  .audio = &audio,
  .dispatcher = &dispatcher,
  .config = &config,
  .keepAlive = keepAlive
};

static CliDevice cli(&Serial, &Serial, &context);

void setup(){

  Watchdog.disable();

  Serial.begin(115200);  
  #ifndef RELEASE
    while(!Serial){}  // ONLY FOR DEBUG
  #endif
  
  //Initialize hardware & halt if any failures (can't function with these modules down)
  relay.init();
  auto ret = audio.init();
  ret += config.init("CBCFG.TXT");
  ret += cbus.init();

  if(ret > 0){
    trace.log("Main", "Initialization failed. Halting execution");
    while(1){
      delay(10);
    }
  }

  actions.init(&relay, &audio, &cbus, &config, &keys, &dispatcher, keepAlive);

  // //Common actions -> 1 TICK = 1 sec (TICK_IN_MILLIS in Defaults.h) 
  dispatcher.add("CBUS", "Looks for CBUS Commands", &Actions::checkCBUSCommandAction, SEC_TO_TICKS(1));
  dispatcher.add("KEYS", "Check for Pushbutton press", &Actions::checkKeyAction, HALF_SECOND);       //These 2 tasks work jointly, and must have the same scheduling
  dispatcher.add( "ACTI", "Checks module activity", &Actions::checkPushButtonActivity, HALF_SECOND);

  //Uncomment for testing actions through the CLIs
  #ifndef RELEASE
    dispatcher.disableAllActions();
  #endif

  trace.log("Main", "Startup complete");
  trace.log("Main - V:", DEVICE_VERSION); 

  //Last acction is to enable WDT with ~10 seconds alarm. Anything that will take time should call "keepALive" tpo avoid a reset.
  Watchdog.enable(WDT_TIMEOUT);
}

void loop(){
  
  //Kicks the WDT
  keepAlive();  
  
  dispatcher.dispatch();

  // If no actions, check if tehre are any commands on the terminal
  cli.run();
}
