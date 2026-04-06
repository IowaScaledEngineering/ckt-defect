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
#include <sstream>

#include "common.h"
#include "io.h"
#include "sound.h"
#include "audio.h"
#include "vocab.h"
#include "sfx.h"
#include "parser.h"

QueueHandle_t parserQueue;

typedef enum
{
	PARSER_IDLE,
	PARSER_DO_SOMETHING,
} ParserState;

ParserState parserState;

bool killParser = false;

TaskHandle_t parseTaskHandle = NULL;

bool parserQueueEmpty(void)
{
	if(0 == uxQueueMessagesWaiting(parserQueue))
		return true;
	else
		return false;
}

void parserQueuePush(ParserObject* obj)
{
	xQueueSend(parserQueue, obj, portMAX_DELAY);
}

BaseType_t parserQueuePop(ParserObject* obj)
{
	return xQueueReceive(parserQueue, obj, portMAX_DELAY);
}


static void parseTask(void *args)
{
	parserState = PARSER_IDLE;
	WavSound wavSound;
	wavSound.seamlessPlay = true;

	ParserObject obj;

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
				if(parserQueuePop(&obj))  // Should only get here when there is something in the queue, so portMAX_DELAY is fine
				{
//					Serial.print("Parsing: ");
//					Serial.println(obj.msg->c_str());
					std::istringstream iss(*(obj.msg));
					std::string token;
					while (iss >> token)
					{
//						Serial.println(token.c_str());

						// Transform special case tokens
						if("." == token)
						{
							// Handle '.' as 'point'
							token = "point";
						}

setTestPoint(TP1);
						while(!audioQueueEmpty())
						{
							vTaskDelay(1 / portTICK_PERIOD_MS);   // Wait 1ms
						}
clrTestPoint(TP1);

						if(token.starts_with("#tone"))
						{
							// 1kHz tone: #tone=M,N
							// M = length in decisecs [optional]
							// N = attenuation factor = 1 / (2^N) [optional]
							size_t pos = token.find('=');
							uint32_t decisecs = 10;   // default: 1 sec
							uint32_t attenuation = 1; // default: 1/2
							if(pos != std::string::npos)
							{
								std::string params = token.substr(pos + 1);
								std::string str_decisecs = "";
								std::string str_attenuation = "";
								pos = params.find(',');
								if(pos != std::string::npos)
								{
									str_decisecs = params.substr(0, pos);
									str_attenuation = params.substr(pos+1);
								}
								else
								{
									// No comma, so extract the entire params as decisecs
									str_decisecs = params;
								}

								// Convert decisecs
								try
								{
									decisecs = std::stoi(str_decisecs);
								}
								catch (const std::invalid_argument& e)
								{
									// Do nothing, use defaults
								}
								catch (const std::out_of_range& e)
								{
									// Do nothing, use defaults
								}

								// Convert attenuation
								try
								{
									attenuation = std::stoi(str_attenuation);
								}
								catch (const std::invalid_argument& e)
								{
									// Do nothing, use defaults
								}
								catch (const std::out_of_range& e)
								{
									// Do nothing, use defaults
								}
							}
							wavSound.wav = new ToneSound(1600*decisecs, 16000, attenuation);
							audioQueuePush(&wavSound);
						}
						else if(token.starts_with("#pause"))
						{
							// Silence: #pause=M
							// M = length in decisecs [optional]
							size_t pos = token.find('=');
							uint32_t decisecs = 5;   // default: 0.5 sec
							if(pos != std::string::npos)
							{
								std::string str_decisecs = token.substr(pos + 1);

								// Convert decisecs
								try
								{
									decisecs = std::stoi(str_decisecs);
								}
								catch (const std::invalid_argument& e)
								{
									// Do nothing, use defaults
								}
								catch (const std::out_of_range& e)
								{
									// Do nothing, use defaults
								}
							}
							wavSound.wav = new SilenceSound(1600*decisecs, 16000);
							audioQueuePush(&wavSound);
						}
						else if(NULL != (wavSound.wav = vocabGetWord(token)))
						{
							audioQueuePush(&wavSound);
						}
						else
						{
							wavSound.wav = sfxGetSound("horn");
							audioQueuePush(&wavSound);
						}


						if(killParser)
						{
							break;  // Escape this while loop
						}
					}
					if(obj.deleteWhenDone)
					{
//						Serial.println("Deleted msg");
						delete obj.msg;
					}
				}
				parserState = PARSER_IDLE;
				break;
		}

		if(killParser)
		{
			killParser = false;
			break;  // Escape the while loop
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
	parserQueue = xQueueCreate(1, sizeof(ParserObject));   /// FIXME?  How many queued messages do we want before the main loop blocks?
	xTaskCreate(parseTask, "parseTask", 8192, NULL, PARSER_TASK_PRIORITY, &parseTaskHandle);
}

void parserTerminate(void)
{
	killParser = true;
	while(killParser)
	{
		delay(10);
	}
	xQueueReset(parserQueue);  // Empty the queue
	vQueueDelete(parserQueue);  // Delete the queue
}
