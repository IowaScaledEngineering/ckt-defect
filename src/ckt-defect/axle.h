/*************************************************************************
Title:    Axle Counter Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     axle.h
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

void axleInit();
uint32_t axleGetCount(uint32_t track);
unsigned long axleGetEntranceDeltaMicros(uint32_t track);
unsigned long axleGetExitDeltaMicros(uint32_t track);
unsigned long axleGetLatestAxleTime(uint32_t track);
void axleReset(uint32_t track);
void axleTerminate(void);
