#ifndef ACTIONS_H
#define ACTIONS_H

#include <Adafruit_SleepyDog.h>

class Actions;

#include "Defaults.h"
#include "cli.h"
#include "Logger.h"
#include "dispatcher.h"
#include "utils.h"
#include "relay.h"
#include "keys.h"
#include "AudioBoard.h"

class Actions {
  
  Relay * relay;
  AudioBoard * audio;
  Dispatcher<Actions> * dispatcher;
  Keys * keys;
  void (*keepAlive)();
  
public:
  // Initialize all static members
  void init(Relay * r, AudioBoard * a, Keys * k, Dispatcher<Actions> * d, void (*wdtCb)()){
    this->audio = a;
    this->relay = r;
    this->keys = k;
    this->dispatcher = d;
    this->keepAlive = wdtCb;
  };
  
  // Start the motor
  void startMotorAction(){
    trace.log("Actions", "Starting motor");
    relay->on();
  };

  // Stop the motor
  void stopMotorAction(){
    trace.log("Actions", "Stopping motor");
    relay->off();
  };

  // Play the default track
  void playDefaultTrackAction(){
    trace.log("Actions", "Playing default track");
    audio->play("default.mp3");
  };

  void checkKeysAction(){
    if(keys->isA()){
      trace.log("Actions", "Key A pressed: startMotorAction");
      startMotorAction();
    }
    if(keys->isB()){
      trace.log("Actions", "Key B pressedn: stopMotorAction");
      stopMotorAction();
    }
    if(keys->isC()){
      trace.log("Actions", "Key C pressed: playing default track");
      playDefaultTrackAction();
    }
  };

  void checkCANCommandAction(){
    trace.log("Actions", "Checking for CAN commands - NOT IMPLEMENTED");
  }

  void SendPingCANCommandAction(){
    trace.log("Actions", "Sending PING CAN command - NOT IMPLEMENTED");
  }
  
  void activateAnimationAction(){
    trace.log("Actions", "Activating animation - NOT IMPLEMENTED");
  }
};

#endif