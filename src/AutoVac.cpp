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
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#include <HardwareSerial.h>

#include "Led.h"
#include "PowerMeter.h"

typedef enum {
	STATE_MANUAL_IDLE = 0,
	STATE_MANUAL_RUNNING,
	STATE_AUTO_IDLE,
	STATE_AUTO_RUNNING,
	STATE_AUTO_FORCED_RUNNING,
	STATE_AUTO_FORCED_STOPPED,
	STATE_AUTO_COOLING_DOWN
} State_type;

extern State_type instate_ManualIdle();
extern State_type instate_ManualRunning();
extern State_type instate_AutoIdle();
extern State_type instate_AutoRunning();
extern State_type instate_AutoForcedRunning();
extern State_type instate_AutoForcedStopped();
extern State_type instate_AutoCoolingDown();
void vacuum_turn_on();
void vacuum_turn_off();

State_type (*state_table[])() = {
	instate_ManualIdle,
	instate_ManualRunning,
	instate_AutoIdle,
	instate_AutoRunning,
	instate_AutoForcedRunning,
	instate_AutoForcedStopped,
	instate_AutoCoolingDown};

State_type _currentState;
bool stateEnterRan = false;

const float WH_PER_PULSE = 0.5; // The meter pulses 2,000 per kWh (0.5Wh/imp).
const long MS_PER_HOUR = 3600000;
const int VAC_WATTS = 1500; // Vacuum Power. Ideally we'll learn this from some sort of calibration eventually.
const int MIN_WATTS = 100; // The minimum trigger value. Ideally we'll learn this from some sort of calibration eventually.
const int UPDATE_INTERVAL = 500; // How many millis between updates.
const long COOLDOWN = 5000; // How many millis after an OFF before the vac can come on again

const int PULSE_PIN = 2; // ISR Pin connected to the power meter.
const int OVERRIDE_PIN = 4; // Input Pin connected to the override button.
const int RELAY_PIN = 7; // Output Pin connected to the main control relay
const int ARMED_PIN = 8; // Input Pin connected to the master on/off toggle switch
const int LED_PIN = 3; // NeoPixel Data Pin

const int POWER_LED = 0;
const int OVERRIDE_LED = 1;

// Power LED Colours
const int POWERLED_OFF[] = { 0, 0, 0 };
const int POWERLED_ARMED[] = { 0, 100, 0 };
const int POWERLED_DISARMED[] = { 0, 0, 0 };
const int POWERLED_RUNNING[] = { 128, 0, 0 };
const int POWERLED_PULSE[] = { 255, 255, 0 };

// Button LED Colours
const int BUTTONLED_OFF[] = { 0, 0, 0 }; // Off
const int BUTTONLED_AUTO[] = { 0, 0, 0 }; // Green
const int BUTTONLED_FORCED_OFF[] = { 255, 100, 0 }; // Orange
const int BUTTONLED_FORCED_ON[] = { 255, 0, 0 }; // Red

Bounce overrideButton = Bounce();
Bounce powerToggle = Bounce();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, LED_PIN, NEO_GRB + NEO_KHZ800);

Led powerled = Led(strip, 0);
Led overrideled = Led(strip, 1);
PowerMeter meter = PowerMeter();


void setup() {
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
	overrideled.set(strip.Color(0, 0, 0));
	strip.show();

	// Set up the relay output
	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, HIGH);

	// Set up the rest
	Serial.begin(9600);
	vacuum_turn_off();
	Serial.println("Started.");
}

void loop() {
	powerToggle.update();
	overrideButton.update();
	powerled.update();
	meter.update();


	// Show a blip if a pulse was detected
	if (meter.pulseSeen()) {
		Serial.print(_currentState); Serial.print(" Pulse "); Serial.print(meter.averageW()); Serial.print(" "); Serial.print(meter.totalWh()); Serial.println();
		powerled.pulse(strip.Color(0, 0, 50), 50);
	}

	_currentState = state_table[_currentState]();

	strip.show();
}



State_type changeState(State_type newState) {
	if (newState != _currentState) {
		stateEnterRan = false;
	}
	_currentState = newState;
	return newState;
}


State_type instate_ManualIdle() {
	if (!stateEnterRan) {
		Serial.println("Now Manual Idle.");
		vacuum_turn_off();
		powerled.set(POWERLED_DISARMED);
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (systemArmed) {
		Serial.println("No longer armed, switching to Manual Idle.");
		return changeState(STATE_AUTO_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		Serial.println("Override pressed, switching to Manual Running.");
		return changeState(STATE_MANUAL_RUNNING);
	}

	return STATE_MANUAL_IDLE;
}

State_type instate_ManualRunning() {
	if (!stateEnterRan) {
		Serial.println("Now Manual Running.");
		vacuum_turn_on();
		powerled.set(POWERLED_RUNNING);
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		Serial.println("No longer armed, switching to Manual Idle.");
		return changeState(STATE_AUTO_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		Serial.println("Override pressed, switching to Manual Idle.");
		return changeState(STATE_MANUAL_IDLE);
	}

	return changeState(STATE_MANUAL_RUNNING);
}

State_type instate_AutoIdle() {

	if (!stateEnterRan) {
		Serial.println("Now Idle.");
		vacuum_turn_off();
		powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
		overrideled.set(strip.Color(BUTTONLED_AUTO[0], BUTTONLED_AUTO[1], BUTTONLED_AUTO[2]));
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		Serial.println("No longer armed, switching to Manual Idle.");
		return changeState(STATE_MANUAL_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		Serial.println("Override pressed, switching to Auto Fored Running.");
		return changeState(STATE_AUTO_FORCED_RUNNING);
	}

	if (meter.averageW() > MIN_WATTS) {
		Serial.println("W above threshold, switching to Auto Running.");
		return changeState(STATE_AUTO_RUNNING);
	}

	return changeState(STATE_AUTO_IDLE);
}


State_type instate_AutoRunning() {

	if (!stateEnterRan) {
		Serial.println("Now Running.");
		vacuum_turn_on();
		powerled.set(POWERLED_RUNNING);
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		Serial.println("No longer armed, switching to Manual Idle.");
		return changeState(STATE_MANUAL_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		Serial.println("Override pressed, switching to Auto Fored Stopped.");
		return changeState(STATE_AUTO_FORCED_STOPPED);
	}

	if (meter.averageW() <= MIN_WATTS + VAC_WATTS) {
		Serial.print(meter.averageW()); Serial.print("W below threshold of "); Serial.print(MIN_WATTS + VAC_WATTS); Serial.println(", switching to Auto Cooling.");
		return changeState(STATE_AUTO_COOLING_DOWN);
	}

	return changeState(STATE_AUTO_RUNNING);
}


State_type instate_AutoForcedRunning() {

	if (!stateEnterRan) {
		Serial.println("Now Overriding on.");
		vacuum_turn_on();
		powerled.set(strip.Color(POWERLED_RUNNING[0], POWERLED_RUNNING[1], POWERLED_RUNNING[2]));
		overrideled.set(strip.Color(BUTTONLED_FORCED_ON[0], BUTTONLED_FORCED_ON[1], BUTTONLED_FORCED_ON[2]));
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		return changeState(STATE_MANUAL_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		return changeState(STATE_AUTO_COOLING_DOWN);
	}

	return changeState(STATE_AUTO_FORCED_RUNNING);
}


State_type instate_AutoForcedStopped() {

	if (!stateEnterRan) {
		Serial.println("Now Overriding off.");
		vacuum_turn_off();
		powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
		overrideled.set(strip.Color(BUTTONLED_FORCED_OFF[0], BUTTONLED_FORCED_OFF[1], BUTTONLED_FORCED_OFF[2]));
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		return changeState(STATE_MANUAL_IDLE);
	}

	bool overridePressed = overrideButton.fell();
	if (overridePressed) {
		return changeState(STATE_MANUAL_IDLE);
	}

	if (meter.averageW() <= MIN_WATTS + VAC_WATTS) {
		return changeState(STATE_MANUAL_IDLE);
	}

	return changeState(STATE_AUTO_FORCED_STOPPED);
}


State_type instate_AutoCoolingDown() {

	if (!stateEnterRan) {
		Serial.println("Now Cooling Down.");
		powerled.set(strip.Color(POWERLED_ARMED[0], POWERLED_ARMED[1], POWERLED_ARMED[2]));
		vacuum_turn_off();
		stateEnterRan = true;
	}

	bool systemArmed = true;
	if (!systemArmed) {
		return changeState(STATE_MANUAL_IDLE);
	}

	static long cooldown_entered = -1;
	if (cooldown_entered == -1) {
		cooldown_entered = millis();
	}

	if (millis() - cooldown_entered <= COOLDOWN) {
		cooldown_entered = -1;
		return changeState(STATE_MANUAL_IDLE);
	}

	return changeState(STATE_AUTO_COOLING_DOWN);
}


void vacuum_turn_on() {
	digitalWrite(RELAY_PIN, LOW);
}


void vacuum_turn_off() {
	digitalWrite(RELAY_PIN, HIGH);
}
