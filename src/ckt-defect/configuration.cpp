/*************************************************************************
Title:    Defect Detector Configuration
Authors:  Michael Petersen <railfan@drgw.net>
File:     configuration.cpp
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
#include <Preferences.h>
#include <math.h>

#include "common.h"
#include "configuration.h"

#define LCD_BRIGHT_DEFAULT          128

#define MILEPOST_EN_DEFAULT         true
#define MILEPOST_DEFAULT            3469

#define TRACK_NAME_EN_DEFAULT       true

#define AXLE_EN_DEFAULT             true
#define ENTRANCE_AXLES_DEFAULT      4
#define MIN_AXLES_DEFAULT           8

#define SPEED_EN_DEFAULT            true
#define SPEED_UNITS_MPH_DEFAULT     true
#define SPEED_TYPE_ENTER_DEFAULT    true
#define MIN_SPEED_DEFAULT           10

#define DETECTOR_TIMEOUT_DEFAULT    5

#define TEMPERATURE_EN_DEFAULT       true
#define TEMPERATURE_REAL_DEFAULT     true
#define TEMPERATURE_UNITS_F_DEFAULT  true
#define MIN_TEMPERATURE_DEFAULT      -20
#define MAX_TEMPERATURE_DEFAULT      100

#define DIRECTION_EN_DEFAULT         true
#define TRIGGER_DIR1_ONLY_DEFAULT    false
#define TRIGGER_DIR2_ONLY_DEFAULT    false

#define INFRASTRUCTURE_MODE_DEFAULT  false

#define PREF_NAMESPACE   "defectdetector"

Preferences preferences;

void loadConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, true);  // Open in read-only mode

	cfg->volumeStep = preferences.getUChar("vol", VOL_STEP_NOM);
	cfg->lcdBrightness = preferences.getUChar("lcd", LCD_BRIGHT_DEFAULT);

	cfg->milepostEnable = preferences.getBool("mpEn", MILEPOST_EN_DEFAULT);
	cfg->milepost = preferences.getUShort("mp", MILEPOST_DEFAULT);

	cfg->trackNameEnable = preferences.getBool("trkNameEn", TRACK_NAME_EN_DEFAULT);

	cfg->axleEnable = preferences.getBool("axleEn", AXLE_EN_DEFAULT);
	cfg->entranceAxles = preferences.getUShort("entAxle", ENTRANCE_AXLES_DEFAULT);
	cfg->minAxles = preferences.getUShort("minAxle", MIN_AXLES_DEFAULT);

	cfg->speedEnable = preferences.getBool("spdEn", SPEED_EN_DEFAULT);
	cfg->speedUnitsMph = preferences.getBool("spdUnit", SPEED_UNITS_MPH_DEFAULT);
	cfg->speedTypeEnter = preferences.getBool("spdTyp", SPEED_TYPE_ENTER_DEFAULT);
	cfg->minSpeed = preferences.getUChar("minSpd", MIN_SPEED_DEFAULT);

	cfg->detectorTimeout = preferences.getUChar("to", DETECTOR_TIMEOUT_DEFAULT);

	cfg->temperatureEnable = preferences.getBool("tmpEn", TEMPERATURE_EN_DEFAULT);
	cfg->temperatureReal = preferences.getBool("tmpReal", TEMPERATURE_REAL_DEFAULT);
	cfg->temperatureUnitsF = preferences.getBool("tmpUnitF", TEMPERATURE_UNITS_F_DEFAULT);
	cfg->minTemperatureC = preferences.getFloat("tmpMin", MIN_TEMPERATURE_DEFAULT);
	cfg->maxTemperatureC = preferences.getFloat("tmpMax", MAX_TEMPERATURE_DEFAULT);

	cfg->directionEnable = preferences.getBool("dirEn", DIRECTION_EN_DEFAULT);
	
	uint8_t loadedDir1Id = preferences.getUChar("dir1Id", 0);
	cfg->direction1NameId = (loadedDir1Id >= directionNames.size()) ? 0 : loadedDir1Id;
	
	uint8_t loadedDir2Id = preferences.getUChar("dir2Id", 1); // fallback default to 1 if available
	cfg->direction2NameId = (loadedDir2Id >= directionNames.size()) ? 0 : loadedDir2Id;

	cfg->triggerDirection1Only = preferences.getBool("trigDir1", TRIGGER_DIR1_ONLY_DEFAULT);
	cfg->triggerDirection2Only = preferences.getBool("trigDir2", TRIGGER_DIR2_ONLY_DEFAULT);

	cfg->infrastructureMode = preferences.getBool("infra", INFRASTRUCTURE_MODE_DEFAULT);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		key = "trkNameId" + String(i);
		uint8_t loadedId = preferences.getUChar(key.c_str(), i);
		
		// Bounds check protection against trackNames array size
		if (loadedId >= trackNames.size())
		{
			cfg->trackNameId[i] = 0; // Default fallback to index 0 safely
		}
		else
		{
			cfg->trackNameId[i] = loadedId;
		}
	}

	preferences.end();
}



void saveConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode

	if(cfg->volumeStep != preferences.getUChar("vol", VOL_STEP_NOM))
		preferences.putUChar("vol", cfg->volumeStep);

	if(cfg->lcdBrightness != preferences.getUChar("lcd", LCD_BRIGHT_DEFAULT))
		preferences.putUChar("lcd", cfg->lcdBrightness);

	if(cfg->milepostEnable != preferences.getBool("mpEn", MILEPOST_EN_DEFAULT))
		preferences.putBool("mpEn", cfg->milepostEnable);

	if(cfg->milepost != preferences.getUShort("mp", MILEPOST_DEFAULT))
		preferences.putUShort("mp", cfg->milepost);

	if(cfg->trackNameEnable != preferences.getBool("trkNameEn", TRACK_NAME_EN_DEFAULT))
		preferences.putBool("trkNameEn", cfg->trackNameEnable);

	if(cfg->axleEnable != preferences.getBool("axleEn", AXLE_EN_DEFAULT))
		preferences.putBool("axleEn", cfg->axleEnable);

	if(cfg->entranceAxles != preferences.getUShort("entAxle", ENTRANCE_AXLES_DEFAULT))
		preferences.putUShort("entAxle", cfg->entranceAxles);

	if(cfg->minAxles != preferences.getUShort("minAxle", MIN_AXLES_DEFAULT))
		preferences.putUShort("minAxle", cfg->minAxles);

	if(cfg->speedEnable != preferences.getBool("spdEn", SPEED_EN_DEFAULT))
		preferences.putBool("spdEn", cfg->speedEnable);

	if(cfg->speedUnitsMph != preferences.getBool("spdUnit", SPEED_UNITS_MPH_DEFAULT))
		preferences.putBool("spdUnit", cfg->speedUnitsMph);

	if(cfg->speedTypeEnter != preferences.getBool("spdTyp", SPEED_TYPE_ENTER_DEFAULT))
		preferences.putBool("spdTyp", cfg->speedTypeEnter);

	if(cfg->minSpeed != preferences.getUChar("minSpd", MIN_SPEED_DEFAULT))
		preferences.putUChar("minSpd", cfg->minSpeed);

	if(cfg->detectorTimeout != preferences.getUChar("to", DETECTOR_TIMEOUT_DEFAULT))
		preferences.putUChar("to", cfg->detectorTimeout);

	if(cfg->temperatureEnable != preferences.getBool("tmpEn", TEMPERATURE_EN_DEFAULT))
		preferences.putBool("tmpEn", cfg->temperatureEnable);

	if(cfg->temperatureReal != preferences.getBool("tmpReal", TEMPERATURE_REAL_DEFAULT))
		preferences.putBool("tmpReal", cfg->temperatureReal);

	if(cfg->temperatureUnitsF != preferences.getBool("tmpUnitF", TEMPERATURE_UNITS_F_DEFAULT))
		preferences.putBool("tmpUnitF", cfg->temperatureUnitsF);

	if(cfg->minTemperatureC != preferences.getFloat("tmpMin", MIN_TEMPERATURE_DEFAULT))
		preferences.putFloat("tmpMin", cfg->minTemperatureC);

	if(cfg->maxTemperatureC != preferences.getFloat("tmpMax", MAX_TEMPERATURE_DEFAULT))
		preferences.putFloat("tmpMax", cfg->maxTemperatureC);

	if(cfg->directionEnable != preferences.getBool("dirEn", DIRECTION_EN_DEFAULT))
		preferences.putBool("dirEn", cfg->directionEnable);

	if (cfg->direction1NameId >= directionNames.size()) cfg->direction1NameId = 0;
	if(cfg->direction1NameId != preferences.getUChar("dir1Id", 0))
		preferences.putUChar("dir1Id", cfg->direction1NameId);

	if (cfg->direction2NameId >= directionNames.size()) cfg->direction2NameId = 0;
	if(cfg->direction2NameId != preferences.getUChar("dir2Id", 1))
		preferences.putUChar("dir2Id", cfg->direction2NameId);

	if(cfg->triggerDirection1Only != preferences.getBool("trigDir1", TRIGGER_DIR1_ONLY_DEFAULT))
		preferences.putBool("trigDir1", cfg->triggerDirection1Only);

	if(cfg->triggerDirection2Only != preferences.getBool("trigDir2", TRIGGER_DIR2_ONLY_DEFAULT))
		preferences.putBool("trigDir2", cfg->triggerDirection2Only);

	if(cfg->infrastructureMode != preferences.getBool("infra", INFRASTRUCTURE_MODE_DEFAULT))
		preferences.putBool("infra", cfg->infrastructureMode);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		// Defensive check: Ensure memory configuration isn't corrupted out-of-bounds before saving
		if (cfg->trackNameId[i] >= trackNames.size()) {
			cfg->trackNameId[i] = 0; 
		}

		key = "trkNameId" + String(i);
		if(cfg->trackNameId[i] != preferences.getUChar(key.c_str(), i))
			preferences.putUChar(key.c_str(), cfg->trackNameId[i]);
	}

	preferences.end();
}



void printConfiguration(DetectorConfiguration* cfg)
{
	Serial.print("Volume: ");
	Serial.println(cfg->volumeStep);

	Serial.print("Lcd Brightness: ");
	Serial.println(cfg->lcdBrightness);

	Serial.print("Milepost Enable: ");
	Serial.println(cfg->milepostEnable);

	Serial.print("Milepost: ");
	Serial.println(cfg->milepost);

	Serial.print("Track Name Enable: ");
	Serial.println(cfg->trackNameEnable);

	Serial.print("Axle Enable: ");
	Serial.println(cfg->axleEnable);

	Serial.print("Minimum Axles: ");
	Serial.println(cfg->minAxles);

	Serial.print("Entrance Axles: ");
	Serial.println(cfg->entranceAxles);

	Serial.print("Speed Enable: ");
	Serial.println(cfg->speedEnable);

	Serial.print("Speed Units: ");
	if(cfg->speedUnitsMph)
		Serial.println("mph");
	else
		Serial.println("kph");

	Serial.print("Min Speed Type: ");
	if(cfg->speedTypeEnter)
		Serial.println("Entrance");
	else
		Serial.println("Exit");

	Serial.print("Min Speed: ");
	Serial.println(cfg->minSpeed);

	Serial.print("Detector Timeout: ");
	Serial.println(cfg->detectorTimeout);

	Serial.print("Temperature Enable: ");
	Serial.println(cfg->temperatureEnable);

	Serial.print("Temperature Mode: ");
	if(cfg->temperatureReal)
		Serial.println("Real");
	else
		Serial.println("Simulated");

	Serial.print("Temperature Units: ");
	if(cfg->temperatureUnitsF)
		Serial.println("Fahrenheit (F)");
	else
		Serial.println("Celsius (C)");

	Serial.print("Min Temperature Limit: ");
	Serial.println(cfg->minTemperatureC);

	Serial.print("Max Temperature Limit: ");
	Serial.println(cfg->maxTemperatureC);

	Serial.print("Direction Enable: ");
	Serial.println(cfg->directionEnable);
	Serial.print("   Direction 1 Name ID: ");
	Serial.println(cfg->direction1NameId);
	Serial.print("   Direction 1 Name: ");
	Serial.println(cfg->direction1Name.c_str());
	Serial.print("   Direction 2 Name ID: ");
	Serial.println(cfg->direction2NameId);
	Serial.print("   Direction 2 Name: ");
	Serial.println(cfg->direction2Name.c_str());
	Serial.print("   Trigger Direction 1 Only: ");
	Serial.println(cfg->triggerDirection1Only);
	Serial.print("   Trigger Direction 2 Only: ");
	Serial.println(cfg->triggerDirection2Only);

	Serial.print("Operation Mode: ");
	if(cfg->infrastructureMode)
		Serial.println("Infrastructure");
	else
		Serial.println("Defect Detect");

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		Serial.print('\n');

		Serial.print("Track ");
		Serial.println(i+1);

		Serial.print("   Track Name ID: ");
		Serial.println(cfg->trackNameId[i]);
		
		Serial.print("   Track Name: ");
		Serial.println(cfg->trackName[i].c_str());
	}
}



void resetConfiguration(void)
{
	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode
	preferences.clear();
	preferences.end();
}



void updateTrackNames(DetectorConfiguration* cfg)
{
	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		cfg->trackName[i] = cfg->trackNameEnable ? trackNames[cfg->trackNameId[i]] : "";
	}
}

void updateTrackNames(DetectorConfiguration* cfg, std::string trackA, std::string trackB)
{
	cfg->trackName[0] = trackA;
	cfg->trackName[1] = trackB;
}

void updateDirectionNames(DetectorConfiguration* cfg)
{
	cfg->direction1Name = cfg->directionEnable ? directionNames[cfg->direction1NameId] : "";
	cfg->direction2Name = cfg->directionEnable ? directionNames[cfg->direction2NameId] : "";
}

void updateDirectionNames(DetectorConfiguration* cfg, std::string dir1, std::string dir2)
{
	cfg->direction1Name = dir1;
	cfg->direction2Name = dir2;
}

int16_t getMinTemperature(DetectorConfiguration* cfg)
{
	if (cfg->temperatureUnitsF)
	{
		return static_cast<int16_t>(std::round((cfg->minTemperatureC * 1.8f) + 32.0f));
	}
	return static_cast<int16_t>(std::round(cfg->minTemperatureC));
}

int16_t getMaxTemperature(DetectorConfiguration* cfg)
{
	if (cfg->temperatureUnitsF)
	{
		return static_cast<int16_t>(std::round((cfg->maxTemperatureC * 1.8f) + 32.0f));
	}
	return static_cast<int16_t>(std::round(cfg->maxTemperatureC));
}

void setMinTemperature(DetectorConfiguration* cfg, int16_t temperature)
{
	if (cfg->temperatureUnitsF)
	{
		cfg->minTemperatureC = (static_cast<float>(temperature) - 32.0f) / 1.8f;
	}
	else
	{
		cfg->minTemperatureC = static_cast<float>(temperature);
	}
}

void setMaxTemperature(DetectorConfiguration* cfg, int16_t temperature)
{
	if (cfg->temperatureUnitsF)
	{
		cfg->maxTemperatureC = (static_cast<float>(temperature) - 32.0f) / 1.8f;
	}
	else
	{
		cfg->maxTemperatureC = static_cast<float>(temperature);
	}
}
