#include "PowerMeter.h"

#define METER_PIN -1 

PowerMeter meter = PowerMeter();

long lastStatusUpdate = 0;

void setup()
{  
  Serial.begin(9600);

  meter.attach(METER_PIN);
}


void loop() 
{
  meter.update();

  // Simulate some activity
  static long lastPulseTime = millis();
  if(millis() - lastPulseTime >= 900)
  {
    lastPulseTime = millis();
    PowerMeter::pulse();
  }





  if(meter.pulseSeen())
  {
    Serial.println("Pulse");
  }

  if(millis() - lastStatusUpdate > 1000)
  {
    //Serial.print(meter.totalWh()); Serial.print("\t");
    //Serial.print(meter.averageWh()); Serial.print("\t");
    //Serial.print(millis()); Serial.print("\t");
    //Serial.print(meter.averageW()); Serial.print("\t");
    //Serial.println();
    lastStatusUpdate = millis();
  }
}

