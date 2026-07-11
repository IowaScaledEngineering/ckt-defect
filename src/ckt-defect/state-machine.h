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
#include "messages.h"
#include "parser.h"

// --- State Enumerations ---

enum class IrState {
	IDLE,
	DETECT,
	TIMER
};

enum class AxleState {
	RESET,
	IDLE,
	DETECT,
	TIMEOUT
};

enum class DetectorState {
	RESET,
	IDLE,
	ENTRANCE_AXLES,
	ENTRANCE_DEFECT,
	ENTRANCE_SPEED,
	ENTRANCE_QUEUE,
	INFRASTRUCTURE_WAIT,
	MINIMUM_AXLES,
	AXLE_COUNT,
	AXLE_DEFECT,
	EXIT_SPEED,
	EXIT_QUEUE,
	TOO_SLOW_QUEUE,
	INTEGRITY_DEFECT_QUEUE,
	BLOCKED_DEFECT_QUEUE,
	WAIT_NO_EXIT,
};

// --- Base State Machine Template ---

template <typename StateType>
class BaseStateMachine {
public:
	BaseStateMachine(DetectorConfiguration* config, DataBundle* dataBundle, StateType initialState, const char* machineName)
		: cfg(config)
		, data(dataBundle)
		, lastStateTime(0)
		, currentState(initialState)
		, nextState(initialState)
		, name(machineName)
	{ }

	virtual ~BaseStateMachine() = default;
	
	virtual void update() = 0;

	StateType getCurrentState() const { return currentState; }
	StateType getNextState() const { return nextState; }

protected:
	DetectorConfiguration* cfg;
	DataBundle* data;
	unsigned long lastStateTime;

	// Store both current and next state so transitions can be detected (current != next)
	StateType currentState;
	StateType nextState;
	const char* name; // Stores the name of the machine (e.g., "IR", "Axle")

	// Centralized transition helper
	void transitionTo(StateType newState) {
		if (newState != currentState) {
			Serial.print(name);
			Serial.print(" State -> ");
			Serial.println(getStateName(newState));
			nextState = newState;
		}
	}

	// Must be implemented by derived classes to convert enum to string
	virtual const char* getStateName(StateType state) const = 0;
};

// --- Derived State Machines ---

class IrStateMachine : public BaseStateMachine<IrState> {
public:
	IrStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
	void update() override;
	const char* getStateName(IrState state) const override;
};

class AxleStateMachine : public BaseStateMachine<AxleState> {
public:
	AxleStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
	void update() override;
	const char* getStateName(AxleState state) const override;
};

class DetectorStateMachine : public BaseStateMachine<DetectorState> {
public:
	DetectorStateMachine(DetectorConfiguration* config, DataBundle* dataBundle, MessageBundle* messageBundle, uint8_t track);
	void update() override;
	const char* getStateName(DetectorState state) const override;

protected:
	MessageBundle* msgs;
	uint8_t trackNum;

	void enqueueMessage(const std::string& message); 
};

