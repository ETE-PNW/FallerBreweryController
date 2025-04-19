#include "defaults.h"

#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SDU.h>

#include "Logger.h"
#include "dispatcher.h"
#include "Actions.h"
#include "cliDevice.h"
#include "Keys.h"
#include "relay.h"
#include "AudioBoard.h"

ConsoleLogger trace("DEBUG");
ConsoleLogger info("INFO ");
FileLogger    error("ERROR", 1);   //1: Verbose

// Core modules
Actions actions;
Dispatcher<Actions> dispatcher(&actions);
Keys keys;
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
  .keepAlive = keepAlive
};

void setup(){

  Watchdog.disable();

  Serial.begin(115200);  
  #ifndef RELEASE
    while(!Serial){}  // ONLY FOR DEBUG
  #endif
  
  // ToDo: Checks all subsystems for any (HW?) failure: SD, Logs, etc.
  // If fatal, it will halt execution
  //diagnostics.run();

  actions.init(&relay, &audio, &keys, &dispatcher, keepAlive);

  // //Common actions -> 1 TICK = 1 sec (TICK_IN_MILLIS in Defaults.h) 
  dispatcher.add("CCAN", "Looks for CAN Commands", &Actions::checkCANCommandAction, 2);
  dispatcher.add("PING", "Sends a PING CAN Command to signal status", &Actions::SendPingCANCommandAction, MIN_TO_TICKS(1));
  dispatcher.add("ACTV", "Animation activation", &Actions::activateAnimationAction, MIN_TO_TICKS(5));
  dispatcher.add("KEYS", "Check for keys", &Actions::checkKeysAction, 1);

  //Uncomment for testing actons through the CLIs
  #ifndef RELEASE
    dispatcher.disableAllActions();
  #endif

  delay(1000);  // Magic delay

  trace.log("Main", "Startup complete");
  trace.log("Main - V:", DEVICE_VERSION); 

  //Last acction is to enable WDT with ~10 seconds alarm. Anything that will take time should call "keepALive" tpo avoid a reset.
  Watchdog.enable(WDT_TIMEOUT);

  if(keys.isA() && keys.isC()){
    //ToDo: Put the device in "LearningMode"
  }
}


void loop(){
  
  //Kicks the WDT
  keepAlive();  
  
  dispatcher.dispatch();

  // If no actions, check if tehre are any commands on the terminal
  CliDevice cli(&Serial, &Serial, &context);
  cli.run();
}
