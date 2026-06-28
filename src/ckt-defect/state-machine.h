/*************************************************************************
Title:    State Machine Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     state-machine.h
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

#include "configuration.h"
#include "data.h"

class IrStateMachine {
public:
	enum class IrState {
		IDLE,
		DETECT,
		TIMER
	};

	IrStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
	~IrStateMachine() = default;
	void update();
	IrState getCurrentState() const { return currentState; }
	IrState getNextState() const { return nextState; }

private:
	DetectorConfiguration* cfg;
	DataBundle* data;
	unsigned long irTime;

	IrState currentState;
	IrState nextState;
};

class AxleStateMachine {
public:
	enum class AxleState {
		RESET,
		IDLE,
		DETECT,
		TIMEOUT
	};

	AxleStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
	~AxleStateMachine() = default;
	void update();
	AxleState getCurrentState() const { return currentState; }
	AxleState getNextState() const { return nextState; }

private:
	DetectorConfiguration* cfg;
	DataBundle* data;
	unsigned long axleTime;

	AxleState currentState;
	AxleState nextState;
};

class DetectorStateMachine {
public:
	enum class DetectorState {
		IDLE,
		ENTRANCE_AXLES,
		ENTRANCE_DEFECT,
		ENTRANCE_SPEED,
		ENTRANCE_QUEUE,
		MINIMUM_AXLES,
		AXLE_COUNT,
		AXLE_DEFECT,
		AXLE_DEFECT_QUEUE,
		EXIT_SPEED,
		EXIT_QUEUE,
		TOO_SLOW_QUEUE,
		INTEGRITY_DEFECT_QUEUE,
		BLOCKED_DEFECT_QUEUE,
		WAIT_NO_EXIT,
	};

	DetectorStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
	~DetectorStateMachine() = default;
	void update();
	DetectorState getCurrentState() const { return currentState; }
	DetectorState getNextState() const { return nextState; }

private:
	DetectorConfiguration* cfg;
	DataBundle* data;
	unsigned long axleTime;

	DetectorState currentState;
	DetectorState nextState;
};
