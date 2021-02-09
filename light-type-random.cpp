#include "light-type-random.h"

LightTypeRandom::LightTypeRandom(Adafruit_NeoPixel* strip, int speed)
:LightType(strip) {
  _speed = speed;
}

void LightTypeRandom::init() {  
  _lastMillis = millis();
}

void LightTypeRandom::loop() {
  if(millis() - _lastMillis >= _speed) {
    for(int i = 0 ; i < _strip->numPixels() ; i += 1) {
      _strip->setPixelColor(i, random(255), random(255), random(255));
    }
    _strip->show();
    _lastMillis = millis();
  }
}
