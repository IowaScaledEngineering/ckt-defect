/*************************************************************************
Title:    Defect Message Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     messages.cpp
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
#include <cmath>
#include <cstdlib>

#include "common.h"
#include "messages.h"
#include "temperature.h"

void printMessages(MessageBundle* msgs)
{
	Serial.print("Entrance: ");
	Serial.println(msgs->entranceMsg.c_str());

	for(uint32_t j=0; j<msgs->defects.size(); j++)
	{
		Serial.print("Alert ");
		Serial.print(j);
		Serial.print(": ");
		Serial.println(msgs->defects[j].alertMsg.c_str());
		Serial.print("Detail ");
		Serial.print(j);
		Serial.print(": ");
		Serial.println(msgs->defects[j].detailMsg.c_str());
		Serial.print("Summary ");
		Serial.print(j);
		Serial.print(": ");
		Serial.print(msgs->defects[j].summaryMsg.c_str());
		Serial.print(" [");
		Serial.print(msgs->defects[j].probability);
		Serial.println("]");
	}

	Serial.print("Exit Clean: ");
	Serial.println(msgs->exitCleanMsg.c_str());
	Serial.print("Exit Defect: ");
	Serial.println(msgs->exitDefectMsg.c_str());
	Serial.print("Integrity: ");
	Serial.print(msgs->integrityMsg.c_str());
	Serial.print(" [");
	Serial.print(msgs->integrityProbability);
	Serial.println("]");
	Serial.print("Too Slow: ");
	Serial.println(msgs->tooSlowMsg.c_str());
	Serial.print("Blocked: ");
	Serial.println(msgs->detectorBlockedMsg.c_str());
}

void insertNumber(std::string& str, int32_t num, uint32_t integerDigits, uint32_t fractionalDigits, bool breakDigits)
{
	std::string formatted = intToString(num, integerDigits, fractionalDigits);

	if (breakDigits)
	{
		for (char c : formatted)
		{
			str.push_back(c);
			str.push_back(' ');
		}
		if (!formatted.empty())
		{
			str.pop_back();
		}
	}
	else
	{
		str += formatted;
	}
}

static void parseModifier(const std::string& token, size_t colonPos, int32_t& outN, int32_t& outM, bool hasFraction) 
{
	std::string mod = token.substr(colonPos + 1);
	if (hasFraction) 
	{
		size_t dotPos = mod.find('.');
		if (dotPos != std::string::npos) 
		{
			outN = std::atoi(mod.substr(0, dotPos).c_str());
			outM = std::atoi(mod.substr(dotPos + 1).c_str());
		} 
		else 
		{
			outN = std::atoi(mod.c_str());
		}
	} 
	else 
	{
		outN = std::atoi(mod.c_str());
	}
}

static void formatStringField(std::string& dest, const std::string& src, int32_t width) 
{
	size_t absWidth = std::abs(width);
	std::string clipped = src.substr(0, absWidth);
	
	if (clipped.length() >= absWidth) 
	{
		dest += clipped;
		return;
	}

	size_t padding = absWidth - clipped.length();
	if (width >= 0) 
	{
		dest.append(padding, ' ');
		dest += clipped;
	} 
	else 
	{
		dest += clipped;
		dest.append(padding, ' ');
	}
}

std::string* transformMessage(const std::string& inputMessage, const DetectorConfiguration& cfg, const DataBundle& data, uint8_t trackNum, bool breakDigits)
{
	std::string* outputMessage = new std::string();
	outputMessage->reserve(inputMessage.length() * (breakDigits ? 2 : 1));

	size_t startPos = 0;
	bool first = true;

	while (startPos < inputMessage.length())
	{
		// Skip leading spaces to handle multiple spaces safely
		startPos = inputMessage.find_first_not_of(" \t\r\n", startPos);
		if (startPos == std::string::npos) 
		{
			break;
		}

		// Find the end of the current token
		size_t endPos = inputMessage.find_first_of(" \t\r\n", startPos);
		std::string token = (endPos == std::string::npos) ? 
		                    inputMessage.substr(startPos) : 
		                    inputMessage.substr(startPos, endPos - startPos);

		if (!first)
		{
			outputMessage->push_back(' ');
		}
		first = false;
		
		size_t colonPos = token.find(':');
		std::string baseToken = (colonPos == std::string::npos) ? token : token.substr(0, colonPos);

		if ("#milepost" == baseToken)
		{
			int32_t n = 1, m = 1;
			if (colonPos != std::string::npos) 
			{
				parseModifier(token, colonPos, n, m, true);
			}
			
			if (n < 0) 
			{
				std::string numStr = intToString(cfg.milepost, 0, m);
				size_t absN = std::abs(n);
				(*outputMessage) += numStr;
				if (numStr.length() < absN) 
				{
					outputMessage->append(absN - numStr.length(), ' ');
				}
			} 
			else 
			{
				insertNumber(*outputMessage, cfg.milepost, n, m, breakDigits);
			}
		}
		else if ("#track" == baseToken)
		{
			std::string trackName = cfg.trackName[trackNum];
			if (colonPos != std::string::npos) 
			{
				int32_t n = 0, m = 0;
				parseModifier(token, colonPos, n, m, false);
				formatStringField(*outputMessage, trackName, n);
			} 
			else 
			{
				(*outputMessage) += trackName;
			}
		}
		else if ("#axle" == baseToken || "#axles" == baseToken || "#speed" == baseToken || "#temp" == baseToken)
		{
			int32_t val = 0;
			if ("#axle" == baseToken)       val = data.defectAxle;
			else if ("#axles" == baseToken) val = data.totalAxles;
			else if ("#speed" == baseToken) val = data.speed;
			else if ("#temp" == baseToken)  {
				TemperatureManager* tempMgr = TemperatureManager::getInstance();
				val = tempMgr->getTemperature() + 0.5;
			}

			int32_t n = 1;
			if (colonPos != std::string::npos) 
			{
				int32_t m = 0;
				parseModifier(token, colonPos, n, m, false);
			}

			if (n < 0) 
			{
				std::string numStr = intToString(val, 0, 0);
				size_t absN = std::abs(n);
				(*outputMessage) += numStr;
				if (numStr.length() < absN) 
				{
					outputMessage->append(absN - numStr.length(), ' ');
				}
			} 
			else 
			{
				insertNumber(*outputMessage, val, n, 0, breakDigits);
			}
		}
		else if ("#defectlist" == baseToken)
		{
			for (auto const& defect : data.defects)
			{
				(*outputMessage) += defect;
				outputMessage->push_back(' ');
			}
			if (!data.defects.empty()) 
			{
				outputMessage->pop_back();
			}
		}
		else
		{
			(*outputMessage) += token;
		}

		if (endPos == std::string::npos) 
		{
			break;
		}
		startPos = endPos;
	}

	toLowercase(*outputMessage);
	return outputMessage;
}
