#ifndef Light_type_fadein-fadeout_h
#define Light_type_fadein-fadeout_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#include "light-type.h"
#include "rgb-struct.h"

class LightTypeFadeInFadeOut : public LightType
{
  public:
    LightTypeFadeInFadeOut(Adafruit_NeoPixel* strip, uint16_t nbStep, uint16_t speedMs);
    virtual void init();
    virtual void loop();
    
    virtual void setColor1(RGB color);
    virtual void setParam(int speedMs) {_speedMs = speedMs;};

    virtual RGB getColor1() {return _currentColor;};
    virtual int getParam() {return _speedMs;};

  private: 
    uint32_t getFadeColor();
    void setLightsColor();

  private:
    RGB _currentColor;
    uint16_t _nbStep;
    int16_t _incStep;
    uint16_t _currentStep;
    uint16_t _speedMs;   
    uint16_t _lastMillis;
};

#endif
