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
#include "common.h"
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

DetectorStateMachine::DetectorStateMachine(DetectorConfiguration* config, DataBundle* dataBundle, MessageBundle* messageBundle, uint8_t track)
	: BaseStateMachine<DetectorState>(config, dataBundle, DetectorState::RESET, "Detector"), msgs(messageBundle), trackNum(track)
{ }

const char* DetectorStateMachine::getStateName(DetectorState state) const
{
	switch (state)
	{
		case DetectorState::RESET:                   return "RESET";
		case DetectorState::IDLE:                    return "IDLE";
		case DetectorState::ENTRANCE_AXLES:          return "ENTRANCE_AXLES";
		case DetectorState::ENTRANCE_DEFECT:         return "ENTRANCE_DEFECT";
		case DetectorState::ENTRANCE_SPEED:          return "ENTRANCE_SPEED";
		case DetectorState::ENTRANCE_QUEUE:          return "ENTRANCE_QUEUE";
		case DetectorState::INFRASTRUCTURE_WAIT:     return "INFRASTRUCTURE_WAIT";
		case DetectorState::MINIMUM_AXLES:           return "MINIMUM_AXLES";
		case DetectorState::AXLE_COUNT:              return "AXLE_COUNT";
		case DetectorState::AXLE_DEFECT:             return "AXLE_DEFECT";
		case DetectorState::EXIT_SPEED:              return "EXIT_SPEED";
		case DetectorState::EXIT_QUEUE:              return "EXIT_QUEUE";
		case DetectorState::TOO_SLOW_QUEUE:          return "TOO_SLOW_QUEUE";
		case DetectorState::INTEGRITY_DEFECT_QUEUE:  return "INTEGRITY_DEFECT_QUEUE";
		case DetectorState::BLOCKED_DEFECT_QUEUE:    return "BLOCKED_DEFECT_QUEUE";
		case DetectorState::WAIT_NO_EXIT:            return "WAIT_NO_EXIT";
		default:                                     return "UNKNOWN";
	}
}

void DetectorStateMachine::enqueueMessageInternal(const std::string& spokenMsg, const std::string* dispMsg)
{
	ParserObject* obj = new ParserObject();
	
	// Spoken message processing
	transformMessage(spokenMsg, obj->msg, *cfg, *data, trackNum, true);
	toLowercase(obj->msg);  // lowercase before parsing

	// Conditional display message processing
	if (dispMsg != nullptr)
	{
		transformMessage(*dispMsg, obj->displayMsg, *cfg, *data, trackNum, false);
	}

	parserQueuePush(obj);

	Serial.print("--> ");
	Serial.println(obj->msg.c_str());
	if (dispMsg != nullptr)
	{
		Serial.println("**> ");
		Serial.println(obj->displayMsg.c_str());
	}
}

void DetectorStateMachine::enqueueMessage(const std::string& spokenMsg)
{
	enqueueMessageInternal(spokenMsg, nullptr);
}

void DetectorStateMachine::enqueueMessage(const std::string& spokenMsg, const std::string& dispMsg)
{
	enqueueMessageInternal(spokenMsg, &dispMsg);
}

void DetectorStateMachine::update()
{
	currentState = nextState;
	switch (currentState)
	{
		case DetectorState::RESET:
			// Cleanup
			data->defects.clear();
			defectCount = 0;
			transitionTo(DetectorState::IDLE);
			break;

		case DetectorState::IDLE:
			if( !cfg->infrastructureMode && data->axleDetect )
			{
				transitionTo(DetectorState::ENTRANCE_AXLES);
			}
			else if( cfg->infrastructureMode && data->irDetect )
			{
				transitionTo(DetectorState::ENTRANCE_DEFECT);
			}
			break;

		case DetectorState::ENTRANCE_AXLES:
			if( !data->axleDetect && !data->irDetect )
			{
				// We didn't get enough axles and everything has timed out (maybe a hand waved over the sensor)
				transitionTo(DetectorState::RESET);
			}
			else if( data->axleCount >= cfg->entranceAxles )
			{
				// We have enough axles, so let's continue
				transitionTo(DetectorState::ENTRANCE_DEFECT);
			}
			break;

		case DetectorState::ENTRANCE_DEFECT:
			// Roll the dice
			if(rollDice() < msgs->integrityProbability)
			{
				// Integrity defect
				transitionTo(DetectorState::INTEGRITY_DEFECT_QUEUE);
			}
			else
			{
				// No defect
				if(!cfg->infrastructureMode && (cfg->speedEnable && cfg->speedTypeEnter))
				{
					// Not infrastructure mode and entrance speed enabled
					transitionTo(DetectorState::ENTRANCE_SPEED);
				}
				else
				{
					transitionTo(DetectorState::ENTRANCE_QUEUE);
				}
			}
			break;

		case DetectorState::ENTRANCE_SPEED:
			if(data->speed > 0)
			{
				// We have a valid speed
				if(data->speed >= cfg->minSpeed)
				{
					transitionTo(DetectorState::ENTRANCE_QUEUE);
				}
				else
				{
					transitionTo(DetectorState::TOO_SLOW_QUEUE);
				}
			}
			else if(!data->axleDetect && !data->irDetect)
			{
				// State machines have timed out.  IR is part of the logic to allow an IR sensor to keep the detector alive.
				if(data->axleInput1 || data->axleInput2)
				{
					// Debounced axle inputs are high, assume they are blocked
					transitionTo(DetectorState::BLOCKED_DEFECT_QUEUE);
				}
				else
				{
					transitionTo(DetectorState::TOO_SLOW_QUEUE);
				}
			}
			break;
			
		case DetectorState::ENTRANCE_QUEUE:
			enqueueMessage(msgs->entranceMsg);
			if(!cfg->infrastructureMode)
			{
				// Not infrastructure mode
				transitionTo(DetectorState::MINIMUM_AXLES);
			}
			else
			{
				// Infrastructure mode
				transitionTo(DetectorState::INFRASTRUCTURE_WAIT);
			}
			break;
			
		case DetectorState::MINIMUM_AXLES:
			if( data->axleCount >= cfg->minAxles )
			{
				// We have enough axles, so let's continue
				transitionTo(DetectorState::AXLE_COUNT);
			}
			else if(!data->axleDetect)
			{
				// Timed out before min axles
				if(data->axleInput1 || data->axleInput2)
				{
					// Something is blocking one of the axle counters
					transitionTo(DetectorState::BLOCKED_DEFECT_QUEUE);
				}
				else
				{
					transitionTo(DetectorState::INTEGRITY_DEFECT_QUEUE);
				}
			}
			break;
			
		case DetectorState::AXLE_COUNT:
			if( data->newAxle )
			{
				transitionTo(DetectorState::AXLE_DEFECT);
			}
			else if(!data->axleDetect && !data->irDetect)
			{
				// Timed out
				if(data->axleInput1 || data->axleInput2)
				{
					// Something is blocking one of the axle counters
					transitionTo(DetectorState::BLOCKED_DEFECT_QUEUE);
				}
				else if(cfg->speedEnable && !cfg->speedTypeEnter)
				{
					// Exit speed enabled
					transitionTo(DetectorState::EXIT_SPEED);
				}
				else
				{
					transitionTo(DetectorState::EXIT_QUEUE);
				}
			}
			break;
			
		case DetectorState::AXLE_DEFECT:
			for (uint32_t i = 0; i < msgs->defects.size(); i++)
			{
				if (rollDice() < msgs->defects[i].probability)
				{
					std::string temporaryMsg;

					defectCount++;  // Yes, this can in theory roll over but we're not going to worry about 4 billion defects...
					if(defectCount <= cfg->maxDefects)
					{
						// Create and store detail message for listing later
						transformMessage(msgs->defects[i].detailMsg, temporaryMsg, *cfg, *data, trackNum, true);
						if(cfg->ordinalDefectList)
						{
							temporaryMsg.insert(0, getOrdinalWord(defectCount) + " ");
						}
						data->defects.push_back(temporaryMsg);
					}
					else if(defectCount == cfg->maxDefects + 1)
					{
						// On the next defect after max, insert excessive alarms message
						data->defects.push_back(msgs->excessAlarmsMsg);
					}
					
					// Create display message
					transformMessage(msgs->defects[i].displayMsg, temporaryMsg, *cfg, *data, trackNum, false);
					
					// Play alert message (and send along display message)
					enqueueMessage(msgs->defects[i].alertMsg, temporaryMsg);
					break;
				}
			}
			transitionTo(DetectorState::AXLE_COUNT);
			break;
			
		case DetectorState::EXIT_SPEED:
			if(data->speed >= cfg->minSpeed)
			{
				transitionTo(DetectorState::EXIT_QUEUE);
			}
			else
			{
				transitionTo(DetectorState::TOO_SLOW_QUEUE);
			}
			break;
			
		case DetectorState::EXIT_QUEUE:
			if(data->defects.size() > 0)
			{
				// We have defects
				enqueueMessage(msgs->exitDefectMsg, msgs->exitDefectDisplayMsg);
			}
			else
			{
				enqueueMessage(msgs->exitCleanMsg, msgs->exitCleanDisplayMsg);
			}
			transitionTo(DetectorState::RESET);
			break;
			
		case DetectorState::INFRASTRUCTURE_WAIT:
			if(!data->irDetect)
			{
				transitionTo(DetectorState::EXIT_QUEUE);
			}
			break;
			
		case DetectorState::TOO_SLOW_QUEUE:
			enqueueMessage(msgs->tooSlowMsg, msgs->tooSlowDisplayMsg);
			transitionTo(DetectorState::WAIT_NO_EXIT);
			break;
			
		case DetectorState::INTEGRITY_DEFECT_QUEUE:
			enqueueMessage(msgs->integrityMsg, msgs->integrityDisplayMsg);
			transitionTo(DetectorState::WAIT_NO_EXIT);
			break;
			
		case DetectorState::BLOCKED_DEFECT_QUEUE:
			enqueueMessage(msgs->detectorBlockedMsg, msgs->detectorBlockedDisplayMsg);
			transitionTo(DetectorState::WAIT_NO_EXIT);
			break;
			
		case DetectorState::WAIT_NO_EXIT:
			if(!data->axleDetect && !data->irDetect && !data->axleInput1 && !data->axleInput2)
			{
				transitionTo(DetectorState::RESET);
			}
			break;
			
	}
}
