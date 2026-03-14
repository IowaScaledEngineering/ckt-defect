/*************************************************************************
Title:    Defect Message Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     messages.h
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

#include <vector>
#include <string>

#include "configuration.h"
#include "data.h"

struct DefectMessage {
	std::string alertMsg;    // Played instantly when defect detected (e.g. words, silence, or tone)
	std::string detailMsg;   // Played in exit message after defect(s) detected 
	std::string summaryMsg;  // Displayed on screen when defect detected (20 character summary)
	uint16_t probability;    // Probability out of every 10,000 axles
};

struct MessageBundle {
	std::string entranceMsg;                       // Triggered when train first detected
	std::vector<DefectMessage> defects;
	std::string exitCleanMsg;                      // Plays after train passes and no defect detected
	std::string exitDefectMsg;                     // Plays after train passes and defect(s) detected
	std::string integrityMsg;                      // Simulates a detector failure
	uint16_t integrityProbability;                 // Probability out of every 10,000 trains
	std::string tooSlowMsg;                        // If speed enabled, plays when speed is below programmed threshold
	std::string detectorBlockedMsg;                // if axle stops in front of detector for longer than timeout
};

void printMessages(MessageBundle* msgs);
std::string* transformMessage(std::string* inputMessage, DetectorConfiguration *cfg, DataBundle *data, uint8_t trackNum);
