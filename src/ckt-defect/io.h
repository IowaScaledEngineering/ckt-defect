/*************************************************************************
Title:    Input/Output Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     io.h
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

#pragma once

// Pins
#define SDA          GPIO_NUM_15
#define SCL          GPIO_NUM_16
#define I2S_SD       GPIO_NUM_7
#define I2S_DOUT     GPIO_NUM_17
#define I2S_BCLK     GPIO_NUM_18
#define I2S_LRCLK    GPIO_NUM_8
#define SDCLK        37
#define SDMOSI       38
#define SDMISO       36
#define SDCS         39
#define SDDET        GPIO_NUM_35

#define AXLE_A1      GPIO_NUM_9
#define AXLE_A1_EN   GPIO_NUM_3
#define AXLE_A2      GPIO_NUM_11
#define AXLE_A2_EN   GPIO_NUM_10
#define AXLE_B1      GPIO_NUM_14
#define AXLE_B1_EN   GPIO_NUM_13
#define AXLE_B2      GPIO_NUM_33
#define AXLE_B2_EN   GPIO_NUM_21
#define TRKA         GPIO_NUM_12
#define TRKB         GPIO_NUM_34

void ioSetup(void);
