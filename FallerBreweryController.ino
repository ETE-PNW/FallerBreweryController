#include "defaults.h"

#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SDU.h>

#include "Logger.h"
#include "dispatcher.h"
#include "Actions.h"
#include "cliDevice.h"
#include "CBUS.h"
#include "Keys.h"
#include "relay.h"
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
Relay relay(RELAY_PIN);
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
  dispatcher.add("CBUS", "Looks for CBUS Commands", &Actions::checkCBUSCommandAction, 2);
  dispatcher.add("KEYS", "Check for keys", &Actions::checkKeysAction, 1);

  //Uncomment for testing actions through the CLIs
  #ifndef RELEASE
    dispatcher.disableAllActions();
  #endif

  delay(1000);  // Magic delay

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
  CliDevice cli(&Serial, &Serial, &context);
  cli.run();
}
