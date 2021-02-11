#include "light-type-random.h"

LightTypeRandom::LightTypeRandom(Adafruit_NeoPixel* strip, uint16_t speedMs)
:LightType(strip) {
  _speedMs = speedMs;
}

void LightTypeRandom::init() {  
  _lastMillis = millis();
}

void LightTypeRandom::loop() {
  if(millis() - _lastMillis >= _speedMs) {
    for(uint16_t i = 0 ; i < _strip->numPixels() ; i += 1) {
      _strip->setPixelColor(i, random(255), random(255), random(255));
    }
    _strip->show();
    _lastMillis = millis();
  }
}

