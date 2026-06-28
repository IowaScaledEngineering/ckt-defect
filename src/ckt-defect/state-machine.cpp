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

// ==========================================
// IrStateMachine Implementation
// ==========================================

IrStateMachine::IrStateMachine(DetectorConfiguration* config, DataBundle* dataBundle)
	: BaseStateMachine<IrState>(config, dataBundle, IrState::IDLE, "IR")
{ }

const char* IrStateMachine::getStateName(IrState state) const
{
	switch (state)
	{
		case IrState::IDLE:   return "IDLE";
		case IrState::DETECT: return "DETECT";
		case IrState::TIMER:  return "TIMER";
		default:              return "UNKNOWN";
	}
}

void IrStateMachine::update()
{
	currentState = nextState;
	switch (currentState)
	{
		case IrState::IDLE:
			data->irDetect = false;
			if (data->irInput)
			{
				transitionTo(IrState::DETECT);
			}
			break;

		case IrState::DETECT:
			data->irDetect = true;
			lastStateTime = millis();
			if (!data->irInput)
			{
				transitionTo(IrState::TIMER);
			}
			break;

		case IrState::TIMER:
			if (data->irInput)
			{
				transitionTo(IrState::DETECT);
			}
			else if (((millis() - lastStateTime) / 1000) >= cfg->detectorTimeout)
			{
				transitionTo(IrState::IDLE);
			}
			break;
	}
}

// ==========================================
// AxleStateMachine Implementation
// ==========================================

AxleStateMachine::AxleStateMachine(DetectorConfiguration* config, DataBundle* dataBundle)
	: BaseStateMachine<AxleState>(config, dataBundle, AxleState::RESET, "Axle")
{ }

const char* AxleStateMachine::getStateName(AxleState state) const
{
	switch (state)
	{
		case AxleState::RESET:   return "RESET";
		case AxleState::IDLE:    return "IDLE";
		case AxleState::DETECT:  return "DETECT";
		case AxleState::TIMEOUT: return "TIMEOUT";
		default:                 return "UNKNOWN";
	}
}

void AxleStateMachine::update()
{
	currentState = nextState;
	switch (currentState)
	{
		case AxleState::RESET:
			data->axleCount = 0;
			data->newAxle = false;
			data->axleDetect = false;
			transitionTo(AxleState::IDLE);
			break;

		case AxleState::IDLE:
			data->newAxle = false;
			if (data->axleCountLive > data->axleCount)
			{
				// If this is the first axle, clear the previous speed; used in main loop to detect first axle
				if (0 == data->axleCount)
				{
					data->speed = 0;
				}
				transitionTo(AxleState::DETECT);
			}
			else if (data->axleDetect && (((millis() - lastStateTime) / 1000) >= cfg->detectorTimeout))
			{
				transitionTo(AxleState::TIMEOUT);
			}
			break;

		case AxleState::DETECT:
			data->axleCount = data->axleCountLive;
			data->newAxle = true;
			data->axleDetect = true;
			lastStateTime = millis();
			transitionTo(AxleState::IDLE);
			break;

		case AxleState::TIMEOUT:
			data->totalAxles = data->axleCount;
			transitionTo(AxleState::RESET);
			break;
	}
}

// ==========================================
// DetectorStateMachine Implementation
// ==========================================

DetectorStateMachine::DetectorStateMachine(DetectorConfiguration* config, DataBundle* dataBundle)
	: BaseStateMachine<DetectorState>(config, dataBundle, DetectorState::IDLE, "Detector")
{ }

void DetectorStateMachine::update()
{
	currentState = nextState;
	switch (currentState)
	{
		case DetectorState::IDLE:
			break;
	}
}
