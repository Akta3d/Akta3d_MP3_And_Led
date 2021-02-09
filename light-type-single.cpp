#include "light-type-single.h"

LightTypeSingle::LightTypeSingle(Adafruit_NeoPixel* strip)
:LightType(strip) {
  _currentColor = {0, 0, 255};
}

void LightTypeSingle::init() {
  uint32_t stripColor = _strip->Color(_currentColor.r, _currentColor.g, _currentColor.b);
  _strip->fill(stripColor, 0, _strip->numPixels());
  _strip->show();
}

void LightTypeSingle::setColor1(RGB color) {
  setColor(color);
}

void LightTypeSingle::setColor2(RGB color) {
  setColor(color);
}

void LightTypeSingle::setColor(RGB color) {
  _currentColor = color;
  init();
}
