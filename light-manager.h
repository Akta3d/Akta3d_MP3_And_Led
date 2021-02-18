#ifndef Light_manager_h
#define Light_manager_h

#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel
#include "light-type.h"
#include "light-type-off.h"
#include "light-type-single.h"
#include "light-type-random-gradation.h"
#include "light-type-fadein-fadeout.h"
#include "light-type-random.h"
#include "rgb-struct.h"

#define ALERT_DURATION 500

#define MAX_LIGHT_MODES 4 // should be equal to maw enum value, used during choosing a random mode

enum lightMode {
  OFF = 0,
  SINGLE = 1,
  FADE = 2,
  RANDOM_GRADATION = 3,
  RANDOM = 4,
};

class LightManager
{
  public:
    LightManager(uint16_t pin, uint16_t nbLed, neoPixelType pixelType = NEO_GRB + NEO_KHZ800);

    // need be called during arduino setup
    void setup(); 

    // choose a specific lightType
    void changeMode(uint16_t mode);

    void nextMode();
    
    // choose a random lightType
    // swith to OFF between each type
    void chooseRandomMode();

    // actions on lightType
    void loop();
    void setColor1(RGB color);
    void setColor2(RGB color);
    void setParam(int param);
    RGB getColor1();
    RGB getColor2();
    int getParam();

    // Disaply all LEDS on color during ALERT_DURATION millisecond
    void displayAlert(RGB color);

    // Can be used to notify an error by bliking the first led in RED
    void blinkRedFirstLED();

    
  private:
    uint16_t _pin;
    uint16_t _nbLed;
    uint16_t _currentMode; 
    uint16_t _previousMode;  
  
    Adafruit_NeoPixel* _strip;
    LightType* _currentLightType;

    LightTypeOff* _lightTypeOff;  
    LightTypeSingle* _lightTypeSingle;  
    LightTypeRandomGradation* _lightTypeRandomGradation;  
    LightTypeFadeInFadeOut* _lightTypeFadeInFadeOut;  
    LightTypeRandom* _lightTypeRandom; 

    uint16_t _alertMillis;
};

#endif
