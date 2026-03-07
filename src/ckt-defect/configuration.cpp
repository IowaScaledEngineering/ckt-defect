/*************************************************************************
Title:    Defect Detector Configuration
Authors:  Michael Petersen <railfan@drgw.net>
File:     configuration.cpp
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

#include <Preferences.h>
#include "configuration.h"

#define PREF_NAMESPACE   "defectdetector"

Preferences preferences;

void loadConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, true);  // Open in read-only mode

	cfg->volumeStep = preferences.getUChar("vol", VOL_STEP_NOM);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		key = "mpEn" + String(i);
		cfg->milepostEnable[i] = preferences.getBool(key.c_str(), true);

		key = "mp" + String(i);
		cfg->milepost[i] = preferences.getFloat(key.c_str(), 123.4);

		key = "trkNameEn" + String(i);
		cfg->trackNameEnable[i] = preferences.getBool(key.c_str(), true);

		key = "trkNameId" + String(i);
		cfg->trackNameId[i] = preferences.getUChar(key.c_str(), 0);

		key = "spdEn" + String(i);
		cfg->speedEnable[i] = preferences.getBool(key.c_str(), true);

		key = "axleEn" + String(i);
		cfg->axleEnable[i] = preferences.getBool(key.c_str(), true);
	}

	preferences.end();
}



void saveConfiguration(DetectorConfiguration* cfg)
{
	String key;

	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode

	if(cfg->volumeStep != preferences.getUChar("vol", VOL_STEP_NOM))
		preferences.putUChar("vol", cfg->volumeStep);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		key = "mpEn" + String(i);
		if(cfg->milepostEnable[i] != preferences.getBool(key.c_str(), true))
			preferences.putBool(key.c_str(), cfg->milepostEnable[i]);

		key = "mp" + String(i);
		if(cfg->milepost[i] != preferences.getFloat(key.c_str(), 123.4))
			preferences.putFloat(key.c_str(), cfg->milepost[i]);

		key = "trkNameEn" + String(i);
		if(cfg->trackNameEnable[i] != preferences.getBool(key.c_str(), true))
			preferences.putBool(key.c_str(), cfg->trackNameEnable[i]);

		key = "trkNameId" + String(i);
		if(cfg->trackNameId[i] != preferences.getUChar(key.c_str(), 0))
			preferences.putUChar(key.c_str(), cfg->trackNameId[i]);

		key = "spdEn" + String(i);
		if(cfg->speedEnable[i] != preferences.getBool(key.c_str(), true))
			preferences.putBool(key.c_str(), cfg->speedEnable[i]);

		key = "axleEn" + String(i);
		if(cfg->axleEnable[i] != preferences.getBool(key.c_str(), true))
			preferences.putBool(key.c_str(), cfg->axleEnable[i]);
	}

	preferences.end();
}



void printConfiguration(DetectorConfiguration* cfg)
{
	Serial.print("Volume: ");
	Serial.println(cfg->volumeStep);

	for(uint32_t i=0; i<NUM_TRACKS; i++)
	{
		Serial.print("Track ");
		Serial.println(i+1);

		Serial.print("Milepost Enable: ");
		Serial.println(cfg->milepostEnable[i]);

		Serial.print("Milepost: ");
		Serial.println(cfg->milepost[i]);

		Serial.print("Track Name Enable: ");
		Serial.println(cfg->trackNameEnable[i]);

		Serial.print("Track Name ID: ");
		if(cfg->trackNameId[i] < trackNames.size())
			Serial.println(trackNames[cfg->trackNameId[i]].c_str());
		else
			Serial.println("ERROR!");

		Serial.print("Speed Enable: ");
		Serial.println(cfg->speedEnable[i]);

		Serial.print("Axle Enable: ");
		Serial.println(cfg->axleEnable[i]);
	}
}



void resetConfiguration(DetectorConfiguration* cfg)
{
	preferences.begin(PREF_NAMESPACE, false);  // Open in read-write mode
	preferences.clear();
	preferences.end();
}
