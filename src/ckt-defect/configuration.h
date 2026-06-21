/*************************************************************************
Title:    Defect Detector Configuration
Authors:  Michael Petersen <railfan@drgw.net>
File:     configuration.h
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

#include "common.h"

struct DetectorConfiguration {
	uint8_t volumeStep;
	uint8_t lcdBrightness;

	bool milepostEnable;
	uint16_t milepost;

	bool trackNameEnable;
	uint8_t trackNameId[NUM_TRACKS];
	std::string trackName[NUM_TRACKS];  // Volatile value, not stored in NVM

	// Axles	
	bool axleEnable;
	uint16_t entranceAxles;
	uint16_t minAxles;
	
	// Speed
	bool speedEnable;
	bool speedUnitsMph;      // True = mph, False = kph
	bool speedTypeEnter;     // True = entrance speed, False = exit speed
	uint8_t minSpeed;

	// Other
	bool temperatureEnable;
};

void loadConfiguration(DetectorConfiguration* cfg);
void saveConfiguration(DetectorConfiguration* cfg);
void printConfiguration(DetectorConfiguration* cfg);
void resetConfiguration(DetectorConfiguration* cfg);
