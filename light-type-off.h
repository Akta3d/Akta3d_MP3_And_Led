#ifndef Light_type_off_h
#define Light_type_off_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeOff : public LightType
{
  public:
    LightTypeOff(Adafruit_NeoPixel* strip);
    virtual void init();
    
  private:
};

#endif
