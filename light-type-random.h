#ifndef Light_type_random_h
#define Light_type_random_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeRandom : public LightType
{
  public:
    LightTypeRandom(Adafruit_NeoPixel* strip, int speed);
    virtual void init();
    virtual void loop();
    
  private:
    int _speed;   
    unsigned long _lastMillis;
    
};

#endif
