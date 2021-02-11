#include "light-type-random-gradation.h"

LightTypeRandomGradation::LightTypeRandomGradation(Adafruit_NeoPixel* strip, uint16_t speedMs)
:LightType(strip) {
  _speedMs = speedMs;
  _color = {random(255), random(255), random(255)};
  _lastColor = {random(255), random(255), random(255)};
}

void LightTypeRandomGradation::init() {  
  _currentLED = 0;
  _lastMillis = millis();
}

void LightTypeRandomGradation::loop() {
  if(millis() - _lastMillis >= _speedMs) {
    // change one led
    renderLED(_currentLED);

    // increase led number for next loop
    _currentLED += 1;
    
    if(_currentLED > _strip->numPixels()) {
      // reset to 0
      _currentLED = 0;
      
      // change color
      _lastColor = _color;
      _color = {random(255), random(255), random(255)};
    }

    _lastMillis = millis();
  }
}

double LightTypeRandomGradation::diffAbs(byte last, byte current) {
  return (double) (last > current ? last - current : current - last);
}

byte LightTypeRandomGradation::computeValueAt(byte last, byte current, uint16_t i) {
  double ratio = (double) i / (double) _strip->numPixels();
  return (current + (last > current ? 1 : -1) * (byte) (diffAbs(last, current) * ratio));
}

void LightTypeRandomGradation::renderLED(uint16_t i) {
  byte r = computeValueAt(_lastColor.r, _color.r, i);
  byte g = computeValueAt(_lastColor.g, _color.g, i);
  byte b = computeValueAt(_lastColor.b, _color.b, i);
      
  _strip->setPixelColor(i, r, g, b);
  _strip->show();
}
