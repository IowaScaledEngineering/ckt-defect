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

#define MILEPOST_EN_DEFAULT     true
#define TRACK_NAME_EN_DEFAULT   true
#define SPEED_EN_DEFAULT        true
#define AXLE_EN_DEFAULT         true
#define TEMPERATURE_EN_DEFAULT  true
#define MILEPOST_DEFAULT        346.9

#define PREF_NAMESPACE   "defectdetector"

Preferences preferences;

void loadConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, true);  // Open in read-only mode

	cfg->volumeStep = preferences.getUChar("vol", VOL_STEP_NOM);

	cfg->milepostEnable = preferences.getBool("mpEn", MILEPOST_EN_DEFAULT);
	cfg->trackNameEnable = preferences.getBool("trkNameEn", TRACK_NAME_EN_DEFAULT);
	cfg->speedEnable = preferences.getBool("spdEn", SPEED_EN_DEFAULT);
	cfg->axleEnable = preferences.getBool("axleEn", AXLE_EN_DEFAULT);
	cfg->temperatureEnable = preferences.getBool("tmpEn", TEMPERATURE_EN_DEFAULT);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		key = "mp" + String(i);
		cfg->milepost[i] = preferences.getFloat(key.c_str(), MILEPOST_DEFAULT);

		key = "trkNameId" + String(i);
		cfg->trackNameId[i] = preferences.getUChar(key.c_str(), i);
	}

	preferences.end();
}



void saveConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode

	if(cfg->volumeStep != preferences.getUChar("vol", VOL_STEP_NOM))
		preferences.putUChar("vol", cfg->volumeStep);

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

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		key = "mp" + String(i);
		if(cfg->milepost[i] != preferences.getFloat(key.c_str(), MILEPOST_DEFAULT))
			preferences.putFloat(key.c_str(), cfg->milepost[i]);

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

	Serial.print("Milepost Enable: ");
	Serial.println(cfg->milepostEnable);

	Serial.print("Track Name Enable: ");
	Serial.println(cfg->trackNameEnable);

	Serial.print("Speed Enable: ");
	Serial.println(cfg->speedEnable);

	Serial.print("Axle Enable: ");
	Serial.println(cfg->axleEnable);

	Serial.print("Temperature Enable: ");
	Serial.println(cfg->temperatureEnable);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		Serial.print('\n');

		Serial.print("Track ");
		Serial.println(i+1);

		Serial.print("   Milepost: ");
		Serial.println(cfg->milepost[i]);

		Serial.print("   Track Name ID: ");
		if(cfg->trackNameId[i] < trackNames.size())
			Serial.println(trackNames[cfg->trackNameId[i]].c_str());
		else
			Serial.println("ERROR!");
	}
}



void resetConfiguration(DetectorConfiguration* cfg)
{
	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode
	preferences.clear();
	preferences.end();
}
