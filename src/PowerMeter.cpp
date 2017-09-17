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

#include "PowerMeter.h"
#include "Arduino.h"
#include "RunningAverage.h"
#include <Bounce2.h>

#define DEBUG_POWERMETER 1

#define WH_PER_PULSE 0.5  // The meter pulses 2,000 per kWh (0.5Wh/imp).
#define AVG_FREQ     250  // ms between stats updates
#define AVG_WINDOW   5000 // Milliseconds of sliding average window.
#define MS_PER_HOUR  3600000
#define WATT_WINDOW  5000 // Milliseconds of sliding average window for the Watt counter

RunningAverage whPerTick(AVG_WINDOW/AVG_FREQ);
RunningAverage wattsAverage(WATT_WINDOW/AVG_FREQ);


PowerMeter::PowerMeter()
{
  _sensor = Bounce();
  _timeSinceLastPulse = 0;
  _lastPulseTime = 0;
  _totalWhSeen = 0;
  _totalPulses = 0;
  _pulseThisFrame = false;
  whPerTick.fillValue(0, whPerTick.getSize());
  wattsAverage.fillValue(0, wattsAverage.getSize());
}


void PowerMeter::attach(int pin)
{
        _pin = pin;
        if(_pin != -1)
        {
                Serial.println("Attaching to meter");
                pinMode(_pin, INPUT_PULLUP);
                _sensor.attach(_pin);
              	_sensor.interval(25);
        }
        whPerTick.clear();
        wattsAverage.clear();
}


void PowerMeter::update()
{
  _sensor.update();

  if(_sensor.rose()) {
    pulse();
  }


  static long lastStatsUpdate = millis();
  static float lastUpdateWh;
  if(millis() - lastStatsUpdate > AVG_FREQ)
  {
    long frameTime = millis() - lastStatsUpdate;

    #ifdef DEBUG_POWERMETER
    Serial.print("pulses:"); Serial.print(_totalPulses); Serial.print("\t");
    Serial.print("totalWh:"); Serial.print(totalWh()); Serial.print("\t");
    Serial.print("lastWh:"); Serial.print(lastUpdateWh); Serial.print("\t");
    #endif

    float whSinceLastTick = totalWh() - lastUpdateWh;
    whPerTick.addValue(whSinceLastTick);
    #ifdef DEBUG_POWERMETER
    Serial.print("whSinceLastTick:"); Serial.print(whSinceLastTick); Serial.print("\t");
    Serial.print("whPerTick.AVG:"); Serial.print(whPerTick.getAverage(), 4); Serial.print("\t");
    #endif

    float deltaHours = (float)frameTime / MS_PER_HOUR;
    float wPerTick = whSinceLastTick / deltaHours;
    #ifdef DEBUG_POWERMETER
    Serial.print("wPerTick:"); Serial.print(wPerTick, 0); Serial.print("\t");
    #endif
    wattsAverage.addValue(wPerTick);

    #ifdef DEBUG_POWERMETER
    Serial.print("wPerTick.AVG:"); Serial.print(wattsAverage.getAverage(), 0); Serial.print("\t");
    Serial.println();
    #endif

    lastUpdateWh = totalWh();
    lastStatsUpdate = millis();
  }

}


bool PowerMeter::pulseSeen()
{
  if(_pulseThisFrame) {
    _pulseThisFrame = false;
    return true;
  }

  return _pulseThisFrame;
}


float PowerMeter::averageWh()
{
        return max(0, whPerTick.getAverage());
}


float PowerMeter::averageW()
{
        return max(0, wattsAverage.getAverage());
}


float PowerMeter::totalWh()
{
        return max(0, _totalWhSeen);
}


void PowerMeter::pulse()
{
        _totalPulses++;
        _timeSinceLastPulse = millis() - _lastPulseTime;
        _totalWhSeen += WH_PER_PULSE;
        _lastPulseTime = millis();
        _pulseThisFrame = true;
}
