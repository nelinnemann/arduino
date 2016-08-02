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
#include "./third-party/ResponsiveAnalogRead.h"
// import motor library
#include "./third-party/MOTOR.h"

//#define SERLOG //Turn on Serial logging

const int PEDAL_PIN = A0;
const int GEAR1_PIN = A1;
const int GEAR2_PIN = A2;
const int REV_PIN   = A3;
const int BRAKE_PIN = A4;

ResponsiveAnalogRead analog(PEDAL_PIN, true); // Initialize the responsive analog library to smooth input.
int pedalMinimumValue = 190; // minimum value from the pedal
int pedalMaximumValue = 780; // maximum value from the pedal

int motorMinimumSpeed = 0;   // Lowest speed the motor can run at
int motorMaximumSpeed = 254; // Highest speed the motor can run at (max 254)
int motorCurrentValue = 0;   // The current speed value, used to feed into the motor control

int gear1_button; // button state for gear 1
int gear2_button; // button state for gear 2
int revGear_button; // button state for reverse gear
int brake_button;   // button state for brake pedal

// States that our electrical car can be in.
typedef enum states{
	PARKING = 0,
	GEAR1,
	GEAR2,
	REVGEAR,
	BRAKING,
} states;

states state = PARKING; // Set the initial state for the state

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

	//Make the pins work as pull-ups to avoid voltage leaks.
	digitalWrite(GEAR1_PIN,HIGH);
	digitalWrite(GEAR2_PIN,HIGH);
	digitalWrite(REV_PIN,HIGH);
	digitalWrite(BRAKE_PIN,HIGH);
}

// The loop function is called in an endless loop
void loop()
{
	// update the ResponsiveAnalogRead object every loop
	analog.update();
	// Check for button presses and set the state accordingly
	checkButtons();
	// Get input from the analog pedal and map it to PWM output value and constrain it to avoid negative numbers.
	motorCurrentValue = map(analog.getValue(),pedalMinimumValue,pedalMaximumValue,motorMinimumSpeed,motorMaximumSpeed);
	motorCurrentValue = constrain(motorCurrentValue, motorMinimumSpeed, motorMaximumSpeed);

	// we can use this if sentence to only start output at desired value
	if(motorCurrentValue <= motorMinimumSpeed && motorMinimumSpeed > 0)
	{
	    motorCurrentValue = 0;
	}

	// Set the desired output depending on state.
	switch(state)
	{
		case PARKING:
			motor.close(B);//set(B, 0, COAST);							// channel B Coast
			motor.close(A);//set(A, 0, COAST);							// channel A Coast
			break;
		case BRAKING:
			motor.set(B, 0, BRAKE);							// channel B Brake
			motor.set(A, 0, BRAKE);							// channel A Brake
			break;
		case GEAR1:
			motor.set(B, motorCurrentValue/2, FORWARD);     // channel B FORWARD rotation at half speed
			motor.set(A, motorCurrentValue/2, FORWARD);     // channel A FORWARD rotation at half speed
			break;
		case GEAR2:
			motor.set(B, motorCurrentValue, FORWARD);     // channel B FORWARD rotation
			motor.set(A, motorCurrentValue, FORWARD);     // channel A FORWARD rotation
			break;
		case REVGEAR:
			motor.set(B, motorCurrentValue/2, REVERSE);     // channel B REVERSE rotation
			motor.set(A, motorCurrentValue/2, REVERSE);     // channel A REVERSE rotation
			break;
	}

}

void checkButtons()
{
	// read digital inputs
	gear1_button = digitalRead(GEAR1_PIN);
	gear2_button = digitalRead(GEAR2_PIN);
	revGear_button = digitalRead(REV_PIN);
	brake_button = digitalRead(BRAKE_PIN);

	// Detect button presses, brake must be first to override the rest.
	if(brake_button == LOW)
	{
		state = BRAKING;
		#ifdef SERLOG
			Serial.print("state = BRAKING, ");
			Serial.println(state);
		#endif
	}
	else if (gear2_button == LOW && motorCurrentValue <= 2)
	{
		state = GEAR2;
		#ifdef SERLOG
			Serial.print("state = GEAR2, ");
			Serial.println(state);
		#endif
	}
	else if (revGear_button == LOW && motorCurrentValue <= 2)
	{
		state = REVGEAR;
		#ifdef SERLOG
			Serial.print("state = REVGEAR, ");
			Serial.println(state);
		#endif
	}
	else if (gear1_button == LOW && motorCurrentValue <= 2)
	{
		state = GEAR1;
		#ifdef SERLOG
			Serial.print("state = GEAR1, ");
			Serial.println(state);
		#endif
	}
	else
	{
		state = PARKING;
		#ifdef SERLOG
			Serial.print("state = PARKING, ");
			Serial.println(state);
		#endif
	}
}
