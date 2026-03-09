/*************************************************************************
Title:    Common Definitions and Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     common.cpp
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

#include <string>
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower

#include "common.h"

void toLowercase(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c){ return std::tolower(c); }
	);
}
