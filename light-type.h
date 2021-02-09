#ifndef Light_type_h
#define Light_type_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel
#include "rgb-struct.h"

class LightType
{
  public:    
    LightType(Adafruit_NeoPixel* strip);
    virtual void init() = 0;
    virtual void loop() {};
    virtual void setColor1(RGB color) {};
    virtual void setColor2(RGB color) {};
    virtual void setParam(int param) {};

    Adafruit_NeoPixel* _strip;
};

#endif
