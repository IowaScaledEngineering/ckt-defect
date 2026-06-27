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
    // Enumeration for the 3 required states
    enum class IrState {
        IDLE,
        DETECT,
        TIMER
    };

    // Constructor taking pointers to your configuration and live data
    IrStateMachine(DetectorConfiguration* config, DataBundle* dataBundle);
    
    // Destructor
    ~IrStateMachine() = default;

    // Main update loop to process state transitions and actions
    void update();

    // Getter to inspect the current state if needed externally
    IrState getCurrentState() const { return currentState; }

private:
    // Pointers to the external configuration and live data bundles
    DetectorConfiguration* cfg;
    DataBundle* data;
    unsigned long irTime;

    // The current state tracker
    IrState currentState;
};
