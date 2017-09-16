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

//#define DEBUG_POWERMETER 1

#define WH_PER_PULSE 0.5  // The meter pulses 2,000 per kWh (0.5Wh/imp).
#define AVG_FREQ     500  // ms between stats updates
#define AVG_WINDOW   5000 // Milliseconds of sliding average window.
#define MS_PER_HOUR  3600000
#define WATT_WINDOW  5000 // Milliseconds of sliding average window for the Watt counter

RunningAverage powerAverage(AVG_WINDOW/AVG_FREQ);
RunningAverage wattsAverage(WATT_WINDOW/AVG_FREQ);


PowerMeter::PowerMeter()
{
  _sensor = Bounce();
  _timeSinceLastPulse = 0;
  _lastPulseTime = 0;
  _totalWhSeen = 0;
  _totalPulses = 0;
  _pulseThisFrame = false;
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
                // attachInterrupt(digitalPinToInterrupt(_pin), pulse, RISING);
        }
        powerAverage.clear();
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
    Serial.print("lastWh:"); Serial.print(lastUpdateWh); Serial.print("\t\t");
    #endif

    float whSinceLastUpdate = totalWh() - lastUpdateWh;
    powerAverage.addValue(whSinceLastUpdate);
    #ifdef DEBUG_POWERMETER
    Serial.print("whSinceLastUpdate:"); Serial.print(whSinceLastUpdate); Serial.print("\t");
    Serial.print("powerAverage:"); Serial.print(powerAverage.getAverage(), 4); Serial.print("\t");
    #endif

    float deltaHours = (float)frameTime / MS_PER_HOUR;
    //float deltaHours =  (float)(powerAverage.getCount() * AVG_FREQ) / MS_PER_HOUR;
    float averagePowerConsumption = (float)powerAverage.getAverage() / deltaHours;
    wattsAverage.addValue(averagePowerConsumption);
    #ifdef DEBUG_POWERMETER
    Serial.print("averagePowerConsumption:"); Serial.print(averagePowerConsumption, 4); Serial.print("\t");
    Serial.print("wattsAverage:");
    Serial.print(0); Serial.print("\t");
    Serial.print(wattsAverage.getAverage(), 4); Serial.print("\t");
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
        return max(0, powerAverage.getAverage());
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
