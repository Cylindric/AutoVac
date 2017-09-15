/*--------------------------------------------------------------------
   This file is part of the AutoVac Project.

   AutoVac is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of
   the License, or (at your option) any later version.

   AutoVac is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with AutoVac.  If not, see
   <http://www.gnu.org/licenses/>.
   --------------------------------------------------------------------*/

#ifndef Led_h
#define Led_h

#include <Adafruit_NeoPixel.h>

class Led
{
public:
Led(Adafruit_NeoPixel strip, int id);
void update();
void set(const uint32_t colour[]);
void pulse(uint32_t colour, long duration);

private:
Adafruit_NeoPixel _strip;
int _id;
uint32_t _colour;
uint32_t _pulseExpires;
bool _inPulse;
};

#endif
