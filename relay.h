#ifndef _RELAY_H
#define _RELAY_H

#define RELAY_PIN 11 

class Relay {
  int pin;
  int state;
public:
  Relay(int pin) : pin(RELAY_PIN){
  }

  void init(){
    pinMode(pin, OUTPUT);
    off();
  }

  int isOn(){
    return state;
  };

  void onFor(long ms){
    on();
    delay(ms);
    off();
  }

  void on(){
    digitalWrite(pin, HIGH);
    state = 1;
  }

  void off(){
    digitalWrite(pin, LOW);
    state = 0;
  }
};

#endif
