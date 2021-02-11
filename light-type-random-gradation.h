#ifndef Light_type_random_gradation_h
#define Light_type_random_gradation_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeRandomGradation : public LightType
{
  public:
    LightTypeRandomGradation(Adafruit_NeoPixel* strip, uint16_t speedMs);
    virtual void init();
    virtual void loop();

  private:
    double diffAbs(byte last, byte current);
    byte computeValueAt(byte last, byte current, uint16_t i);
    void renderLED(uint16_t i);
    
  private:
    uint16_t _speedMs;
    RGB _color;
    RGB _lastColor;    
    uint16_t _currentLED;
    uint16_t _lastMillis;
    
};

#endif
