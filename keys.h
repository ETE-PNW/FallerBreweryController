#ifndef KEYS_H
#define KEYS_H

#include "Defaults.h"

#define DEBOUNCE_DELAY  500

#define PUSHBUTTON_PIN 14

class Keys {

  unsigned long lastDebounceTime;
  int keyInput;
  
public:
  Keys(int keyInput = PUSHBUTTON_PIN) : keyInput(keyInput){
    pinMode(keyInput, INPUT_PULLUP);
    lastDebounceTime = millis();
  }

  int isOn(){
    int reading = digitalRead(keyInput);
    
    if(reading == HIGH){
      return 0;
    }

    if((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      lastDebounceTime = millis();
      return 1;
    }

    return 0;
  }
};

#endif
