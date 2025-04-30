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
#include "CAN.h"
#include "AudioBoard.h"

enum Command : char {
  PLAY_AUDIO  = 'P',
  STOP_AUDIO  = 'S',
  MOTOR       = 'M',
  STATUS      = 'F'
};

class Actions {
  
  Relay * relay;
  AudioBoard * audio;
  CAN * can;
  Dispatcher<Actions> * dispatcher;
  Keys * keys;
  void (*keepAlive)();
  
public:
  // Initialize all static members
  void init(Relay * r, AudioBoard * a, CAN * can, Keys * k, Dispatcher<Actions> * d, void (*wdtCb)()){
    this->audio = a;
    this->relay = r;
    this->keys = k;
    this->can = can;
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
    audio->play("001.mp3");
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
    char commandData[MAX_CAN_COMMAND];
    int length;
    auto cmd = can->getCommand(commandData, &length);  

    if(!cmd){
      trace.log("Actions", "No command received");
      return;
    }

    commandData[length] = '\0';

    trace.logHex("Actions", "Command received: ", cmd);
    switch(cmd){
      case STOP_AUDIO:
        trace.log("Actions", "Stop playing command received");
        audio->stopPlaying();
        break;
      case PLAY_AUDIO:
        trace.log("Actions", "Play command received");
        trace.logHex("Actions", "Command data: ", commandData, length);
        //The track is part of the payload after the NodeId
        audio->play(&commandData[1]);
        break;
      case MOTOR:
        trace.log("Actions", "Relay command received");
        if(commandData[1] == '1' || commandData[1] == 'O'){
          relay->on();
        } else {
          relay->off();
        }
        break;
      default:
        trace.log("Actions", "Unknown command received");
    }
  };

  void sendStatusCANCommandAction(){
    char commandData[MAX_CAN_COMMAND];
    snprintf(commandData, sizeof(commandData), "A%cM%c", audio->isPlaying() ? '1' : '0', relay->isOn() ? '1' : '0');
    trace.log("Actions", "Sending Status CAN command", commandData);
    can->sendCommand(STATUS, commandData);
  };
};

#endif