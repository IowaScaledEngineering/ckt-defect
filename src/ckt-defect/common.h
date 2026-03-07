/*************************************************************************
Title:    Common Definitions and Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     common.h
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

#pragma once

#define NUM_TRACKS     2
#define VOL_STEP_NOM   20

#include <vector>
#include <string>

const std::vector<std::string> trackNames = {
	"Track 1",
	"Track 2",
	"North Track",
	"South Track",
	"East Track",
	"West Track",
};
