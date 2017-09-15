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

#include "Led.h"


Led::Led(Adafruit_NeoPixel strip, int id) {
								_pulseExpires = 0;
								_inPulse = false;
								_colour = 0;
								_strip = strip;
								_id = id;
}


void Led::update() {
								if (_inPulse && _pulseExpires <= millis()) {
																_strip.setPixelColor(_id, _colour);
																_inPulse = false;
								}
}


void Led::set(const uint32_t colour[]) {
								_colour = _strip.Color(colour[0], colour[1], colour[2]);
								_strip.setPixelColor(_id, _colour);
}


void Led::pulse(uint32_t colour, long duration) {
								_pulseExpires = millis() + duration;
								_inPulse = true;
								_strip.setPixelColor(_id, colour);
}
