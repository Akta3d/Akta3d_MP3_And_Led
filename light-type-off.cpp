#include "light-type-off.h"

LightTypeOff::LightTypeOff(Adafruit_NeoPixel* strip)
:LightType(strip) {
}

void LightTypeOff::init() {
  _strip->clear();
  _strip->show();
}
