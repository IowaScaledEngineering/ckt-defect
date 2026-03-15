/*************************************************************************
Title:    Axle Counter Simulator
Authors:  Michael Petersen <railfan@drgw.net>
File:     axleator.ino
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2026 Michael Petersen & Nathan Holmes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#define   PINA   12
#define   PINB   11

//  Timing in ms
#define   WHEEL       50
#define   TRUCK      150
#define   BOLSTER   1285
#define   COUPLER    395

uint8_t buttonsPressed = 0;

unsigned long debounceMillis = 0;

void setup()
{
	digitalWrite(PINA, 0);
	digitalWrite(PINB, 0);
	pinMode(PINA, OUTPUT);
	pinMode(PINB, OUTPUT);
	pinMode(A0, INPUT_PULLUP);
	
	delay(100);
	buttonsPressed = !digitalRead(A0);  // pre load
}

uint8_t debounce(uint8_t debouncedState, uint8_t newInputs)
{
	static uint8_t clock_A = 0, clock_B = 0;
	uint8_t delta = newInputs ^ debouncedState; // Find all of the changes
	uint8_t changes;

	clock_A ^= clock_B; //Increment the counters
	clock_B  = ~clock_B;

	clock_A &= delta; //Reset the counters if no changes
	clock_B &= delta; //were detected.

	changes = ~((~delta) | clock_A | clock_B);
	debouncedState ^= changes;
	return(debouncedState);
}

uint8_t stateA = 0;
uint8_t stateB = 0;

unsigned long millisA = 0;
unsigned long millisB = 0;

void loop()
{
	uint8_t inputStatus;
	
	if( (millis() - debounceMillis) >= 10 )
	{
		inputStatus = !digitalRead(A0);
		buttonsPressed = debounce(buttonsPressed, inputStatus);
	}

	if(buttonsPressed)
	{
		switch(stateA)
		{
			case 0:
				digitalWrite(PINA, 1);
				millisA = millis();
				stateA++;
				break;
			case 1:
				if( (millis() - millisA) >= (WHEEL) )
				{
					digitalWrite(PINA, 0);
					millisA = millis();
					stateA++;
				}
				break;
			case 2:
				if( (millis() - millisA) >= (TRUCK-WHEEL) )
				{
					digitalWrite(PINA, 1);
					millisA = millis();
					stateA++;
				}
				break;
			case 3:
				if( (millis() - millisA) >= (WHEEL) )
				{
					digitalWrite(PINA, 0);
					millisA = millis();
					stateA++;
				}
				break;
			case 4:
				if( (millis() - millisA) >= (BOLSTER - (WHEEL + TRUCK)) )
				{
					digitalWrite(PINA, 1);
					millisA = millis();
					stateA++;
				}
				break;
			case 5:
				if( (millis() - millisA) >= (WHEEL) )
				{
					digitalWrite(PINA, 0);
					millisA = millis();
					stateA++;
				}
				break;
			case 6:
				if( (millis() - millisA) >= (TRUCK-WHEEL) )
				{
					digitalWrite(PINA, 1);
					millisA = millis();
					stateA++;
				}
				break;
			case 7:
				if( (millis() - millisA) >= (WHEEL) )
				{
					digitalWrite(PINA, 0);
					millisA = millis();
					stateA++;
				}
				break;
			case 8:
				if( (millis() - millisA) >= (COUPLER - (WHEEL + TRUCK)) )
				{
					stateA++;
				}
				break;

			default:
				stateA = 0;
				break;
		}




/*
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);
		delay(TRUCK-WHEEL);
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);

		delay(BOLSTER - (WHEEL + TRUCK));

		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);
		delay(TRUCK-WHEEL);
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);

		delay(COUPLER - (WHEEL + TRUCK));
*/

	}
}
