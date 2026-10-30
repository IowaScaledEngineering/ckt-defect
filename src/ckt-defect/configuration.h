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

#define CONFIG_SCHEMA_VERSION         1

// ============================================================================
// CONFIGURATION PARAMETER LIMITS (Bounds for UI dials & packing verification)
// ============================================================================
#define ENTRANCE_AXLES_MIN            0
#define ENTRANCE_AXLES_MAX            10

#define MIN_AXLES_MIN                 0
#define MIN_AXLES_MAX                 100

#define MIN_SPEED_MIN                 0
#define MIN_SPEED_MAX                 50

#define SPEED_SCALE_MIN               0
#define SPEED_SCALE_MAX               999   // deciscale (0.0 to 99.9)

#define DETECTOR_TIMEOUT_MIN          2
#define DETECTOR_TIMEOUT_MAX          30

#define SUMMARY_DISP_TIME_MIN         2
#define SUMMARY_DISP_TIME_MAX         60

#define MSG_REPEAT_TIMEOUT_MIN        2
#define MSG_REPEAT_TIMEOUT_MAX        60

#define TEMPERATURE_MIN               -99
#define TEMPERATURE_MAX               150

#define MAX_DEFECTS_MIN               1
#define MAX_DEFECTS_MAX               9

#define PTT_DELAY_MIN                 0
#define PTT_DELAY_MAX                 3000  // milliseconds (stored as 0..30 deciseconds)
#define PTT_DELAY_STEP_MS             100

#define MIN_AXLE_TIME_MIN             0
#define MIN_AXLE_TIME_MAX             25000 // microseconds (stored as 0..250 in 100us units)
#define MIN_AXLE_TIME_STEP_US         100

#define MILEPOST_MIN                  0
#define MILEPOST_MAX                  9999  // decimiles (0.0 to 999.9)

// ============================================================================
// CONFIG CODEC ANNOTATIONS & PACKING SPECIFICATION (Proposal C)
// ============================================================================
// Trailing inline comments on struct members define bitstream serialization
// into the compact alphanumeric code displayed on the LCD.
//
// Supported Annotations:
//   • bool                 : Implicit 1-bit allocation (no annotation needed).
//   • @bits: <1..32>       : Dense bit-packing for bounded unsigned integers.
//                            Enforces: static_assert(FIELD_MAX < (1 << BITS)).
//   • @type: varint        : LEB128 variable-length integer (7 payload bits/byte).
//                            Best for open-ended or wide dynamic range (defect rates).
//   • @offset: <N>         : Linear bias offset added before packing (e.g. for signed).
//   • @string: <N>         : String identifier / flag packed in N bits (0 = empty).
//
// Decision Matrix: When to use Varint vs. Fixed Bits (@bits: N):
//   +-------------------------------+----------------------------------------+
//   | Variable Characteristic       | Recommended Encoding & Rationale       |
//   +-------------------------------+----------------------------------------+
//   | Boolean flag                  | Implicit 1 bit (saves 7 bits vs varint)|
//   | UI Dial / Enum (MAX <= 255)   | @bits: N (Dense packing, saves bits)   |
//   | Defect trigger rates (0-1M+)  | @type: varint (1 byte if small or 0,   |
//   |                               |                scales up transparently)|
//   | String selection / filename   | @string: N (Packs presence/ID in N b)  |
//   +-------------------------------+----------------------------------------+
//
// Rule of Thumb:
//   Is the variable a bool?
//    ├── YES ──> No annotation (implicit 1 bit)
//    └── NO  ──> Does it have a fixed UI limit (e.g. MAX <= 255)?
//                 ├── YES ──> Use `@bits: N` where N = ceil(log2(MAX + 1))
//                 └── NO  ──> Use `@type: varint` (open-ended 0 to millions)
//
// TOTAL BIT BUDGET:
//   Combined bit sum MUST NOT exceed 300 bits (60 Base32 characters) to fit
//   across 3 lines of the 20x4 LCD display.
// ============================================================================
struct DetectorConfiguration {
	// Audio & Display
	uint8_t                  volumeStep;                          // @bits: 5
	uint8_t                  noiseStep;                           // @bits: 4
	uint8_t                  pttDelay;                            // @bits: 5 (Units = deciseconds)
	uint8_t                  popcornStep;                         // @bits: 4
	uint8_t                  lcdBrightness;                       // @bits: 8

	// Milepost & Tracks
	bool                     milepostEnable;                      // Enable for built-in messages
	bool                     milepostDisplay;                     // Enable for homescreen display
	uint16_t                 milepost;                            // @bits: 14 (Units = decimiles)
	bool                     trackNameEnable;                     // Enable for built-in messages
	uint8_t                  trackNameId[NUM_TRACKS];             // @bits: 3
	std::string              trackName[NUM_TRACKS];               // Volatile value, not stored in NVM

	// Direction & Rail
	bool                     directionEnable;                     // Enable for built-in messages
	uint8_t                  direction1NameId;                    // @bits: 3
	uint8_t                  direction2NameId;                    // @bits: 3
	std::string              direction1Name;                      // Volatile value, not stored in NVM
	std::string              direction2Name;                      // Volatile value, not stored in NVM
	bool                     triggerDirection1Only;
	bool                     triggerDirection2Only;
	bool                     railNameEnable;                      // Enable for built-in messages
	uint8_t                  railNameId;                          // @bits: 2
	std::string              railName;                            // Volatile value, not stored in NVM

	// Axles	
	bool                     axleEnable;                          // Enable for built-in messages
	uint16_t                 entranceAxles;                       // @bits: 4
	uint16_t                 minAxles;                            // @bits: 7
	
	// Speed
	bool                     speedEnable;                         // Enable for built-in messages
	bool                     speedUnitsMph;                       // True = mph, False = kph
	bool                     speedTypeEnter;                      // True = entrance speed, False = exit speed
	uint8_t                  minSpeed;                            // @bits: 6
	uint16_t                 speedScale;                          // @bits: 10 (Units = deciscale)

	// Other
	uint8_t                  detectorTimeout;                     // @bits: 5 (Units = seconds)
	uint8_t                  summaryDisplayTime;                  // @bits: 6 (Units = seconds)
	uint8_t                  msgRepeatTimeout;                    // @bits: 6 (Units = seconds)

	// Temperature
	bool                     temperatureEnable;                   // Enable for built-in messages
	bool                     temperatureDisplay;                  // Enable for homescreen display
	bool                     temperatureReal;
	bool                     temperatureUnitsF;
	float                    minTemperatureC;                     // @bits: 8, @offset: 100
	float                    maxTemperatureC;                     // @bits: 8, @offset: 100

	// Built-In Defects
	bool                     defectHotJournalEnable;
	uint32_t                 defectHotJournalAxleRate;            // @type: varint
	bool                     defectHotWheelEnable;
	uint32_t                 defectHotWheelAxleRate;              // @type: varint
	bool                     defectHighImpactWheelEnable;
	uint32_t                 defectHighImpactWheelAxleRate;       // @type: varint
	bool                     defectDraggingEquipmentEnable;
	uint32_t                 defectDraggingEquipmentAxleRate;     // @type: varint
	bool                     defectHighLoadEnable;
	uint32_t                 defectHighLoadAxleRate;              // @type: varint
	bool                     defectWideLoadEnable;
	uint32_t                 defectWideLoadAxleRate;              // @type: varint

	// Random Integrity Error
	bool                     defectIntegrityEnable;
	uint32_t                 defectIntegrityTrainRate;            // @type: varint

	// Messages
	bool                     entranceMessageEnable;
	bool                     alertMessageEnable;

	// Exit Config
	bool                     talkOnDefectOnly;
	uint8_t                  ordinalTypeId;                       // @bits: 3
	uint8_t                  maxDefects;                          // @bits: 4

	// Operation Mode
	bool                     infrastructureMode;
	
	// Vocab
	bool                     externalVocabPresent;                // Volatile value, not stored in NVM
	std::string              vocabSelected;                       // @string: 8
	std::vector<std::string> vocabsAvailable;                     // Loaded at runtime, not stored in NVM

	// Config Tracking
	bool                     externalConfigPresent;               // Volatile value, not stored in NVM
	
	// Misc. Stuff
	uint8_t                  minAxleTime;                         // @bits: 8 (100us increments)
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