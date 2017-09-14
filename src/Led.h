#ifndef Led_h
#define Led_h

#include <Adafruit_NeoPixel.h>

class Led
{
  public:
    Led(Adafruit_NeoPixel strip, int id);
    void update();
    void set(uint32_t colour);
    void pulse(uint32_t colour, long duration);

  private:
    Adafruit_NeoPixel _strip;
    int _id;
    uint32_t _colour;
    uint32_t _pulseExpires;
    bool _inPulse;
};

#endif
