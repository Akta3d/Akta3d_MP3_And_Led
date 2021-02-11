#ifndef Light_type_single_h
#define Light_type_single_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeSingle : public LightType
{
  public:
    LightTypeSingle(Adafruit_NeoPixel* strip);
    virtual void init();
    
    virtual void setColor1(RGB color);
    virtual void setColor2(RGB color);

  private:
    void setColor(RGB color);
    
  private:
    RGB _currentColor;
};

#endif
