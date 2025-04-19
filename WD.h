#ifndef WD_H
#define WD_H

#include <Adafruit_SleepyDog.h>
#include "Defaults.h"

class WD {
  public:
    WD(){
      //Serial.println("WD disabled");
      Watchdog.disable();
    }

    ~WD(){
      //Serial.println("WD enabled");
      Watchdog.enable(WDT_TIMEOUT);
    }
};

#endif
