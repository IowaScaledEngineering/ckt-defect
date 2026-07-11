/*************************************************************************
Title:    Defect Detector Live Data
Authors:  Michael Petersen <railfan@drgw.net>
File:     data.h
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

struct DataBundle {
	std::vector<std::string> defects;
	uint16_t axleCount = 0;
	uint16_t totalAxles = 0;
	uint16_t speed = 0;
	float speedFloat = 0;

	//  Inter-state machine communication
	bool irDetect = false;
	bool axleDetect = false;
	bool newAxle = false;

	//  Loaded by main loop, consumed by state machine(s)
	uint16_t axleCountLive = 0;
	bool irInput = false;
	bool axleInput1 = false;
	bool axleInput2 = false;

	bool active = false;
};
