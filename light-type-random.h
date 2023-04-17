#ifndef Light_type_random_h
#define Light_type_random_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeRandom : public LightType
{
  public:
    LightTypeRandom(Adafruit_NeoPixel* strip, uint16_t speedMs);
    virtual void init();
    virtual void loop();
    virtual void setParam(uint16_t speedMs) {_speedMs = speedMs;};
    virtual uint16_t getParam() {return _speedMs;};
    
  private:
    uint16_t _speedMs;   
    unsigned long _lastMillis;
    
};

#endif
