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

#include <Arduino.h>

#include "common.h"
#include "sound.h"
#include "audio.h"
#include "parser.h"

QueueHandle_t parserQueue;

typedef enum
{
	PARSER_IDLE,
	PARSER_DO_SOMETHING,
} ParserState;

ParserState parserState;

TaskHandle_t parseTaskHandle = NULL;

bool parserQueueEmpty(void)
{
	if(0 == uxQueueMessagesWaiting(parserQueue))
		return true;
	else
		return false;
}

void parserQueuePush(std::string** msgPtr)
{
	xQueueSend(parserQueue, msgPtr, portMAX_DELAY);
}

BaseType_t parserQueuePop(std::string** msgPtr)
{
	return xQueueReceive(parserQueue, msgPtr, portMAX_DELAY);
}


static void parseTask(void *args)
{
	std::string* msgPtr;
	parserState = PARSER_IDLE;

	while(1)
	{
		switch(parserState)
		{
			case PARSER_IDLE:
				if(!parserQueueEmpty())
				{
					// Queue not empty
					parserState = PARSER_DO_SOMETHING;
				}
				break;

			case PARSER_DO_SOMETHING:
				if(parserQueuePop(&msgPtr))  // Should only get here when there is something in the queue, so portMAX_DELAY is fine
				{
					Serial.print("Parsing: ");
					Serial.println(msgPtr->c_str());
				}
				parserState = PARSER_IDLE;
				break;
		}

		if(PARSER_IDLE == parserState)
		{
			vTaskDelay(10 / portTICK_PERIOD_MS);  // Block execution of this task for 10ms since we're not doing anything useful at the moment
		}
		
	}
	vTaskDelete(NULL);
}


void parserInit(void)
{
	parserQueue = xQueueCreate(10, sizeof(std::string *));
	xTaskCreate(parseTask, "parseTask", 8192, NULL, 5, &parseTaskHandle);
}
