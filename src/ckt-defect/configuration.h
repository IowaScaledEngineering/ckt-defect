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

	bool directionEnable;
	uint8_t direction1NameId;
	uint8_t direction2NameId;
	std::string direction1Name;  // Volatile value, not stored in NVM
	std::string direction2Name;  // Volatile value, not stored in NVM
	bool triggerDirection1Only;
	bool triggerDirection2Only;

	// Axles	
	bool axleEnable;
	uint16_t entranceAxles;
	uint16_t minAxles;
	
	// Speed
	bool speedEnable;
	bool speedUnitsMph;        // True = mph, False = kph
	bool speedTypeEnter;       // True = entrance speed, False = exit speed
	uint8_t minSpeed;

	// Other
	uint8_t detectorTimeout;   // Units = seconds

	// Temperature
	bool temperatureEnable;
	bool temperatureReal;
	bool temperatureUnitsF;
	int16_t minTemperature;
	int16_t maxTemperature;
};

void loadConfiguration(DetectorConfiguration* cfg);
void saveConfiguration(DetectorConfiguration* cfg);
void printConfiguration(DetectorConfiguration* cfg);
void resetConfiguration(void);

void updateTrackNames(DetectorConfiguration* cfg);
void updateTrackNames(DetectorConfiguration* cfg, std::string trackA, std::string trackB);
