/*************************************************************************
Title:    Defect Detector Configuration
Authors:  Michael Petersen <railfan@drgw.net>
File:     configuration.h
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2026 Michael Petersen

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

#include <math.h>
#include "common.h"

struct DetectorConfiguration {
	uint8_t volumeStep;

	bool milepostEnable[NUM_TRACKS];
	float_t milepost[NUM_TRACKS];

	bool trackNameEnable[NUM_TRACKS];
	uint8_t trackNameId[NUM_TRACKS];

	bool speedEnable[NUM_TRACKS];

	bool axleEnable[NUM_TRACKS];
};

void loadConfiguration(DetectorConfiguration* cfg);
void saveConfiguration(DetectorConfiguration* cfg);
void printConfiguration(DetectorConfiguration* cfg);
void resetConfiguration(DetectorConfiguration* cfg);
