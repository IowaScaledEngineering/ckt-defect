/*************************************************************************
Title:    Configuration File Parser
Authors:  Michael Petersen <railfan@drgw.net>
File:     config-file.h
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

#include <string>
#include <istream>
#include "configuration.h"
#include "messages.h"

class DisplayLcd;

struct ConfigFileResult {
	bool success;
	int errorLine;
	std::string errorTitle;
	std::string errorDetail;

	ConfigFileResult() : success(true), errorLine(0), errorTitle(""), errorDetail("") {}
	ConfigFileResult(int line, const std::string& title, const std::string& detail)
		: success(false), errorLine(line), errorTitle(title), errorDetail(detail) {}
};

struct DefectBuilder {
	bool hasAlertMsg = false;
	bool hasDisplayMsg = false;
	bool hasDetailMsg = false;
	bool hasAxleRate = false;
	int startLine = 0;
	DefectMessage defect;
};

class ConfigFileParser {
public:
	ConfigFileParser(DetectorConfiguration& cfg, MessageBundle& msgs);

	ConfigFileResult processLine(const std::string& rawLine, int lineNumber);
	ConfigFileResult finish(int lastLineNumber);

private:
	DetectorConfiguration& _cfg;
	MessageBundle& _msgs;
	bool _inDefectSection = false;
	bool _bomChecked = false;
	DefectBuilder _currentDefect;

	ConfigFileResult commitCurrentDefect(int lineNumber);
	bool isValidString(const std::string& str) const;
	bool isValidVocab(const std::string& str) const;
};

// Stream & string parsers for unit testing and in-memory usage
ConfigFileResult parseConfigurationStream(std::istream& is, DetectorConfiguration& cfg, MessageBundle& msgs);
ConfigFileResult parseConfigurationString(const std::string& content, DetectorConfiguration& cfg, MessageBundle& msgs);

#if defined(ESP_PLATFORM) || defined(ARDUINO)
// High-level SD card loader and error screen for embedded target
bool loadExternalConfigFile(const char* filepath, DetectorConfiguration& cfg, MessageBundle& msgs, DisplayLcd* lcd);
void freezeOnError(DisplayLcd* lcd, int lineNum, const std::string& errTitle, const std::string& errDetail);
#endif
