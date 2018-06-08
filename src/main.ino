/*
 * Perego control
 * Arduino sketch for using the Arduino as a simple state machine for an electrical toy car.
 *
 * Uses the motor library from elechouse for their "50A Dual-Channel motor drive module-Arduino Compatible"
 * motor control. Other PWM controlled motor controllers could probably also be used.
 *
 * Also uses the ResponsiveAnalogRead library from Damien Clarke too smooth out the input from the speed pedal
 *
 * Copyright (c) 2016 Niels E. Linnemann Nielsen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// import ResponsiveAnalogRead library
#include <ResponsiveAnalogRead.h>
// import motor library
#include <MOTOR.h>
// import the Metro library
#include <Metro.h>


//#define SERLOG //Turn on Serial logging

const int PEDAL_PIN = A0;
const int GEAR1_PIN = A1;
const int GEAR2_PIN = A2;
const int REV_PIN   = A3;
const int BRAKE_PIN = A4;
const int RELAY_PIN = A5;

const int LOW_GEAR = 5;
const int HIGH_GEAR = 6;

// Used to enable the pedal above a certain point when it is pressed.
// helps in making the car coast once the pedal is released.
const int enablePedal = 230;

const int punishTime = 2000; //punish for x ms if the pedal is yanked all the time

// Instanciate a metro object and set the interval to 100 milliseconds (0.1 seconds).
Metro pedalTimer = Metro(25);

// The timer for changing gear, if the gear is changed faster than this delay it wont do it.
unsigned long previousGearTime=0;

ResponsiveAnalogRead analog(PEDAL_PIN, true); // Initialize the responsive analog library to smooth input.
int pedalMinimumValue = 210; // minimum value from the pedal
int pedalMaximumValue = 780; // maximum value from the pedal
int pedalCurrentValue = pedalMinimumValue; // current value of the pedal.
int pedalPrevValue = pedalMinimumValue; // Previous value of the pedal, used to compare
int pedalDiff = 0; // diff between current value and previous value.

bool first = true; //First iteration in the loop to avoid speeder being pressed down on boot

int motorMinimumSpeed = 0;   // Lowest speed the motor can run at
int motorMaximumSpeed = 254; // Highest speed the motor can run at (max 254)
int motorCurrentValue = 0;   // The current speed value, used to feed into the motor control
int accelRate = 15; // Acceleration rate for the car
int deccelRate = 10; // Decceleration rate for the car
int brakeRate = 20; // Brake rate for the car


int gear1_button; // button state for gear 1
int gear2_button; // button state for gear 2
int revGear_button; // button state for reverse gear
int brake_button;   // button state for brake pedal

int lowGear_button; // Button state for low gear
int highGear_button; // Button state for high gear

int prevButton;

// States that our electrical car can be in.
typedef enum states{
	PARKING = 0,
	GEAR1,
	GEAR2,
	REVGEAR,
	BRAKING,
	BRAKEGEAR1,
	BRAKEGEAR2,
	BRAKEREVGEAR,
} states;

states state = PARKING; // Set the initial state for the state
states prevState = PARKING;

void setup()
{
	// motor driver initialize
	motor.begin();

	// init serial
	#ifdef SERLOG
		Serial.begin(9600);
	#endif

	// set led pin output
	pinMode(13, OUTPUT);

	//Set the input pins
	pinMode(GEAR1_PIN,INPUT);
	pinMode(GEAR2_PIN,INPUT);
	pinMode(REV_PIN,INPUT);
	pinMode(BRAKE_PIN,INPUT);
	pinMode(LOW_GEAR,INPUT);
	pinMode(HIGH_GEAR,INPUT);

	//Set the output pins
	pinMode(RELAY_PIN,OUTPUT);

	//Make the pins work as pull-ups to avoid voltage leaks.
	digitalWrite(GEAR1_PIN,HIGH);
	digitalWrite(GEAR2_PIN,HIGH);
	digitalWrite(REV_PIN,HIGH);
	digitalWrite(BRAKE_PIN,HIGH);
	digitalWrite(RELAY_PIN,HIGH);
	digitalWrite(LOW_GEAR,HIGH);
	digitalWrite(HIGH_GEAR,HIGH);

	// Start the gear timer at 0
	unsigned long previousMillis=0;
}

// The loop function is called in an endless loop
void loop()
{

	// update the ResponsiveAnalogRead object every time the timer is true
	analog.update();
	if(pedalTimer.check() == 1)
	{

		pedalCurrentValue = analog.getValue();

		//Set the value for the first loop to avoid speeder being pressed down.
		if(first)
		{
			pedalCurrentValue = pedalMinimumValue; //Reset the value to avoid wheel spin
		}

		if(state==PARKING || state==BRAKING)
		{
			pedalCurrentValue = pedalMinimumValue; //Reset the value to avoid wheel spin
		}

		pedalDiff = abs(pedalCurrentValue-pedalPrevValue);

		if(state==BRAKEGEAR1 || state==BRAKEGEAR2 || state==BRAKEREVGEAR)
		{
			pedalCurrentValue = pedalPrevValue - brakeRate;
		}
		// if the pedal is released or pressed too rapidly we control the accel/deccel
		if(pedalDiff > accelRate && (state==GEAR1 || state==GEAR2 || state==REVGEAR))
		{
			if(pedalCurrentValue > pedalPrevValue)
			{
				pedalCurrentValue = pedalPrevValue + accelRate;
			}
			else
			{
				pedalCurrentValue = pedalPrevValue - deccelRate;
			}

			pedalPrevValue = pedalCurrentValue;

		}
		else
		{
			pedalPrevValue = pedalCurrentValue;
		}
	}

	// Check for button presses and set the state accordingly
	checkButtons();

	// Get input from the analog pedal and map it to PWM output value and constrain it to avoid negative numbers.
	motorCurrentValue = map(pedalCurrentValue,pedalMinimumValue,pedalMaximumValue,motorMinimumSpeed,motorMaximumSpeed);
	motorCurrentValue = constrain(motorCurrentValue, motorMinimumSpeed, motorMaximumSpeed);

	#ifdef SERLOG
			Serial.print("State, ");
			Serial.print(state);
  #endif

	// Set the desired output depending on state.
	switch(state)
	{
		case PARKING:

			motor.disable(B);							// channel B Coast
			motor.disable(A);							// channel A Coast
			//motor.close(B);
			//motor.close(A);
			prevState = PARKING;
			break;
		case BRAKING:
			motor.brake(B);							// channel B Brake
			motor.brake(A);							// channel A Brake
			prevState = BRAKING;
			break;
		case GEAR1:
			motor.set(B, motorCurrentValue/2, FORWARD);     // channel B FORWARD rotation at half speed
			motor.set(A, motorCurrentValue/2, FORWARD);     // channel A FORWARD rotation at half speed
			prevState = GEAR1;
			break;
		case BRAKEGEAR1:
			motor.set(B, motorCurrentValue/2, FORWARD);     // channel B FORWARD rotation at half speed
			motor.set(A, motorCurrentValue/2, FORWARD);     // channel A FORWARD rotation at half speed
			prevState = BRAKEGEAR1;
			break;
		case GEAR2:
			motor.set(B, motorCurrentValue, FORWARD);     // channel B FORWARD rotation
			motor.set(A, motorCurrentValue, FORWARD);     // channel A FORWARD rotation
			prevState = GEAR2;
			break;
		case BRAKEGEAR2:
			motor.set(B, motorCurrentValue, FORWARD);     // channel B FORWARD rotation
			motor.set(A, motorCurrentValue, FORWARD);     // channel A FORWARD rotation
			prevState = BRAKEGEAR2;
			break;
		case REVGEAR:
			motor.set(B, motorCurrentValue/2, REVERSE);     // channel B REVERSE rotation
			motor.set(A, motorCurrentValue/2, REVERSE);     // channel A REVERSE rotation
			prevState = REVGEAR;
			break;
		case BRAKEREVGEAR:
			motor.set(B, motorCurrentValue/2, REVERSE);     // channel B REVERSE rotation
			motor.set(A, motorCurrentValue/2, REVERSE);     // channel A REVERSE rotation
			prevState = BRAKEREVGEAR;
			break;
	}

	first = false; //Set the variable to false after the first loop.

}

void checkButtons()
{
	unsigned long currentMillis = millis();

	#ifdef SERLOG
			Serial.print(" currentMillis, ");
			Serial.print(currentMillis);
			Serial.print(" previousGearTime, ");
			Serial.println(previousGearTime);
	#endif
	// read digital inputs
	gear1_button = digitalRead(GEAR1_PIN);
	gear2_button = digitalRead(GEAR2_PIN);
	revGear_button = digitalRead(REV_PIN);
	brake_button = digitalRead(BRAKE_PIN);

	lowGear_button = digitalRead(LOW_GEAR);
	highGear_button = digitalRead(HIGH_GEAR);

	// Here we check for the button that sets the maximum speed of the car
	if(lowGear_button == LOW)
	{
		motorMaximumSpeed = 64; // quarter speed, button at 1
	}
	else if(highGear_button == LOW)
	{
		motorMaximumSpeed = 254; // full speed, button at 2
	}
  else
	{
		motorMaximumSpeed = 127; // Half speed, button at 0
	}


	// Detect button presses, brake must be first to override the rest.
	if(brake_button == LOW && analog.getValue() < enablePedal)
	{
		state = BRAKING;
	}
	else if (gear2_button == LOW && brake_button == LOW && analog.getValue() > enablePedal)
	{
		state = BRAKEGEAR2;
	}
	else if (brake_button == LOW && analog.getValue() > enablePedal)
	{
		state = BRAKEGEAR1;
	}
	else if (revGear_button == LOW && brake_button == LOW && analog.getValue() > enablePedal)
	{
		state = BRAKEGEAR2;
	}
	else if ((unsigned long)(currentMillis - previousGearTime) <= punishTime)
	{
		state = PARKING;
	}
	else if ((analog.getValue() < enablePedal) && ((unsigned long)(currentMillis - previousGearTime) >= punishTime))
	{
		state = PARKING;
	}
	else if (revGear_button == LOW && analog.getValue() > enablePedal)
	{
		state = REVGEAR;
	}
	else if (gear2_button == LOW && analog.getValue() > enablePedal)
	{
		state = GEAR2;
	}
	else if (analog.getValue() > enablePedal)
	{
		state = GEAR1;
	}

	// Here we punish the driver if the gear leaver is yanked around all the time.
	if((prevState == GEAR1 && state == REVGEAR) || (prevState == REVGEAR && state == GEAR1))
	{
		previousGearTime = currentMillis;
		pedalCurrentValue = pedalMinimumValue;
		state = PARKING;
	}

}
