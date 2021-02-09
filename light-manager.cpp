#include "light-manager.h"
#include "light-type-single.h"

LightManager::LightManager(int pin, int nbLed) {
  _pin = pin;
  _nbLed = nbLed;
  _currentMode = OFF;
  _previousMode = OFF;

  _strip = new Adafruit_NeoPixel(_nbLed, _pin, NEO_GRB + NEO_KHZ800);
  _lightTypeOff = new LightTypeOff(_strip);
  _lightTypeSingle = new LightTypeSingle(_strip);
  _lightTypeRandomGradation = new LightTypeRandomGradation(_strip, 100 /* speed */);
  _lightTypeRandom = new LightTypeRandom(_strip, 200 /* speed */);
  
  changeMode(SINGLE);
}

void LightManager::setup() {
  _strip->begin();
  _strip->show(); // set all led to off
}

void LightManager::changeMode(lightMode mode) {
  switch(mode) {
    case SINGLE:
      _currentLightType = _lightTypeSingle;
      _currentMode = mode;
    break;
    case RANDOM_GRADATION:
      _currentLightType = _lightTypeRandomGradation;
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
    int nextMode = _previousMode + 1;
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
  _currentLightType->loop();
}

void LightManager::setColor1(RGB color) {
  _currentLightType->setColor1(color);
}

void LightManager::setColor2(RGB color) {
  _currentLightType->setColor1(color);  
}

void LightManager::setParam(int param) {
  _currentLightType->setParam(param);  
}
