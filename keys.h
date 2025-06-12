#ifndef KEYS_H
#define KEYS_H

#include "Defaults.h"

#define DEBOUNCE_DELAY  500

class Keys {

  unsigned long lastDebounceTime;
  
public:
  Keys(){
    pinMode(KEY_A, INPUT_PULLUP);

    lastDebounceTime = millis();
  }

  int isA(){
    int reading = digitalRead(KEY_A);
    
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
