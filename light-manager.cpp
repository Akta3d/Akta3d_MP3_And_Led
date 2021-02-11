#include "light-manager.h"
#include "light-type-single.h"

LightManager::LightManager(uint16_t pin, uint16_t nbLed) {
  _pin = pin;
  _nbLed = nbLed;
  _currentMode = OFF;
  _previousMode = OFF;
  _alertMillis = 0;

  _strip = new Adafruit_NeoPixel(_nbLed, _pin, NEO_GRB + NEO_KHZ800);
  _lightTypeOff = new LightTypeOff(_strip);
  _lightTypeSingle = new LightTypeSingle(_strip);
  _lightTypeRandomGradation = new LightTypeRandomGradation(_strip, 100 /* speedspeedMs */);
  _lightTypeFadeInFadeOut = new LightTypeFadeInFadeOut(_strip, 25 /* nbStep */, 100 /* speedspeedMs */);
  _lightTypeRandom = new LightTypeRandom(_strip, 200 /* speedMs */);

  changeMode(SINGLE);
}

void LightManager::setup() {
  _strip->begin();
  _strip->show(); // set all led to off
}

void LightManager::changeMode(uint16_t mode) {
  switch(mode) {
    case SINGLE:
      _currentLightType = _lightTypeSingle;
      _currentMode = mode;
    break;
    case RANDOM_GRADATION:
      _currentLightType = _lightTypeRandomGradation;
      _currentMode = mode;
    break;
    case FADE:
      _currentLightType = _lightTypeFadeInFadeOut;
      _currentMode = mode;
    break;
    case RANDOM:
      _currentLightType = _lightTypeRandom;
      _currentMode = mode;
    break;
    default: // OFF
      _currentLightType = _lightTypeOff;
      _currentMode = OFF;
    break;
  };

  _currentLightType->init();
}

void LightManager::nextMode() {
  if(_currentMode != OFF) {
    _previousMode = _currentMode;
    changeMode(OFF);
  } else {
    uint16_t nextMode = _previousMode + 1;
    if(nextMode > MAX_LIGHT_MODES) {
      nextMode = 1;
    }
    changeMode((lightMode)nextMode);
  }
}

void LightManager::chooseRandomMode() {
  if(_currentMode != OFF) {
    changeMode(OFF);
  } else {
    changeMode( (lightMode)random(1, MAX_LIGHT_MODES) );
  }
}

void LightManager::loop() {
  if(_alertMillis == 0) {
    _currentLightType->loop();
  } 
  else if(
    _alertMillis != 0 &&
    _alertMillis + ALERT_DURATION < millis()
  ) {
      _alertMillis = 0;
      _strip->clear();
      _strip->show();

      changeMode(_currentMode);
  }    
}

void LightManager::setColor1(RGB color) {
  _currentLightType->setColor1(color);
}

void LightManager::setColor2(RGB color) {
  _currentLightType->setColor2(color);  
}

void LightManager::displayAlert(RGB color) {
  uint32_t stripColor = _strip->Color(color.r, color.g, color.b);
  _strip->fill(stripColor, 0, _strip->numPixels());
  _strip->show();

  _alertMillis = millis();
}
