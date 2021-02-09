#ifndef Light_type_random_gradation_h
#define Light_type_random_gradation_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeRandomGradation : public LightType
{
  public:
    LightTypeRandomGradation(Adafruit_NeoPixel* strip, int speed);
    virtual void init();
    virtual void loop();

  private:
    double diffAbs(byte last, byte current);
    byte computeValueAt(byte last, byte current, int i);
    void renderLED(int i);
    
  private:
    int _speed;
    RGB _color;
    RGB _lastColor;    
    int _currentLED;
    unsigned long _lastMillis;
    
};

#endif
