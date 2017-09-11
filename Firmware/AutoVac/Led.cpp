#include "Led.h"


Led::Led(Adafruit_NeoPixel strip, int id)
{
  _strip = strip;
  _id = id;
}

void Led::update()
{
  if(_inPulse && _pulseExpires <= millis()) {
    set(_colour);
    _inPulse = false;
  }
}

void Led::set(uint32_t colour)
{
  _colour = colour;
  _strip.setPixelColor(_id, _colour);
}

void Led::pulse(uint32_t colour, long duration)
{
  _pulseExpires = millis() + duration;
  _inPulse = true;
  _strip.setPixelColor(_id, colour);
}

