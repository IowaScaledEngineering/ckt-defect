/*************************************************************************
Title:    Input/Output Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     io.cpp
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

#define INPUT_TRKA    0x01
#define INPUT_TRKB    0x02

static uint8_t debouncedInputs = 0;

void ioInit(void)
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

	// SDCS: output
	io_conf.pin_bit_mask = (1ULL << SDCS);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

	// SDDET: input with pullup
	io_conf.pin_bit_mask = (1ULL << SDDET);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&io_conf);

	// AXLE_nn_EN: output
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

	io_conf.pin_bit_mask = (1ULL << AXLE_A1_EN);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_A2_EN);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_B1_EN);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_B2_EN);
	gpio_config(&io_conf);

	gpio_set_level(AXLE_A1_EN, 0);
	gpio_set_level(AXLE_A2_EN, 0);
	gpio_set_level(AXLE_B1_EN, 0);
	gpio_set_level(AXLE_B2_EN, 0);

	// AXLE_nn: input
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.intr_type = GPIO_INTR_POSEDGE;

	io_conf.pin_bit_mask = (1ULL << AXLE_A1);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_A2);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_B1);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << AXLE_B2);
	gpio_config(&io_conf);
	
	io_conf.intr_type = GPIO_INTR_DISABLE;  // Reset back to disable

	// TRKn: input with pullup
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

	io_conf.pin_bit_mask = (1ULL << TRKA);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TRKB);
	gpio_config(&io_conf);

	// AUX: output
	io_conf.pin_bit_mask = (1ULL << AUX);
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

	// TPn: output
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

	io_conf.pin_bit_mask = (1ULL << TP0);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TP1);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TP2);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TP3);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TP4);
	gpio_config(&io_conf);
	io_conf.pin_bit_mask = (1ULL << TP5);
	gpio_config(&io_conf);
}

void enableAuxRelay(void)
{
	gpio_set_level(AUX, 1);
}

void disableAuxRelay(void)
{
	gpio_set_level(AUX, 0);
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


void ioProcessInputs(void)
{
	uint8_t inputStatus = 0;
	
	if(gpio_get_level(TRKA))
		inputStatus &= ~INPUT_TRKA;
	else
		inputStatus |= INPUT_TRKA;
	
	if(gpio_get_level(TRKB))
		inputStatus &= ~INPUT_TRKB;
	else
		inputStatus |= INPUT_TRKB;
	
	debouncedInputs = debounce(debouncedInputs, inputStatus);
}


bool isTrackADetected(void)
{
	return debouncedInputs & INPUT_TRKA;
}

bool isTrackBDetected(void)
{
	return debouncedInputs & INPUT_TRKB;
}



void setTestPoint(gpio_num_t tp)
{
	gpio_set_level(tp, 1);
}

void clrTestPoint(gpio_num_t tp)
{
	gpio_set_level(tp, 0);
}
