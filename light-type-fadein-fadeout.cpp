#include "light-type-fadein-fadeout.h"

LightTypeFadeInFadeOut::LightTypeFadeInFadeOut(Adafruit_NeoPixel* strip, uint16_t nbStep, uint16_t speedMs)
:LightType(strip) {
  _nbStep = nbStep;
  _speedMs = speedMs;

  _incStep = 1;
  _currentStep = 0;
  _currentColor = {0, 0, 255};
}

void LightTypeFadeInFadeOut::init() {
  _lastMillis = millis();
  setLightsColor();
}

void LightTypeFadeInFadeOut::setColor1(RGB color) {
  _currentColor = color;
  init();
}

uint32_t LightTypeFadeInFadeOut::getFadeColor() {
  int r = map(_currentStep, 0, _nbStep, 1, (uint16_t)_currentColor.r);
  int g = map(_currentStep, 0, _nbStep, 1, (uint16_t)_currentColor.g);
  int b = map(_currentStep, 0, _nbStep, 1, (uint16_t)_currentColor.b);
  return _strip->Color(r, g, b);
}

void LightTypeFadeInFadeOut::setLightsColor() {
  _strip->fill(getFadeColor(), 0);
  _strip->show();
}

void LightTypeFadeInFadeOut::loop() {
  unsigned long now = millis();
  if(now - _lastMillis >= _speedMs) {
    _currentStep = _currentStep + _incStep;

    if(_currentStep <= 0) {
      _incStep = 1;
    } else if(_currentStep >= _nbStep) {
      _incStep = -1;
    }

    setLightsColor();

    _lastMillis = now;
  }
}