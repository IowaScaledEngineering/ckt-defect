/*************************************************************************
Title:    Axle Counter Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     axle.cpp
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

#include <Arduino.h>

#include "common.h"
#include "io.h"
#include "axle.h"

volatile uint32_t axleCount[NUM_TRACKS];
volatile unsigned long firstAxleTime[NUM_TRACKS];
volatile unsigned long currentAxleTime[NUM_TRACKS];
volatile unsigned long entranceDeltaMicros[NUM_TRACKS];
volatile unsigned long exitDeltaMicros[NUM_TRACKS];

enum class AxleIsrState
{
	IDLE,
	SPEED_1,
	COUNT_1,
	SPEED_2,
	COUNT_2,
};

volatile AxleIsrState axleIsrState[NUM_TRACKS];

/*
   ISR Notes:

   Capture time first thing to avoid jitter from the state machine.

   Exit time can give weird intermediate results, if the truck wheelbase is
      shorter than the sensor distance.  But, in the end, it will always
      give the delta of the last wheel (assuming no backup moves, in which
      case it's screwed up anyway).
   
   The state machines will always end up stuck in either the COUNT_1 or
      COUNT_2 states.  axleReset() should be called to clean up once
      timeout is detected in the main loop and the information is read.
*/

void IRAM_ATTR axle_A1_isr(void *arg)
{
	unsigned long time = micros();
	switch(axleIsrState[0])
	{
		case AxleIsrState::IDLE:
			axleCount[0] = axleCount[0] + 1;
			firstAxleTime[0] = time;
			currentAxleTime[0] = time;
			axleIsrState[0] = AxleIsrState::SPEED_1;
			break;
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case AxleIsrState::SPEED_1:
		case AxleIsrState::COUNT_1:
		#pragma GCC diagnostic pop
			axleCount[0] = axleCount[0] + 1;
			currentAxleTime[0] = time;
			break;
		case AxleIsrState::SPEED_2:
			entranceDeltaMicros[0] = time - firstAxleTime[0];
			axleIsrState[0] = AxleIsrState::COUNT_2;
			break;
		case AxleIsrState::COUNT_2:
			exitDeltaMicros[0] = time - currentAxleTime[0];
			break;
	}
}

void IRAM_ATTR axle_A2_isr(void *arg)
{
	unsigned long time = micros();
	switch(axleIsrState[0])
	{
		case AxleIsrState::IDLE:
			axleCount[0] = axleCount[0] + 1;
			firstAxleTime[0] = time;
			currentAxleTime[0] = time;
			axleIsrState[0] = AxleIsrState::SPEED_2;
			break;
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case AxleIsrState::SPEED_2:
		case AxleIsrState::COUNT_2:
		#pragma GCC diagnostic pop
			axleCount[0] = axleCount[0] + 1;
			currentAxleTime[0] = time;
			break;
		case AxleIsrState::SPEED_1:
			entranceDeltaMicros[0] = time - firstAxleTime[0];
			axleIsrState[0] = AxleIsrState::COUNT_1;
			break;
		case AxleIsrState::COUNT_1:
			exitDeltaMicros[0] = time - currentAxleTime[0];
			break;
	}
}

void IRAM_ATTR axle_B1_isr(void *arg)
{
	unsigned long time = micros();
	switch(axleIsrState[1])
	{
		case AxleIsrState::IDLE:
			axleCount[1] = axleCount[1] + 1;
			firstAxleTime[1] = time;
			currentAxleTime[1] = time;
			axleIsrState[1] = AxleIsrState::SPEED_1;
			break;
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case AxleIsrState::SPEED_1:
		case AxleIsrState::COUNT_1:
		#pragma GCC diagnostic pop
			axleCount[1] = axleCount[1] + 1;
			currentAxleTime[1] = time;
			break;
		case AxleIsrState::SPEED_2:
			entranceDeltaMicros[1] = time - firstAxleTime[1];
			axleIsrState[1] = AxleIsrState::COUNT_2;
			break;
		case AxleIsrState::COUNT_2:
			exitDeltaMicros[1] = time - currentAxleTime[1];
			break;
	}
}

void IRAM_ATTR axle_B2_isr(void *arg)
{
	unsigned long time = micros();
	switch(axleIsrState[1])
	{
		case AxleIsrState::IDLE:
			axleCount[1] = axleCount[1] + 1;
			firstAxleTime[1] = time;
			currentAxleTime[1] = time;
			axleIsrState[1] = AxleIsrState::SPEED_2;
			break;
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
		case AxleIsrState::SPEED_2:
		case AxleIsrState::COUNT_2:
		#pragma GCC diagnostic pop
			axleCount[1] = axleCount[1] + 1;
			currentAxleTime[1] = time;
			break;
		case AxleIsrState::SPEED_1:
			entranceDeltaMicros[1] = time - firstAxleTime[1];
			axleIsrState[1] = AxleIsrState::COUNT_1;
			break;
		case AxleIsrState::COUNT_1:
			exitDeltaMicros[1] = time - currentAxleTime[1];
			break;
	}
}

void axleInit()
{
	gpio_install_isr_service(0);
	gpio_isr_handler_add(AXLE_A1, axle_A1_isr, NULL);
	gpio_isr_handler_add(AXLE_A2, axle_A2_isr, NULL);
	gpio_isr_handler_add(AXLE_B1, axle_B1_isr, NULL);
	gpio_isr_handler_add(AXLE_B2, axle_B2_isr, NULL);
}

uint32_t axleGetCount(uint32_t track)
{
	return axleCount[track];
}

// micros() returns an unsigned long
// On ESP32, unsigned long = uint32_t so these returns will be atomic
unsigned long axleGetEntranceDeltaMicros(uint32_t track)
{
	return entranceDeltaMicros[track];
}

unsigned long axleGetExitDeltaMicros(uint32_t track)
{
	return exitDeltaMicros[track];
}

unsigned long axleGetLatestAxleTime(uint32_t track)
{
	return currentAxleTime[track];
}

void axleReset(uint32_t track)
{
	axleCount[track] = 0;
	currentAxleTime[track] = 0;
	entranceDeltaMicros[track] = 0;
	exitDeltaMicros[track] = 0;
	axleIsrState[track] = AxleIsrState::IDLE;
}

void axleTerminate(void)
{
	gpio_isr_handler_remove(AXLE_A1);
	gpio_isr_handler_remove(AXLE_A2);
	gpio_isr_handler_remove(AXLE_B1);
	gpio_isr_handler_remove(AXLE_B2);
	gpio_uninstall_isr_service();
}
