/*************************************************************************
Title:    Configuration File Parser
Authors:  Michael Petersen <railfan@drgw.net>
File:     config-file.cpp
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

#include "config-file.h"
#include <sstream>
#include <algorithm>
#include <cctype>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include <Arduino.h>
#include "ff.h"
#include "esp_task_wdt.h"
#include "display-lcd.h"
#include "common.h"
#endif

namespace {

std::string trim(const std::string& str)
{
	size_t start = 0;
	while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start])))
		start++;
	if (start == str.length())
		return "";
	size_t end = str.length() - 1;
	while (end > start && std::isspace(static_cast<unsigned char>(str[end])))
		end--;
	return str.substr(start, end - start + 1);
}

std::string toLower(const std::string& str)
{
	std::string res = str;
	for (char& c : res)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return res;
}

bool isNumeric(const std::string& str)
{
	if (str.empty())
		return false;
	size_t i = 0;
	if (str[0] == '+' || str[0] == '-')
		i = 1;
	if (i >= str.length())
		return false;
	for (; i < str.length(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(str[i])))
			return false;
	}
	return true;
}

bool parseConfigInt(const std::string& str, int32_t minVal, int32_t maxVal, int32_t& outVal)
{
	try {
		long long val = std::stoll(str);
		if (val < minVal || val > maxVal)
			return false;
		outVal = static_cast<int32_t>(val);
		return true;
	} catch (...) {
		return false;
	}
}

} // namespace

ConfigFileParser::ConfigFileParser(DetectorConfiguration& cfg, MessageBundle& msgs)
	: _cfg(cfg), _msgs(msgs)
{
	_cfg.externalConfigPresent = false;
	_cfg.externalMessagesPresent = false;
	for (uint32_t i = 0; i < NUM_TRACKS; i++)
		_cfg.externalTrackNamePresent[i] = false;
	_cfg.externalDirection1NamePresent = false;
	_cfg.externalDirection2NamePresent = false;
	_cfg.externalRailNamePresent = false;
	_cfg.externalVocabSelectedPresent = false;
	_cfg.externalIntegrityTrainRatePresent = false;
}

bool ConfigFileParser::isValidString(const std::string& str) const
{
	// Allowed characters: [a-zA-Z0-9 _\-:\.\/\\=,#\?!]
	for (char c : str)
	{
		if (std::isalnum(static_cast<unsigned char>(c)))
			continue;
		switch (c)
		{
			case ' ':
			case '_':
			case '-':
			case ':':
			case '.':
			case '/':
			case '\\':
			case '=':
			case ',':
			case '#':
			case '?':
			case '!':
				continue;
			default:
				return false;
		}
	}
	return true;
}

bool ConfigFileParser::isValidVocab(const std::string& str) const
{
	if (str.empty() || str.length() > 31)
		return false;
	for (char c : str)
	{
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
			return false;
	}
	return true;
}

ConfigFileResult ConfigFileParser::commitCurrentDefect(int lineNumber)
{
	if (!_inDefectSection)
		return ConfigFileResult();

	int errLine = (_currentDefect.startLine > 0) ? _currentDefect.startLine : lineNumber;
	if (!_currentDefect.hasAlertMsg)
		return ConfigFileResult(errLine, "Incomplete Defect", "Missing alertMsg");
	if (!_currentDefect.hasDisplayMsg)
		return ConfigFileResult(errLine, "Incomplete Defect", "Missing displayMsg");
	if (!_currentDefect.hasDetailMsg)
		return ConfigFileResult(errLine, "Incomplete Defect", "Missing detailMsg");
	if (!_currentDefect.hasAxleRate)
		return ConfigFileResult(errLine, "Incomplete Defect", "Missing axleRate");

	_msgs.defects.push_back(_currentDefect.defect);
	_cfg.externalMessagesPresent = true;
	_currentDefect = DefectBuilder();
	_inDefectSection = false;
	return ConfigFileResult();
}

ConfigFileResult ConfigFileParser::processLine(const std::string& rawLine, int lineNumber)
{
	std::string line = trim(rawLine);
	if (line.empty() || line[0] == '#' || line[0] == ';')
		return ConfigFileResult();

	// Section header: [section]
	if (line[0] == '[')
	{
		size_t closePos = line.find(']');
		if (closePos == std::string::npos)
			return ConfigFileResult(lineNumber, "Syntax Error", "Missing ']' in section");

		std::string trailing = trim(line.substr(closePos + 1));
		if (!trailing.empty() && trailing[0] != ';')
			return ConfigFileResult(lineNumber, "Syntax Error", "Text after ']'");

		std::string sectionName = toLower(trim(line.substr(1, closePos - 1)));
		if (sectionName == "defect")
		{
			if (_inDefectSection)
			{
				ConfigFileResult commitRes = commitCurrentDefect(lineNumber);
				if (!commitRes.success)
					return commitRes;
			}
			_inDefectSection = true;
			_currentDefect = DefectBuilder();
			_currentDefect.startLine = lineNumber;
			return ConfigFileResult();
		}
		else if (sectionName == "general" || sectionName == "global" || sectionName == "config")
		{
			if (_inDefectSection)
			{
				ConfigFileResult commitRes = commitCurrentDefect(lineNumber);
				if (!commitRes.success)
					return commitRes;
			}
			_inDefectSection = false;
			return ConfigFileResult();
		}
		else
		{
			return ConfigFileResult(lineNumber, "Unknown Section", "[" + sectionName + "]");
		}
	}

	// Inline comment: strip from ';' to end of line
	size_t semiPos = line.find(';');
	if (semiPos != std::string::npos)
	{
		line = trim(line.substr(0, semiPos));
		if (line.empty())
			return ConfigFileResult();
	}

	// Split key = value
	size_t eqPos = line.find('=');
	if (eqPos == std::string::npos)
		return ConfigFileResult(lineNumber, "Syntax Error", "Missing '=' in line");

	std::string rawKey = trim(line.substr(0, eqPos));
	std::string value = trim(line.substr(eqPos + 1));
	if (rawKey.empty())
		return ConfigFileResult(lineNumber, "Syntax Error", "Missing key before '='");

	std::string key = toLower(rawKey);

	if (_inDefectSection)
	{
		if (key == "alertmsg")
		{
			if (_currentDefect.hasAlertMsg)
				return ConfigFileResult(lineNumber, "Duplicate Key", "alertMsg");
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In alertMsg");
			_currentDefect.defect.alertMsg = value;
			_currentDefect.hasAlertMsg = true;
			return ConfigFileResult();
		}
		else if (key == "displaymsg")
		{
			if (_currentDefect.hasDisplayMsg)
				return ConfigFileResult(lineNumber, "Duplicate Key", "displayMsg");
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In displayMsg");
			_currentDefect.defect.displayMsg = value;
			_currentDefect.hasDisplayMsg = true;
			return ConfigFileResult();
		}
		else if (key == "detailmsg")
		{
			if (_currentDefect.hasDetailMsg)
				return ConfigFileResult(lineNumber, "Duplicate Key", "detailMsg");
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In detailMsg");
			_currentDefect.defect.detailMsg = value;
			_currentDefect.hasDetailMsg = true;
			return ConfigFileResult();
		}
		else if (key == "axlerate")
		{
			if (_currentDefect.hasAxleRate)
				return ConfigFileResult(lineNumber, "Duplicate Key", "axleRate");
			if (!isNumeric(value))
				return ConfigFileResult(lineNumber, "Invalid Integer", value);
			int32_t rateVal = 0;
			if (!parseConfigInt(value, 1, 999999, rateVal))
				return ConfigFileResult(lineNumber, "Out of Range", value + " (1-999999)");
			_currentDefect.defect.axleRate = static_cast<uint32_t>(rateVal);
			_currentDefect.hasAxleRate = true;
			return ConfigFileResult();
		}
		else
		{
			return ConfigFileResult(lineNumber, "Unknown Defect Key", rawKey);
		}
	}
	else
	{
		// Global Scope: Defect keys require [defect] section
		if (key == "alertmsg" || key == "displaymsg" || key == "detailmsg" || key == "axlerate")
		{
			return ConfigFileResult(lineNumber, "Invalid Context", rawKey + " requires [defect]");
		}

		// Global Hardware & Configuration keys
		if (key == "trackname1" || key == "tracknamea")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			if (value.length() > 16)
				return ConfigFileResult(lineNumber, "String Too Long", rawKey + " > 16 chars");
			_cfg.trackName[0] = value;
			_cfg.externalTrackNamePresent[0] = true;
			return ConfigFileResult();
		}
		else if (key == "trackname2" || key == "tracknameb")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			if (value.length() > 16)
				return ConfigFileResult(lineNumber, "String Too Long", rawKey + " > 16 chars");
			_cfg.trackName[1] = value;
			_cfg.externalTrackNamePresent[1] = true;
			return ConfigFileResult();
		}
		else if (key == "direction1name")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			if (value.length() > 16)
				return ConfigFileResult(lineNumber, "String Too Long", rawKey + " > 16 chars");
			_cfg.direction1Name = value;
			_cfg.externalDirection1NamePresent = true;
			return ConfigFileResult();
		}
		else if (key == "direction2name")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			if (value.length() > 16)
				return ConfigFileResult(lineNumber, "String Too Long", rawKey + " > 16 chars");
			_cfg.direction2Name = value;
			_cfg.externalDirection2NamePresent = true;
			return ConfigFileResult();
		}
		else if (key == "railname")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_cfg.railName = value;
			_cfg.externalRailNamePresent = true;
			return ConfigFileResult();
		}
		else if (key == "vocabselected")
		{
			if (!isValidVocab(value))
				return ConfigFileResult(lineNumber, "Invalid Vocab Name", value);
			_cfg.vocabSelected = value;
			_cfg.externalVocabSelectedPresent = true;
			return ConfigFileResult();
		}
		else if (key == "integritytrainrate")
		{
			if (!isNumeric(value))
				return ConfigFileResult(lineNumber, "Invalid Integer", value);
			int32_t trainRate = 0;
			if (!parseConfigInt(value, 1, 999999, trainRate))
				return ConfigFileResult(lineNumber, "Out of Range", value + " (1-999999)");
			_msgs.integrityTrainRate = static_cast<uint32_t>(trainRate);
			_cfg.externalIntegrityTrainRatePresent = true;
			return ConfigFileResult();
		}
		// Global Message Keys
		else if (key == "entrancemsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.entranceMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "exitcleanmsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.exitCleanMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "exitcleandisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.exitCleanDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "exitdefectmsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.exitDefectMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "exitdefectdisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.exitDefectDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "integritymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.integrityMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "integritydisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.integrityDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "minaxlemsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.minAxleMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "minaxledisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.minAxleDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "tooslowmsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.tooSlowMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "tooslowdisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.tooSlowDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "detectorblockedmsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.detectorBlockedMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "detectorblockeddisplaymsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.detectorBlockedDisplayMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else if (key == "excessalarmsmsg")
		{
			if (!isValidString(value))
				return ConfigFileResult(lineNumber, "Illegal Character", "In " + rawKey);
			_msgs.excessAlarmsMsg = value;
			_cfg.externalMessagesPresent = true;
			return ConfigFileResult();
		}
		else
		{
			return ConfigFileResult(lineNumber, "Unknown Key", rawKey);
		}
	}
}

ConfigFileResult ConfigFileParser::finish(int lastLineNumber)
{
	if (_inDefectSection)
	{
		ConfigFileResult commitRes = commitCurrentDefect(lastLineNumber);
		if (!commitRes.success)
			return commitRes;
	}
	return ConfigFileResult();
}

ConfigFileResult parseConfigurationStream(std::istream& is, DetectorConfiguration& cfg, MessageBundle& msgs)
{
	ConfigFileParser parser(cfg, msgs);
	std::string line;
	int lineNum = 0;
	while (std::getline(is, line))
	{
		lineNum++;
		ConfigFileResult res = parser.processLine(line, lineNum);
		if (!res.success)
			return res;
	}
	return parser.finish(lineNum);
}

ConfigFileResult parseConfigurationString(const std::string& content, DetectorConfiguration& cfg, MessageBundle& msgs)
{
	std::istringstream iss(content);
	return parseConfigurationStream(iss, cfg, msgs);
}

#if defined(ESP_PLATFORM) || defined(ARDUINO)
static bool readFatFsLine(FIL *fp, std::string& outLine)
{
	outLine.clear();
	UINT bytesRead = 0;
	char c;
	bool hasContent = false;

	while (f_read(fp, &c, 1, &bytesRead) == FR_OK && bytesRead > 0)
	{
		hasContent = true;
		if (c == '\r')
			continue;
		if (c == '\n')
			break;
		outLine.push_back(c);
	}

	return hasContent;
}

void freezeOnError(DisplayLcd* lcd, int lineNum, const std::string& errTitle, const std::string& errDetail)
{
	Serial.printf("Config Error at line %d: %s %s\n", lineNum, errTitle.c_str(), errDetail.c_str());

	if (lcd)
	{
		lcd->clear();
		lcd->gotoxy(0, 0);
		std::string r0 = "Config Error: Line " + std::to_string(lineNum);
		lcd->print(r0.substr(0, 20));

		lcd->gotoxy(0, 1);
		std::string r1 = errTitle.substr(0, 20);
		lcd->print(r1);

		lcd->gotoxy(0, 2);
		std::string r2 = errDetail.substr(0, 20);
		lcd->print(r2);

		lcd->gotoxy(14, 3);
		lcd->print("REBOOT");
	}

	while (true)
	{
		esp_task_wdt_reset();
		if (lcd)
		{
			lcd->readKeys();
			DisplayEvent ev;
			if (lcd->getEvent(&ev) && ev.type == DisplayEventType::KEY_PRESS)
			{
				if (ev.keyNum == 4) // Button 4: REBOOT
				{
					restartWithDelay(500, lcd);
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

bool loadExternalConfigFile(const char* filepath, DetectorConfiguration& cfg, MessageBundle& msgs, DisplayLcd* lcd)
{
	FIL fp;
	FRESULT fr = f_open(&fp, filepath, FA_READ);
	if (fr != FR_OK && filepath[0] != '0')
	{
		std::string p = std::string("0:") + filepath;
		fr = f_open(&fp, p.c_str(), FA_READ);
	}

	if (fr != FR_OK)
	{
		// File not present on SD card; normal condition
		return false;
	}

	ConfigFileParser parser(cfg, msgs);
	std::string line;
	int lineNum = 0;

	while (readFatFsLine(&fp, line))
	{
		lineNum++;
		esp_task_wdt_reset();
		ConfigFileResult res = parser.processLine(line, lineNum);
		if (!res.success)
		{
			f_close(&fp);
			freezeOnError(lcd, res.errorLine, res.errorTitle, res.errorDetail);
			return false;
		}
	}

	ConfigFileResult finishRes = parser.finish(lineNum);
	if (!finishRes.success)
	{
		f_close(&fp);
		freezeOnError(lcd, finishRes.errorLine, finishRes.errorTitle, finishRes.errorDetail);
		return false;
	}

	f_close(&fp);

	// Configuration loaded successfully
	cfg.externalConfigPresent = true;

	if (lcd)
	{
		lcd->clear();
		lcd->gotoxy(0, 1);
		lcd->print("  External Config   ");
		lcd->gotoxy(0, 2);
		lcd->print("       Loaded       ");
		unsigned long start = millis();
		while (millis() - start < 2000)
		{
			esp_task_wdt_reset();
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}

	return true;
}
#endif
