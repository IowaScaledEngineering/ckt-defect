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

#include <Arduino.h>
#include <string>
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower
#include <format>
#include "bootloader_random.h"

#include "common.h"

const std::vector<std::string> trackNames = {
	"Track 1",
	"Track 2",
	"Main 1",
	"Main 2",
	"North Track",
	"South Track",
	"East Track",
	"West Track",
};

const std::vector<std::string> directionNames = {
	"Eastbound",
	"Westbound",
	"Northbound",
	"Southbound",
};

void toLowercase(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c){ return std::tolower(c); }
	);
}

std::string intToString(int32_t intVal, uint32_t integerDigits, uint32_t fractionalDigits)
{
	// Track if the original value was negative
	bool isNegative = (intVal < 0);
	
	// Convert to absolute value for string processing
	uint32_t absVal = isNegative ? static_cast<uint32_t>(-intVal) : static_cast<uint32_t>(intVal);

	// Convert the raw absolute integer to its base string representation
	std::string numStr = std::to_string(absVal);
	std::string formatted;

	// Handle leading zero if fractionalDigits is greater than or equal to the total digits
	if (fractionalDigits >= numStr.length()) {
		size_t leadingZeros = fractionalDigits - numStr.length() + 1; // +1 for the single leading '0' before '.'
		formatted.append(leadingZeros, '0');
		formatted.insert(1, 1, '.'); // Insert decimal after the first '0'
		formatted.append(numStr);
	} 
	// Normal case: Precision fits inside the number length
	else {
		formatted = numStr;
		if (fractionalDigits > 0) {
			// Insert '.' 'fractionalDigits' places from the end
			formatted.insert(formatted.length() - fractionalDigits, 1, '.');
		}
	}

	// Prepend the negative sign if applicable before evaluating integer width
	if (isNegative) {
		formatted.insert(0, 1, '-');
	}

	// Calculate the length of the integer portion currently in the string (including the sign)
	size_t currentIntLength = formatted.find('.');
	if (currentIntLength == std::string::npos) {
		currentIntLength = formatted.length();
	}

	// Pad with spaces to satisfy the integerDigits layout requirement
	if (currentIntLength < integerDigits) {
		formatted.insert(0, integerDigits - currentIntLength, ' ');
	}

	return formatted;
}


uint32_t rollDice(void)
{
	bootloader_random_enable();
	uint32_t raw = esp_random();
	bootloader_random_disable();
	uint32_t dice = raw % (PROBABILITY_MAX);
	Serial.print("Dice: ");
	Serial.println(dice);
	return dice;
}
