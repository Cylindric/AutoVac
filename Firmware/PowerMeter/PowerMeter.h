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

#ifndef PowerMeter_h
#define PowerMeter_h

class PowerMeter
{
  public:
    PowerMeter();
    void attach(int pulsePin);
    void update();
    bool pulseSeen();
    float totalWh();
    float averageWh();
    float averageW();
    static void pulse();

  private:
    static long _timeSinceLastPulse;
    static long _lastPulseTime;
    static bool _pulseThisFrame;
    static float _totalWhSeen;
    
    int _pin;
};

#endif
