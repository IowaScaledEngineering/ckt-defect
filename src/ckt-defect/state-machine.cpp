/*************************************************************************
Title:    State Machine Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     state-machine.cpp
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
#include "state-machine.h"

IrStateMachine::IrStateMachine(DetectorConfiguration* config, DataBundle* dataBundle)
	: cfg(config)
	, data(dataBundle)
	, currentState(IrState::IDLE)
{ }

void IrStateMachine::update() {
	switch (currentState) {
		case IrState::IDLE:
			data->irDetect = false;
			if(data->irInput)
			{
				Serial.println("IR State -> DETECT");
				currentState = IrState::DETECT;
			}
			break;

		case IrState::DETECT:
			data->irDetect = true;
			irTime = millis();
			if(!data->irInput)
			{
				Serial.println("IR State -> TIMER");
				currentState = IrState::TIMER;
			}
			break;

		case IrState::TIMER:
			if(data->irInput)
			{
				Serial.println("IR State -> DETECT");
				currentState = IrState::DETECT;
			}
			else if(((millis() - irTime)/1000) >= cfg->detectorTimeout)
			{
				Serial.println("IR State -> IDLE");
				currentState = IrState::IDLE;
			}
			
			break;
	}
}

AxleStateMachine::AxleStateMachine(DetectorConfiguration* config, DataBundle* dataBundle)
	: cfg(config)
	, data(dataBundle)
	, currentState(AxleState::RESET)
{ }

void AxleStateMachine::update() {
	switch (currentState) {
		case AxleState::RESET:
			data->axleCount = 0;
			data->newAxle = false;
			data->axleDetect = false;
			Serial.println("Axle State -> IDLE");
			currentState = AxleState::IDLE;
			break;

		case AxleState::IDLE:
			data->newAxle = false;
			if(data->axleCountLive > data->axleCount)
			{
				// If this is the first axle, clear the previous speed; used in main loop to detect first axle
				if(0 == data->axleCount)
				{
					data->speed = 0;
				}
				Serial.println("Axle State -> DETECT");
				currentState = AxleState::DETECT;
			}
			else if(data->axleDetect && (((millis() - axleTime)/1000) >= cfg->detectorTimeout))
			{
				Serial.println("Axle State -> TIMEOUT");
				currentState = AxleState::TIMEOUT;
			}
			break;

		case AxleState::DETECT:
			data->axleCount = data->axleCountLive;
			data->newAxle = true;
			data->axleDetect = true;
			axleTime = millis();
			Serial.println("Axle State -> IDLE");
			currentState = AxleState::IDLE;
			break;

		case AxleState::TIMEOUT:
			data->totalAxles = data->axleCount;
			Serial.println("Axle State -> RESET");
			currentState = AxleState::RESET;
			break;
	}
}
