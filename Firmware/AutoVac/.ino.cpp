#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2017-09-13 23:21:29

#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Bounce2.h>
#include <HardwareSerial.h>
#include "Led.h"
#include "PowerMeter.h"
void setup() ;
void loop() ;
void runstate() ;
int instate_ManualIdle() ;
int instate_ManualRunning() ;
int instate_AutoIdle() ;
int instate_AutoRunning() ;
int instate_AutoForcedRunning() ;
int instate_AutoForcedStopped() ;
int instate_AutoCoolingDown() ;
void set_led_colour(int id, const int colour[]) ;
void determine_next_state() ;
void apply_state() ;
void vacuum_turn_on() ;
void vacuum_turn_off() ;
void update_power_readings() ;

#include "AutoVac.ino"


#endif
