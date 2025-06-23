#ifndef ACTIONS_H
#define ACTIONS_H

#include <Adafruit_SleepyDog.h>

class Actions;

#include "Defaults.h"
#include "Cli.h"
#include "Logger.h"
#include "Dispatcher.h"
#include "Utils.h"
#include "Relay.h"
#include "Keys.h"
#include "CBUS.h"
#include "CBUSConfig.h"
#include "AudioBoard.h"

#define ACTIVITY_IDLE -1

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

  Actions() : relay(nullptr), 
              audio(nullptr), 
              dispatcher(nullptr), 
              keys(nullptr), 
              keepAlive(nullptr), 
              cbus(nullptr), 
              config(nullptr), 
              runningCount(ACTIVITY_IDLE){
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

  void checkPushButtonActivity(){
    if(runningCount>0){
      trace.log("Actions", "checkPushButtonActivity. Activity running", runningCount);
      runningCount--; //Decrement 1 and keep going
      return;
    }

    if(runningCount==0){
      //Last tick -> disable activity
      trace.log("Actions", "checkPushButtonActivity", "Activity completed");
      relay->off();
      audio->stopPlaying();
      runningCount = ACTIVITY_IDLE;
      return;
    }
  }

  void checkKeyAction(){
    if(keys->isOn()){
      trace.log("Actions", "checkKeysAction", "Key pressed");
      if(runningCount == ACTIVITY_IDLE){  //Action is IDLE, start activity
        trace.log("Actions", "checkKeysAction", "Activating relay & default audio");
        relay->on();
        audio->play(config->getDefaultAudio());
        runningCount = SEC_TO_TICKS(15);
      }
      return;
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
      } else {
        if(cmd == ACOF){
          trace.log("Actions", "Event for deactivation of relay received");
          relay->off();
        }
      }
    }

    //Check if event number is mapped to any audio file
    char * track = config->getAudioByEventNumber(eventNumber);
    if(track){
      if(cmd == ACON){
        trace.log("Actions", "Event for activation of audio received");
        audio->play(track);
        return;
      } else {
        if(cmd == ACOF){
          trace.log("Actions", "Event for deactivation of audio received");
          audio->stopPlaying();
          return;
        }
      }
    }

    // The event comes from a recognized node, but it is not mapped to any action here
    trace.log("Actions", "Unmapped event: ", eventNumber);
    return;
  };
};

#endif