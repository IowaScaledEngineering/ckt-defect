/*************************************************************************
Title:    Common Definitions and Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     common.h
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

#define NUM_TRACKS     2

#define VOL_STEP_MAX   30
#define VOL_STEP_NOM   20

#define AUDIO_TASK_PRIORITY   5
#define PARSER_TASK_PRIORITY  4

#define PROBABILITY_MAX       1'000'000
#define MAX_DEFECTS_MAX       10

// Speed Calculation
//
// speedScale   1000000 us/s * 3600 s/hr        us * (mile or km)
// ---------- * ----------------------------- = -----------------
//     10       12 in/ft * (speed units coef)    in * hr
//
// Speed Units Coef:
// 5280 ft/mile
// 3280.84 ft/km
#define SPEED_COEF (1'000'000.0 * 3600.0) / (12.0 * (cfg.speedUnitsMph ? 5280.0 : 3280.84))

#include <vector>
#include <string>
#include <cstdint>

extern const std::vector<std::string> trackNames;
extern const std::vector<std::string> directionNames;
extern const std::vector<std::string> railNames;

void toLowercase(std::string& str);
std::string centerString(const std::string& text, int width = 20);
std::string intToString(int32_t intVal, uint32_t integerDigits, uint32_t fractionalDigits);
std::string getOrdinalWord(const uint8_t& num);
uint32_t rollDice(void);
