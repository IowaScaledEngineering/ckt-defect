/*************************************************************************
Title:    Message Parser
Authors:  Michael Petersen <railfan@drgw.net>
File:     parser.cpp
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

#include "common.h"
#include "sound.h"
#include "audio.h"
#include "parser.h"

struct ParserObject {
	std::string* msg;      // Pointer to the message string
	bool deleteWhenDone;   // If true, message pointed to by msg will be deleted
};

bool parserQueueEmpty(void);
void parserQueuePush(ParserObject* obj);
BaseType_t parserQueuePop(ParserObject* obj);
void parserInit(void);
void parserTerminate(void);
