#ifndef KEYS_H
#define KEYS_H


#include "Defaults.h"

#define DEBOUNCE_DELAY  500

class Keys {

  unsigned long lastDebounceTime[3];

  int isKey(KEYS key){
    int reading = digitalRead(key);
    
    if(reading == HIGH){
      return 0;
    }

    if((millis() - lastDebounceTime[key]) > DEBOUNCE_DELAY) {
      lastDebounceTime[key] = millis();
      return 1;
    }

    return 0;    
  }
  
public:
  Keys(){
    pinMode(KEY_A, INPUT_PULLUP);
    pinMode(KEY_B, INPUT_PULLUP);
    pinMode(KEY_C, INPUT_PULLUP);

    lastDebounceTime[0] = lastDebounceTime[1] = lastDebounceTime[2] = millis();
  }

  int isA(){
    return isKey(KEY_A);
  }

  int isB(){
    return isKey(KEY_B);
  }

  int isC(){
    return isKey(KEY_C);
  }
};

#endif
