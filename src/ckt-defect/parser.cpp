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
#include <string_view>

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

volatile bool killParser = false;

TaskHandle_t parseTaskHandle = NULL;

std::string displayMessage;

bool parserQueueEmpty(void)
{
	if(0 == uxQueueMessagesWaiting(parserQueue))
		return true;
	else
		return false;
}

void parserQueuePush(ParserObject* obj)
{
	xQueueSend(parserQueue, &obj, portMAX_DELAY);
}

BaseType_t parserQueuePop(ParserObject** obj)
{
	return xQueueReceive(parserQueue, obj, portMAX_DELAY);
}


static void parseTask(void *args)
{
	parserState = PARSER_IDLE;
	WavSound wavSound = { nullptr, true };

	ParserObject* obj = nullptr;

	while(1)
	{
		switch(parserState)
		{
			case PARSER_IDLE:
				if(!parserQueueEmpty() && audioQueueEmpty())
				{
					// Something is in the parser queue and the audio is not playing
					parserState = PARSER_DO_SOMETHING;
				}
				break;

			case PARSER_DO_SOMETHING:
				if(parserQueuePop(&obj) && obj != nullptr)  // Should only get here when there is something in the queue, so portMAX_DELAY is fine
				{
					Serial.print("Parsing: ");
					Serial.println(obj->msg.c_str());
					
					std::string_view msg_view(obj->msg);
					size_t start = 0;
					size_t end = 0;

					while((start = msg_view.find_first_not_of(" \t\r\n", end)) != std::string_view::npos)
					{
						end = msg_view.find_first_of(" \t\r\n", start);
						std::string_view token = msg_view.substr(start, end - start);
						Serial.print('[');
						Serial.print(std::string(token).c_str());
						Serial.println(']');

						// Transform special case tokens
						std::string_view lookup_token = token;
						if("." == token)
						{
							// Handle '.' as 'point'
							lookup_token = "point";
						}
						else if("-" == token)
						{
							// Handle '-' as 'minus'
							lookup_token = "minus";
						}

setTestPoint(TP1);
						while(!audioQueueEmpty())
						{
							vTaskDelay(10 / portTICK_PERIOD_MS);   // Wait 10ms
						}
clrTestPoint(TP1);

						if(lookup_token.starts_with("#tone"))
						{
							// 1kHz tone: #tone=M,N
							// M = length in decisecs [optional]
							// N = attenuation factor = 1 / (2^N) [optional]
							size_t pos = lookup_token.find('=');
							uint32_t decisecs = 10;   // default: 1 sec
							uint32_t attenuation = 1; // default: 1/2
							if(pos != std::string_view::npos)
							{
								std::string_view params = lookup_token.substr(pos + 1);
								std::string_view str_decisecs = "";
								std::string_view str_attenuation = "";
								pos = params.find(',');
								if(pos != std::string_view::npos)
								{
									str_decisecs = params.substr(0, pos);
									str_attenuation = params.substr(pos + 1);
								}
								else
								{
									// No comma, so extract the entire params as decisecs
									str_decisecs = params;
								}

								// Convert decisecs
								try
								{
									decisecs = std::stoi(std::string(str_decisecs));
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
									attenuation = std::stoi(std::string(str_attenuation));
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
							wavSound.wav = new ToneSound(1600 * decisecs, 16000, attenuation);
							audioQueuePush(&wavSound);
						}
						else if(lookup_token.starts_with("#pause"))
						{
							// Silence: #pause=M
							// M = length in decisecs [optional]
							size_t pos = lookup_token.find('=');
							uint32_t decisecs = 5;   // default: 0.5 sec
							if(pos != std::string_view::npos)
							{
								std::string_view str_decisecs = lookup_token.substr(pos + 1);

								// Convert decisecs
								try
								{
									decisecs = std::stoi(std::string(str_decisecs));
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
							wavSound.wav = new SilenceSound(1600 * decisecs, 16000);
							audioQueuePush(&wavSound);
						}
						else if(NULL != (wavSound.wav = vocabGetWord(std::string(lookup_token))))
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
					delete obj;
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
	parserQueue = xQueueCreate(10, sizeof(ParserObject*));   /// FIXME?  How many queued messages do we want before the main loop blocks?
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

std::string getDisplayMessage(void)
{
	return displayMessage;
}
