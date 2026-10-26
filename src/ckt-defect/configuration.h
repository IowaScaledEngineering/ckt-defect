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
	uint8_t                  volumeStep;
	uint8_t                  noiseStep;
	uint8_t                  pttDelay;                            // Units = deciseconds
	uint8_t                  popcornStep;
	
	uint8_t                  lcdBrightness;

	bool                     milepostEnable;                      // Enable for built-in messages
	bool                     milepostDisplay;                     // Enable for homescreen display
	uint16_t                 milepost;                            // Units = decimiles

	bool                     trackNameEnable;                     // Enable for built-in messages
	uint8_t                  trackNameId[NUM_TRACKS];
	std::string              trackName[NUM_TRACKS];               // Volatile value, not stored in NVM

	bool                     directionEnable;                     // Enable for built-in messages
	uint8_t                  direction1NameId;
	uint8_t                  direction2NameId;
	std::string              direction1Name;                      // Volatile value, not stored in NVM
	std::string              direction2Name;                      // Volatile value, not stored in NVM
	bool                     triggerDirection1Only;
	bool                     triggerDirection2Only;

	bool                     railNameEnable;                      // Enable for built-in messages
	uint8_t                  railNameId;
	std::string              railName;                            // Volatile value, not stored in NVM

	// Axles	
	bool                     axleEnable;                          // Enable for built-in messages
	uint16_t                 entranceAxles;
	uint16_t                 minAxles;
	
	// Speed
	bool                     speedEnable;                         // Enable for built-in messages
	bool                     speedUnitsMph;                       // True = mph, False = kph
	bool                     speedTypeEnter;                      // True = entrance speed, False = exit speed
	uint8_t                  minSpeed;
	uint16_t                 speedScale;                          // Units = deciscale

	// Other
	uint8_t                  detectorTimeout;                     // Units = seconds
	uint8_t                  summaryDisplayTime;                  // Units = seconds
	uint8_t                  msgRepeatTimeout;                    // Units = seconds

	// Temperature
	bool                     temperatureEnable;                   // Enable for built-in messages
	bool                     temperatureDisplay;                  // Enable for homescreen display
	bool                     temperatureReal;
	bool                     temperatureUnitsF;
	float                    minTemperatureC;                     // Float so conversion to/from F works cleanly
	float                    maxTemperatureC;

	// Built-In Defects
	bool                     defectHotJournalEnable;
	uint32_t                 defectHotJournalAxleRate;
	bool                     defectHotWheelEnable;
	uint32_t                 defectHotWheelAxleRate;
	bool                     defectHighImpactWheelEnable;
	uint32_t                 defectHighImpactWheelAxleRate;
	bool                     defectDraggingEquipmentEnable;
	uint32_t                 defectDraggingEquipmentAxleRate;
	bool                     defectHighLoadEnable;
	uint32_t                 defectHighLoadAxleRate;
	bool                     defectWideLoadEnable;
	uint32_t                 defectWideLoadAxleRate;

	// Random Integrity Error
	bool                     defectIntegrityEnable;
	uint32_t                 defectIntegrityTrainRate;

	// Messages
	bool                     entranceMessageEnable;
	bool                     alertMessageEnable;

	// Exit Config
	bool                     talkOnDefectOnly;
	uint8_t                  ordinalTypeId;
	uint8_t                  maxDefects;

	// Operation Mode
	bool                     infrastructureMode;
	
	// Vocab
	bool                     externalVocabPresent;                // Volatile value, not stored in NVM
	std::string              vocabSelected;
	std::vector<std::string> vocabsAvailable;                     // Loaded at runtime, not stored in NVM
	
	// Misc. Stuff
	uint8_t                  minAxleTime;                         // 100us increments
};

void loadConfiguration(DetectorConfiguration* cfg);
void saveConfiguration(DetectorConfiguration* cfg);
void printConfiguration(DetectorConfiguration* cfg);
void resetConfiguration(void);

void updateTrackNames(DetectorConfiguration* cfg);
void updateTrackNames(DetectorConfiguration* cfg, std::string trackA, std::string trackB);

void updateDirectionNames(DetectorConfiguration* cfg);
void updateDirectionNames(DetectorConfiguration* cfg, std::string dir1, std::string dir2);

void updateRailNames(DetectorConfiguration* cfg);
void updateRailNames(DetectorConfiguration* cfg, std::string rail);

int16_t getMinTemperature(DetectorConfiguration* cfg);
int16_t getMaxTemperature(DetectorConfiguration* cfg);
void setMinTemperature(DetectorConfiguration* cfg, int16_t temperature);
void setMaxTemperature(DetectorConfiguration* cfg, int16_t temperature);