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

// Requires the following libraries from the library manager:
// Adafruit NeoPixel
// Bounce2
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Bounce2.h>
#include <HardwareSerial.h>

#include "Led.h"
#include "PowerMeter.h"


#define STATE_MANUAL_IDLE 0
#define STATE_MANUAL_RUNNING 1
#define STATE_AUTO_IDLE 2
#define STATE_AUTO_RUNNING 3
#define STATE_AUTO_FORCED_RUNNING 4
#define STATE_AUTO_FORCED_STOPPED 5
#define STATE_AUTO_COOLING_DOWN 6

#define STATE_IDLE       0
#define STATE_FORCED_ON  1
#define STATE_FORCED_OFF 2
#define STATE_DISARMED   3
#define STATE_RUNNING    4
#define STATE_COOLING_DOWN 5


const float WH_PER_PULSE = 0.5; // The meter pulses 2,000 per kWh (0.5Wh/imp).
const long MS_PER_HOUR = 3600000;
const int VAC_WATTS = 1500; // Vacuum Power. Ideally we'll learn this from some sort of calibration eventually.
const int MIN_WATTS = 100; // The minimum trigger value. Ideally we'll learn this from some sort of calibration eventually.
const int UPDATE_INTERVAL = 500; // How many millis between updates.
const long COOLDOWN = 5000; // How many millis after an OFF before the vac can come on again

const int PULSE_PIN    = 2; // ISR Pin connected to the power meter.
const int OVERRIDE_PIN = 4; // Input Pin connected to the override button.
const int RELAY_PIN    = 7; // Output Pin connected to the main control relay
const int ARMED_PIN    = 8; // Input Pin connected to the master on/off toggle switch
const int LED_PIN      = 3; // NeoPixel Data Pin

const int POWER_LED = 0;
const int OVERRIDE_LED = 1;

// Power LED Colours
const int POWERLED_OFF[] = {0, 0, 0};
const int POWERLED_ARMED[] = {0, 100, 0};
const int POWERLED_DISARMED[] = {0, 0, 0};
const int POWERLED_RUNNING[] = {128, 0, 0};
const int POWERLED_PULSE[] = {255, 255, 0};

// Button LED Colours
const int BUTTONLED_OFF[] = {0, 0, 0}; // Off
const int BUTTONLED_AUTO[] = {0, 0, 0}; // Green
const int BUTTONLED_FORCED_OFF[] = {255, 100, 0}; // Orange
const int BUTTONLED_FORCED_ON[] = {255, 0, 0}; // Red



int currentState = STATE_DISARMED;
int targetState = STATE_DISARMED;

// Per-frame loop variables
long lastUpdateTime = 0;
long frameTime = 0;
long timeEnteredCooldown = 0;


Bounce overrideButton = Bounce();
Bounce powerToggle = Bounce();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, LED_PIN, NEO_GRB + NEO_KHZ800);

Led powerled = Led(strip, 0);
PowerMeter meter = PowerMeter();

void setup()
{
  // Set up the power toggle input
  pinMode(ARMED_PIN, INPUT_PULLUP);
  powerToggle.attach(ARMED_PIN);
  powerToggle.interval(25);

  // Set up the override button
  pinMode(OVERRIDE_PIN, INPUT_PULLUP);
  overrideButton.attach(OVERRIDE_PIN);
  overrideButton.interval(25);

  // Set up the power meter input
  meter.attach(PULSE_PIN);

  // Set up the LED output
  strip.begin();
  strip.setBrightness(64);
  powerled.set(strip.Color(0, 0, 0));
  strip.show();

  // Set up the relay output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Set up the rest
  Serial.begin(9600);
  vacuum_turn_off();
  lastUpdateTime = millis();
  Serial.println("Started.");
}


void loop()
{
  powerToggle.update();
  overrideButton.update();
  powerled.update();
  meter.update();

  update_power_readings();

  // Show a blip if a pulse was detected
  if (meter.pulseSeen()) {
    powerled.pulse(strip.Color(0, 0, 50), 50);
  }

  runstate();

  // See if we need to change state.
  //determine_next_state();

  // Implement the current state.
  //apply_state();
  strip.show();
}


void runstate() {
	switch(currentState) {
	case STATE_MANUAL_IDLE:
		currentState = instate_ManualIdle();
		break;

	case STATE_MANUAL_RUNNING:
		currentState = instate_ManualRunning();
		break;

	case STATE_AUTO_IDLE:
		currentState = instate_AutoIdle();
		break;

	case STATE_AUTO_RUNNING:
		currentState = instate_AutoRunning();
		break;

	case STATE_AUTO_FORCED_RUNNING:
		currentState = instate_AutoForcedRunning();
		break;

	case STATE_AUTO_FORCED_STOPPED:
		currentState = instate_AutoForcedStopped();
		break;

	case STATE_AUTO_COOLING_DOWN:
		currentState = instate_AutoCoolingDown();
		break;
	};
}


int instate_ManualIdle() {
  bool systemArmed = !powerToggle.read();
  bool overridePressed = overrideButton.fell();

  if(systemArmed) {
	  return STATE_AUTO_IDLE;
  }

  if(overridePressed) {
	  return STATE_MANUAL_RUNNING;
  }

  return STATE_MANUAL_IDLE;
}


int instate_ManualRunning() {
  bool systemArmed = !powerToggle.read();
  bool overridePressed = overrideButton.fell();

  if(systemArmed){
	  return STATE_AUTO_IDLE;
  }

  if(overridePressed) {
	  return STATE_MANUAL_IDLE;
  }

  return STATE_MANUAL_RUNNING;
}


int instate_AutoIdle() {
  bool systemArmed = !powerToggle.read();
  if(!systemArmed){
	  return STATE_MANUAL_IDLE;
  }

  bool overridePressed = overrideButton.fell();
  if(overridePressed) {
	  return STATE_AUTO_FORCED_RUNNING;
  }

  if(meter.averageW() > MIN_WATTS) {
	  return STATE_AUTO_RUNNING;
  }

  return STATE_AUTO_IDLE;
}


int instate_AutoRunning() {
  bool systemArmed = !powerToggle.read();
  if(!systemArmed){
	  return STATE_MANUAL_IDLE;
  }

  bool overridePressed = overrideButton.fell();
  if(overridePressed) {
	  return STATE_AUTO_FORCED_STOPPED;
  }

  if(meter.averageW() <= MIN_WATTS + VAC_WATTS) {
	  return STATE_AUTO_COOLING_DOWN;
  }

  return STATE_AUTO_RUNNING;
}


int instate_AutoForcedRunning() {
  bool systemArmed = !powerToggle.read();

  if(!systemArmed){
	  return STATE_MANUAL_IDLE;
  }

  bool overridePressed = overrideButton.fell();
  if(overridePressed) {
	  return STATE_AUTO_COOLING_DOWN;
  }

  return STATE_AUTO_FORCED_RUNNING;
}



int instate_AutoForcedStopped() {
  bool systemArmed = !powerToggle.read();

  if(!systemArmed){
	  return STATE_MANUAL_IDLE;
  }

  bool overridePressed = overrideButton.fell();
  if(overridePressed) {
	  return STATE_MANUAL_IDLE;
  }

  if(meter.averageW() <= MIN_WATTS + VAC_WATTS) {
	  return STATE_MANUAL_IDLE;
  }

  return STATE_AUTO_FORCED_STOPPED;
}


int instate_AutoCoolingDown() {
  bool systemArmed = !powerToggle.read();
  if(!systemArmed){
	  return STATE_MANUAL_IDLE;
  }

  static long cooldown_entered = -1;
  if(cooldown_entered == -1) {
	  cooldown_entered = millis();
  }

  if(millis() - cooldown_entered <= COOLDOWN) {
	  cooldown_entered = -1;
	  return STATE_MANUAL_IDLE;
  }

  return STATE_AUTO_COOLING_DOWN;
}


void set_led_colour(int id, const int colour[]) {
}


void determine_next_state()
{
  bool systemArmed = !powerToggle.read();
  bool overridePressed = overrideButton.fell();

  // IDLE ---> RUNNING
  // RUNNING ---> COOLDOWN
  // These are handled by the update_power_readings() function based on power usage.

  // DISARMED ---> IDLE
  // * ---> DISARMED
  if (systemArmed)
  {
    if (currentState == STATE_DISARMED)
    {
      Serial.println("Requesting IDLE");
      targetState = STATE_IDLE;
    }
  }
  else
  {
    if (currentState != STATE_DISARMED)
    {
      Serial.println("Requesting DISARMED");
      targetState = STATE_DISARMED;
      return;
    }
  }

  // IDLE ---> FORCED_ON
  if (currentState == STATE_IDLE)
  {
    if (overridePressed) {
      targetState = STATE_FORCED_ON;
      Serial.println("Requesting FORCED_ON");
    }
  }

  // FORCED_ON ---> IDLE
  if (currentState == STATE_FORCED_ON)
  {
    if (overridePressed) {
      targetState = STATE_IDLE;
      Serial.println("Requesting IDLE");
    }
  }

  // RUNNING ---> IDLE
  if (currentState == STATE_COOLING_DOWN)
  {
    if (millis() - timeEnteredCooldown > COOLDOWN) {
      targetState = STATE_IDLE;
      Serial.println("Requesting IDLE");
    }
  }

  // RUNNING ---> FORCED_OFF
  if (currentState == STATE_RUNNING)
  {
    if (overridePressed) {
      targetState = STATE_FORCED_OFF;
      Serial.println("Requesting FORCED_OFF");
    }
  }
}


void apply_state()
{
  if (currentState == targetState)
  {
    return;
  }

  switch (targetState)
  {
    case STATE_DISARMED:
      Serial.println("Now Disarmed.");
      vacuum_turn_off();
      powerled.set(strip.Color(POWERLED_DISARMED[0], POWERLED_DISARMED[1], POWERLED_DISARMED[2]));
      break;

    case STATE_IDLE:
      Serial.println("Now idle.");
      vacuum_turn_off();
      powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
      set_led_colour(OVERRIDE_LED, BUTTONLED_AUTO);
      break;

    case STATE_RUNNING:
      Serial.println("Now Running.");
      vacuum_turn_on();
      powerled.set(strip.Color(POWERLED_RUNNING[0], POWERLED_RUNNING[1], POWERLED_RUNNING[2]));
      break;

    case STATE_FORCED_ON:
      Serial.println("Now Overriding on.");
      vacuum_turn_on();
      powerled.set(strip.Color(POWERLED_RUNNING[0], POWERLED_RUNNING[1], POWERLED_RUNNING[2]));
      set_led_colour(OVERRIDE_LED, BUTTONLED_FORCED_ON);
      break;

    case STATE_FORCED_OFF:
      Serial.println("Now Overriding off.");
      vacuum_turn_off();
      powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
      set_led_colour(OVERRIDE_LED, BUTTONLED_FORCED_OFF);
      break;

    case STATE_COOLING_DOWN:
      Serial.println("Now Cooling Down.");
      powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
      vacuum_turn_off();
      timeEnteredCooldown = millis();
      break;
  }

  currentState = targetState;
}


void vacuum_turn_on()
{
  // Check again to make sure we are not disarmed. The FSM should prevent this anyway.
  if (currentState != STATE_DISARMED)
  {
    digitalWrite(RELAY_PIN, LOW);
  }
}

void vacuum_turn_off()
{
  digitalWrite(RELAY_PIN, HIGH);
}

void update_power_readings()
{
  frameTime = millis() - lastUpdateTime;

  if (frameTime > UPDATE_INTERVAL)
  {
    // Update required.
    float wh = meter.averageWh();
    float w = meter.averageW();

    if (currentState == STATE_IDLE && w > MIN_WATTS)
    {
      Serial.print("Requesting running because power average "); + Serial.print(w); Serial.print(" is above MIN_WATTS "); Serial.println(MIN_WATTS);
      targetState = STATE_RUNNING;
    }
    else if (currentState == STATE_RUNNING && w < MIN_WATTS + VAC_WATTS)
    {
      Serial.print("Requesting cooldown because power average "); + Serial.print(w); Serial.print(" is below MIN_WATTS+VAC_WATTS "); Serial.println(MIN_WATTS + VAC_WATTS);
      targetState = STATE_COOLING_DOWN;
      //powerAverage.clear();
    }

    Serial.print("TotalWattHours:"); Serial.print(meter.totalWh()); Serial.print("\t");
    Serial.print("averagedeltaWattHours:"); Serial.print(wh); Serial.print("\t");
    Serial.print("averagePowerConsumption:"); Serial.print(w); Serial.print("\t");
    Serial.println();

    lastUpdateTime = millis();
  }
}

