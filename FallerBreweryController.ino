#include "defaults.h"

#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SDU.h>

#include "Logger.h"
#include "dispatcher.h"
#include "Actions.h"
#include "cliDevice.h"
#include "CAN.h"
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
CAN can;
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
  .can = &can,
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

  relay.init();
  audio.init();
  can.init(0xEE); //The identifier for this controller 
                  //ToDo: adjust based on the actual protocol

  actions.init(&relay, &audio, &can, &keys, &dispatcher, keepAlive);

  // //Common actions -> 1 TICK = 1 sec (TICK_IN_MILLIS in Defaults.h) 
  dispatcher.add("CCAN", "Looks for CAN Commands", &Actions::checkCANCommandAction, 1);
  dispatcher.add("STAT", "Sends a device Status command", &Actions::sendStatusCANCommandAction, 10);  // Perhaps we can run this less frequently MIN_TO_TICKS(1));
  //dispatcher.add("KEYS", "Check for keys", &Actions::checkKeysAction, 1);

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
