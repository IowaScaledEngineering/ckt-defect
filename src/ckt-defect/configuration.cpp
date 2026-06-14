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

#define LCD_BRIGHT_DEFAULT      128

#define MILEPOST_EN_DEFAULT     true
#define TRACK_NAME_EN_DEFAULT   true
#define SPEED_EN_DEFAULT        true
#define AXLE_EN_DEFAULT         true
#define TEMPERATURE_EN_DEFAULT  true
#define MILEPOST_DEFAULT        3469

#define MIN_SPEED_TYPE_DEFAULT  0 // Corresponds to MinSpeed::Off
#define MIN_SPEED_DEFAULT       10
#define MIN_AXLES_DEFAULT       4

#define PREF_NAMESPACE   "defectdetector"

Preferences preferences;

void loadConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, true);  // Open in read-only mode

	cfg->volumeStep = preferences.getUChar("vol", VOL_STEP_NOM);
	cfg->lcdBrightness = preferences.getUChar("lcd", LCD_BRIGHT_DEFAULT);

	cfg->milepostEnable = preferences.getBool("mpEn", MILEPOST_EN_DEFAULT);
	cfg->trackNameEnable = preferences.getBool("trkNameEn", TRACK_NAME_EN_DEFAULT);
	cfg->speedEnable = preferences.getBool("spdEn", SPEED_EN_DEFAULT);
	cfg->axleEnable = preferences.getBool("axleEn", AXLE_EN_DEFAULT);
	cfg->temperatureEnable = preferences.getBool("tmpEn", TEMPERATURE_EN_DEFAULT);

	cfg->minSpeedType = static_cast<MinSpeed>(preferences.getUChar("spdType", MIN_SPEED_TYPE_DEFAULT));
	cfg->minSpeed = preferences.getUChar("minSpd", MIN_SPEED_DEFAULT);
	cfg->minimumAxles = preferences.getUShort("minAxle", MIN_AXLES_DEFAULT);

	cfg->milepost = preferences.getUShort("mp", MILEPOST_DEFAULT);

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

	if(cfg->trackNameEnable != preferences.getBool("trkNameEn", TRACK_NAME_EN_DEFAULT))
		preferences.putBool("trkNameEn", cfg->trackNameEnable);

	if(cfg->speedEnable != preferences.getBool("spdEn", SPEED_EN_DEFAULT))
		preferences.putBool("spdEn", cfg->speedEnable);

	if(cfg->axleEnable != preferences.getBool("axleEn", AXLE_EN_DEFAULT))
		preferences.putBool("axleEn", cfg->axleEnable);

	if(cfg->temperatureEnable != preferences.getBool("tmpEn", TEMPERATURE_EN_DEFAULT))
		preferences.putBool("tmpEn", cfg->temperatureEnable);

	if(static_cast<uint8_t>(cfg->minSpeedType) != preferences.getUChar("spdType", MIN_SPEED_TYPE_DEFAULT))
		preferences.putUChar("spdType", static_cast<uint8_t>(cfg->minSpeedType));

	if(cfg->minSpeed != preferences.getUChar("minSpd", MIN_SPEED_DEFAULT))
		preferences.putUChar("minSpd", cfg->minSpeed);

	if(cfg->minimumAxles != preferences.getUShort("minAxle", MIN_AXLES_DEFAULT))
		preferences.putUShort("minAxle", cfg->minimumAxles);

	if(cfg->milepost != preferences.getUShort("mp", MILEPOST_DEFAULT))
		preferences.putUShort("mp", cfg->milepost);

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

	Serial.print("Speed Enable: ");
	Serial.println(cfg->speedEnable);

	Serial.print("Min Speed Type: ");
	if(static_cast<size_t>(cfg->minSpeedType) < minSpeedName.size())
		Serial.println(minSpeedName[static_cast<size_t>(cfg->minSpeedType)].c_str());
	else
		Serial.println("ERROR!");

	Serial.print("Min Speed: ");
	Serial.println(cfg->minSpeed);

	Serial.print("Axle Enable: ");
	Serial.println(cfg->axleEnable);

	Serial.print("Minimum Axles: ");
	Serial.println(cfg->minimumAxles);

	Serial.print("Temperature Enable: ");
	Serial.println(cfg->temperatureEnable);

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



void resetConfiguration(DetectorConfiguration* cfg)
{
	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode
	preferences.clear();
	preferences.end();
}
