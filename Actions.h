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
#include "CBUS.h"
#include "CBUSConfig.h"
#include "AudioBoard.h"

class Actions {
  
  Relay * relay;
  AudioBoard * audio;
  CBUS * cbus;
  CBUSConfig * config;
  Dispatcher<Actions> * dispatcher;
  Keys * keys;
  void (*keepAlive)();

  int runningCount;
  
public:

  Actions() : relay(nullptr), audio(nullptr), dispatcher(nullptr), keys(nullptr), keepAlive(nullptr), cbus(nullptr), config(nullptr), runningCount(0) {
  };

  // Initialize all static members
  void init(Relay * r, AudioBoard * a, CBUS * cbus, CBUSConfig * c, Keys * k, Dispatcher<Actions> * d, void (*wdtCb)()){
    this->audio = a;
    this->relay = r;
    this->keys = k;
    this->dispatcher = d;
    this->keepAlive = wdtCb;
    this->cbus = cbus;
    this->config = c;
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
    audio->play("001");
  };

  void checkKeysAction(){
    if(keys->isA()){
      trace.log("Actions", "Key A pressed");
      if(runningCount == 0){  //Button was not pressed
        startMotorAction();
        playDefaultTrackAction();
        runningCount = SEC_TO_TICKS(10);
      }
      return;
    }

    if(runningCount > 1){
      runningCount--;
      return;
    }

    if(runningCount == 1){
      stopMotorAction();
      audio->stopPlaying();
      runningCount = 0;
    }
  };

  void checkCBUSCommandAction(){
    int nodeNumber, eventNumber;
    auto cmd = cbus->getEvent(&nodeNumber, &eventNumber);

    if(!cmd){
      trace.log("Actions", "No command received");
      return;
    }

    if(nodeNumber != config->getNodeNumber()){
      trace.log("Actions", "Ignoring Event from Node: ", nodeNumber);
      return;
    }
    
    //Check if event number is mapped to the relay
    if(eventNumber == config->getRelayEventNumber()){
      if(cmd == ACON){
        trace.log("Actions", "Event for activation of relay received");
        relay->on();
        return;
      }

      if(cmd == ACOF){
        trace.log("Actions", "Event for deactivation of relay received");
        relay->off();
        return;
      }
    }

    //Check if event number is mapped to any audio file
    char * track = config->getAudioByEventNumber(eventNumber);
    if(track){
      if(cmd == ACON){
        trace.log("Actions", "Event for activation of audio received");
        audio->play(track);
        return;
      }
      if(cmd == ACOF){
        trace.log("Actions", "Event for deactivation of audio received");
        audio->stopPlaying();
        return;
      }
    }

    // The event comes from a recognized node, but it is not mapped to any action here
    trace.log("Actions", "Unmapped event: ", eventNumber);
    return;
  };
};

#endif