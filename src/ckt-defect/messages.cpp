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

#include "common.h"
#include "messages.h"

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
