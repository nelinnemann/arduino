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


// #define SERLOG //Turn on Serial logging

const int PEDAL_PIN = A0;
const int GEAR1_PIN = A1;
const int GEAR2_PIN = A2;
const int REV_PIN   = A3;
const int BRAKE_PIN = A4;
const int RELAY_PIN = A5;

// Used to enable the pedal above a certain point when it is pressed.
// helps in making the car coast once the pedal is released.
const int enablePedal = 5;

const int punishTime = 2000; //punish for x ms if the pedal is yanked all the time

// Instanciate a metro object and set the interval to 100 milliseconds (0.1 seconds).
Metro pedalTimer = Metro(50);

// The timer for changing gear, if the gear is changed faster than this delay it wont do it.
unsigned long previousGearTime=0;

ResponsiveAnalogRead analog(PEDAL_PIN, true); // Initialize the responsive analog library to smooth input.
int pedalMinimumValue = 190; // minimum value from the pedal
int pedalMaximumValue = 780; // maximum value from the pedal
int pedalCurrentValue = pedalMinimumValue; // current value of the pedal.
int pedalPrevValue = pedalMinimumValue; // Previous value of the pedal, used to compare
int pedalDiff = 0; // diff between current value and previous value.


int motorMinimumSpeed = 0;   // Lowest speed the motor can run at
int motorMaximumSpeed = 254; // Highest speed the motor can run at (max 254)
int motorCurrentValue = 0;   // The current speed value, used to feed into the motor control
int accelRate = 10; // Acceleration rate for the car


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

	//Set the output pins
	pinMode(RELAY_PIN,OUTPUT);

	//Make the pins work as pull-ups to avoid voltage leaks.
	digitalWrite(GEAR1_PIN,HIGH);
	digitalWrite(GEAR2_PIN,HIGH);
	digitalWrite(REV_PIN,HIGH);
	digitalWrite(BRAKE_PIN,HIGH);
	digitalWrite(RELAY_PIN,HIGH);

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

		pedalDiff = abs(pedalCurrentValue-pedalPrevValue);


		// if the pedal is released or pressed too rapidly we control the accel/deccel
		if(pedalDiff > accelRate)
		{
			if(pedalCurrentValue > pedalPrevValue)
			{
				pedalCurrentValue = pedalPrevValue + accelRate;
			}
			else
			{
				pedalCurrentValue = pedalPrevValue - accelRate;
			}
			pedalPrevValue = pedalCurrentValue;

		}
		else
		{
			pedalPrevValue = pedalCurrentValue;
		}
		#ifdef SERLOG
			Serial.print("pedal = ");
			Serial.println(pedalCurrentValue);
		#endif
	}

		// Get input from the analog pedal and map it to PWM output value and constrain it to avoid negative numbers.
	motorCurrentValue = map(pedalCurrentValue,pedalMinimumValue,pedalMaximumValue,motorMinimumSpeed,motorMaximumSpeed);
	motorCurrentValue = constrain(motorCurrentValue, motorMinimumSpeed, motorMaximumSpeed);

	// Check for button presses and set the state accordingly
	checkButtons();

	// Set the desired output depending on state.
	switch(state)
	{
		case PARKING:
		  digitalWrite(RELAY_PIN,HIGH);
			// motor.set(B, 0, COAST);							// channel B Coast
			// motor.set(A, 0, COAST);							// channel A Coast
			motor.close(B);
			motor.close(A);
			prevState = PARKING;
			break;
		case BRAKING:
		  digitalWrite(RELAY_PIN,LOW);
			motor.set(B, 0, BRAKE);							// channel B Brake
			motor.set(A, 0, BRAKE);							// channel A Brake
			prevState = BRAKING;
			break;
		case GEAR1:
		  digitalWrite(RELAY_PIN,LOW);
			motor.set(B, motorCurrentValue/2, FORWARD);     // channel B FORWARD rotation at half speed
			motor.set(A, motorCurrentValue/2, FORWARD);     // channel A FORWARD rotation at half speed
			prevState = GEAR1;
			break;
		case GEAR2:
		  digitalWrite(RELAY_PIN,LOW);
			motor.set(B, motorCurrentValue, FORWARD);     // channel B FORWARD rotation
			motor.set(A, motorCurrentValue, FORWARD);     // channel A FORWARD rotation
			prevState = GEAR2;
			break;
		case REVGEAR:
		  digitalWrite(RELAY_PIN,LOW);
			motor.set(B, motorCurrentValue/2, REVERSE);     // channel B REVERSE rotation
			motor.set(A, motorCurrentValue/2, REVERSE);     // channel A REVERSE rotation
			prevState = REVGEAR;
			break;
	}

}

void checkButtons()
{
	unsigned long currentMillis = millis();

	#ifdef SERLOG
		Serial.print("time = ");
		Serial.println((unsigned long)(currentMillis - previousGearTime));
	#endif

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

	else if ((unsigned long)(currentMillis - previousGearTime) <= punishTime)
	{
		state = PARKING;
		pedalCurrentValue = pedalMinimumValue;

		#ifdef SERLOG
			Serial.print("state = PARKING, ");
			Serial.println("PUNISH");
		#endif
	}

	else if (gear2_button == LOW && motorCurrentValue > enablePedal)
	{
		state = GEAR2;

		#ifdef SERLOG
			Serial.print("state = GEAR2, ");
			Serial.println(state);
		#endif
	}
	else if (revGear_button == LOW && motorCurrentValue > enablePedal)
	{
		state = REVGEAR;

		#ifdef SERLOG
			Serial.print("state = REVGEAR, ");
			Serial.println(state);
		#endif
	}
	else if (gear1_button == LOW && motorCurrentValue > enablePedal)
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
		pedalCurrentValue = pedalMinimumValue;

		#ifdef SERLOG
			Serial.print("state = PARKING, ");
			Serial.println(state);
		#endif
	}

  // Here we punish the driver if the gear leaver is yanked around all the time.
	if( (prevState == GEAR1 && state == REVGEAR) || (prevState == REVGEAR && state == GEAR1) || (prevState == REVGEAR && state == GEAR2) || (prevState == GEAR2 && state == REVGEAR) )
	{
		previousGearTime = currentMillis;
		pedalCurrentValue = pedalMinimumValue;
		state = PARKING;
	}
	else
	{
		state = state;
	}

}
