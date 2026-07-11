/*************************************************************************
Title:    Defect Detector
Authors:  Michael Petersen <railfan@drgw.net>
File:     ckt-defect.c
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
#include <SPI.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

#include "common.h"
#include "configuration.h"
#include "io.h"
#include "sound.h"
#include "audio.h"
#include "messages.h"
#include "parser.h"
#include "vocab.h"
#include "sfx.h"
#include "axle.h"
#include "display-lcd.h"
#include "src/menu/menu.h"
#include "src/menu/menu-mgr.h"
#include "menu-factory.h"
#include "state-machine.h"
#include "temperature.h"


// 3 sec watchdog 
#define TWDT_TIMEOUT_MS    3000

bool restart = false;

struct WavData {
	uint32_t sampleRate;
	uint32_t wavDataSize;
	size_t dataStartPosition;
};

void printMemoryUsage(void)
{
	Serial.printf("\n[SYS]: stack: %u heap: %u\n\n", uxTaskGetStackHighWaterMark(NULL), xPortGetFreeHeapSize());
}

void printNVSStats()
{
	nvs_stats_t nvs_stats;
	
	// Get stats for the default "nvs" partition
	esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
	
	if (err == ESP_OK)
	{
		Serial.println("--- NVS Preferences Stats ---");
		Serial.print("Used Entries: ");
		Serial.println(nvs_stats.used_entries);
		Serial.print("Free Entries: ");
		Serial.println(nvs_stats.free_entries);
		Serial.print("Total Entries: ");
		Serial.println(nvs_stats.total_entries);
		Serial.print('\n');
	}
	else
	{
		Serial.println("Failed to get NVS stats");
	}
}

char* rtrim(char* in)
{
	char* endPtr = in + strlen(in) - 1;
	while (endPtr >= in && isspace(*endPtr))
		*endPtr-- = 0;

	return in;
}

char* ltrim(char* in)
{
	char* startPtr = in;
	uint32_t bytesToMove = strlen(in);
	while(isspace(*startPtr))
		startPtr++;
	bytesToMove -= (startPtr - in);
	memmove(in, startPtr, bytesToMove);
	in[bytesToMove] = 0;
	return in;
}

bool configKeyValueSplit(char* key, uint32_t keySz, char* value, uint32_t valueSz, const char* configLine)
{
	char lineBuffer[256];
	char* separatorPtr = NULL;
	char* lineBufferPtr = NULL;
	uint32_t bytesToCopy;

	separatorPtr = strchr(configLine, '=');
	if (NULL == separatorPtr)
		return false;

	memset(key, 0, keySz);
	memset(value, 0, valueSz);

	// Copy the part that's eligible to be a key into the line buffer
	bytesToCopy = separatorPtr - configLine;
	if (bytesToCopy > sizeof(lineBuffer)-1)
		bytesToCopy = sizeof(lineBuffer);
	memset(lineBuffer, 0, sizeof(lineBuffer));
	strncpy(lineBuffer, configLine, bytesToCopy);

	lineBufferPtr = ltrim(rtrim(lineBuffer));
	if (0 == strlen(lineBufferPtr) || '#' == lineBufferPtr[0])
		return false;

	strncpy(key, lineBufferPtr, keySz);

//	bytesToCopy = strlen(separatorPtr+1);
//	if (bytesToCopy > sizeof(lineBuffer)-1)
//		bytesToCopy = sizeof(lineBuffer);
	memset(lineBuffer, 0, sizeof(lineBuffer));
	// Changed to sizeof(lineBuffer)-1 below instead of bytesToCopy due to -Werror=stringop-overflow and -Werror=stringop-truncation
	strncpy(lineBuffer, separatorPtr+1, sizeof(lineBuffer)-1);
	lineBufferPtr = ltrim(rtrim(lineBuffer));
	if (0 == strlen(lineBufferPtr))
	{
		memset(key, 0, keySz);
		return false;
	}
	strncpy(value, lineBufferPtr, valueSz);
	return true;
}


hw_timer_t * timer = NULL;
volatile bool timerTick = false;

void IRAM_ATTR tickTimer(void)
{
	timerTick = true;
}

bool validateWavFile(File *wavFile, struct WavData *wavData)
{
	const char *fileName;
	size_t fileNameLength;
	uint16_t channels;
	uint16_t bitsPerSample;
	uint32_t sampleRate;
	uint32_t wavDataSize;

	fileName = wavFile->name();
	fileNameLength = strlen(fileName);
	if(fileNameLength < 5)
		return false;  // Filename too short (x.wav = min 5 chars)
	const char *extension = &fileName[strlen(fileName)-4];
	if(strcasecmp(extension, ".wav"))
	{
		Serial.print("	Ignoring: ");
		Serial.println(fileName);
		return false;  // Not a wav file (by extension anyway)
	}
	
	if(!wavFile->find("fmt "))  // Includes trailing space
	{
		Serial.print("! No fmt section: ");
		Serial.println(fileName);
		return false;
	}

	wavFile->seek(wavFile->position() + 6);  // Seek to number of channels
	wavFile->read((uint8_t*)&channels, 2);  // Read channels - WAV is little endian, only works if uC is also little endian

	if(channels > 1)
	{
		Serial.print("! Not mono: ");
		Serial.println(fileName);
		return false;
	}

	wavFile->read((uint8_t*)&sampleRate, 4);  // Read sample rate - WAV is little endian, only works if uC is also little endian
	wavData->sampleRate = sampleRate;

	if((8000 != sampleRate) && (16000 != sampleRate) && (32000 != sampleRate) && (44100 != sampleRate))
	{
		Serial.print("! Incorrect sample rate: ");
		Serial.println(fileName);
		return false;
	}

	wavFile->seek(wavFile->position() + 6);  // Seek to bits per sample
	wavFile->read((uint8_t*)&bitsPerSample, 2);	// Read bits per sample - WAV is little endian, only works if uC is also little endian

	if(16 != bitsPerSample)
	{
		Serial.print("! Not 16-bit: ");
		Serial.println(fileName);
		return false;
	}

	if(!wavFile->find("data"))
	{
		Serial.print("! No data section: ");
		Serial.println(fileName);
		return false;
	}

	wavFile->read((uint8_t*)&wavDataSize, 4);	// Read data size - WAV is little endian, only works if uC is also little endian
	wavData->wavDataSize = wavDataSize;
	// Actual data is now the current position
	
	wavData->dataStartPosition = wavFile->position();
	return true;
}

void findWavFiles(File *rootDir, String dirName, std::vector<Sound *> *soundsVector)
{
	File wavFile;
	WavData wavData;

	while(true)
	{
		esp_task_wdt_reset();
		wavFile = rootDir->openNextFile();

		if (!wavFile)
		{
			break;	// No more files
		}
		if(wavFile.isDirectory())
		{
			Serial.print("	Skipping directory: ");
			Serial.println(wavFile.name());
		}
		else
		{
			if(validateWavFile(&wavFile, &wavData))
			{
				// If we got here, then it looks like a valid wav file
				String fullFileName = dirName + wavFile.name();

				Serial.print("+ Adding ");
				Serial.print(fullFileName);
				Serial.print(" (");
				Serial.print(wavData.sampleRate);
				Serial.print(",");
				Serial.print(wavData.wavDataSize);
				Serial.print(",");
				Serial.print(wavData.dataStartPosition);
				Serial.print(")");

				Serial.println("");

				soundsVector->push_back(new SdSound(fullFileName.c_str(), wavData.wavDataSize, wavData.dataStartPosition, wavData.sampleRate));
			}
		}
		wavFile.close();
	}
}


void setup()
{
	Serial.begin();

	ioInit();

	Wire.begin(SDA, SCL);

	esp_task_wdt_config_t twdt_config = {
		.timeout_ms = TWDT_TIMEOUT_MS,
		.idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
		.trigger_panic = false,
	};
	esp_task_wdt_init(&twdt_config);
    	esp_task_wdt_add(NULL); //add current thread to WDT watch
	esp_task_wdt_reset();

	timer = timerBegin(1000000);                  // 1MHz = 1us
	timerAttachInterrupt(timer, &tickTimer);
	timerAlarm(timer, 10000, true, 0);            // 1us * 10000 = 10ms, autoreload, unlimited reloads
}


void loop()
{
	File rootDir;
	
	DetectorConfiguration cfg;
	MessageBundle trackMessages;
	DataBundle data[NUM_TRACKS];
	
	DisplayLcd *lcd = new DisplayLcd();
	bool displayPresent = true;  // Start assuming it's there so we don't take the refresh delay initially
	bool oldDisplayPresent = true;
	bool lcdRefresh = false;

	bool sdCardPresent = false;
	bool configFilePresent = false;
	bool externalVocabPresent = false;

	TemperatureManager temperatureMgr(&cfg);
	unsigned long temperatureUpdateTime = millis();
	
/*
	uint8_t state = 255;  // Start in default
	uint8_t returnState = 0;
	ParserObject obj;
	std::string* msg;
	std::string* msgPtr;
	unsigned long startTime;
	unsigned long axleTime = millis();
*/

	bool sdCardInserted = false;
	unsigned long sdDetectTime = 0;

	std::array<IrStateMachine, 2> irStateMachines = {
		IrStateMachine(&cfg, &data[0]),
		IrStateMachine(&cfg, &data[1])
	};

	std::array<AxleStateMachine, 2> axleStateMachines = {
		AxleStateMachine(&cfg, &data[0]),
		AxleStateMachine(&cfg, &data[1])
	};

	std::array<DetectorStateMachine, 2> detectorStateMachines = {
		DetectorStateMachine(&cfg, &data[0], &trackMessages, 0),
		DetectorStateMachine(&cfg, &data[1], &trackMessages, 1)
	};

	uint32_t centisecs = 0;
	bool decisecsTick = true; // Trigger initially
	uint32_t decisecs = 0;
	bool secondsTick = true;  // Trigger initially

	esp_task_wdt_reset();

	// Read NVM configuration
	loadConfiguration(&cfg);
	// Preload the track name based on loaded configuration.  Might be overwritten below by SD card.
	updateTrackNames(&cfg);
	updateDirectionNames(&cfg);

	audioSetVolumeStep(cfg.volumeStep);
	lcd->setBrightness(cfg.lcdBrightness);
	audioSetVolumeUpCoef(10);   // FIXME: should be leaded from NVM
	audioSetVolumeDownCoef(8);   // FIXME: should be leaded from NVM;
	audioSetPttDelay(750);   // FIXME: should be leaded from NVM


	// Show splash screen
	lcd->clear();
	lcd->backlightOn();
	uint8_t copyright[8] = {
		0b01110, 
		0b11011, 
		0b10101, 
		0b10111, 
		0b10101, 
		0b11011, 
		0b01110, 
		0b00000};
	lcd->createCustomChar(0, copyright);

	lcd->gotoxy(3,0);
	lcd->print("Talking Defect");
	lcd->gotoxy(2,1);
	lcd->print("Detector ");
	lcd->print(VERSION_STRING);
	lcd->gotoxy(2,2);
	lcd->print("Iowa Scaled Engr");
	lcd->gotoxy(2,3);
	lcd->print("www.iascaled.com");


	// Create menus
	auto menus = createAppMenu(cfg, lcd, data);

	Menu::setDisplay(lcd);
	Menu::setTimingCallback(millis); 
	Menu::setInitialHoldDelay(1000);
	Menu::setHoldDelay(400);
	Menu::setLongHoldDelay(3000);
	Menu::setFastDelay(100);

	MenuManager menuManager(menus);


	// Set up audio
	audioSetPttEnableCallback(enableAuxRelay);
	audioSetPttDisableCallback(disableAuxRelay);


	// Load sound effects
	loadSfx();


	// Initialize SPI and SD card
	SPIClass vspi = SPIClass(FSPI);
	vspi.begin(SDCLK, SDMISO, SDMOSI, SDCS);
	if(SD.begin(SDCS, vspi))
	{
		sdCardPresent = true;
	}

	
	// Check for config file and load data from it if present
	// FIXME
	if(sdCardPresent)
	{
		// Check for and read config file
		File f = SD.open("/config.txt");
		if (f)
		{
			configFilePresent = true;
			// FIXME: Somewhere in here overwrite trackName[i] from config file
			while(f.available())
			{
				char keyStr[128];
				char valueStr[128];
				bool kvFound = configKeyValueSplit(keyStr, sizeof(keyStr), valueStr, sizeof(valueStr), f.readStringUntil('\n').c_str());
				if (!kvFound)
					continue;

				// Okay, looks like we have a valid key/value pair, see if it's something we care about
				if (0 == strcmp(keyStr, "volumeUp"))
				{
					audioSetVolumeUpCoef(atoi(valueStr));
				}
				else if (0 == strcmp(keyStr, "volumeDown"))
				{
					audioSetVolumeDownCoef(atoi(valueStr));
				}
			}
		}
		f.close();

		esp_task_wdt_reset();
	}


	// If no config file, set defaults
	if(!configFilePresent)
	{
		// Entrance Message
		trackMessages.entranceMsg = "Equipment Defect Detector";
//		trackMessages.entranceMsg = "1 #tone 2 #tone= 3 #tone=5 4 #tone=20 5 #tone=10,0 #tone=10,1 #tone=10,2 #tone=10,3";   // Tone Test
//		trackMessages.entranceMsg = "#tone=1 #pause #tone=1 #pause= #tone=1 #pause=10 #tone=1 #pause=20 #tone";   // Silence Test
		if(cfg.milepostEnable)
		{
			trackMessages.entranceMsg += " milepost #milepost";
		}
		trackMessages.entranceMsg += " #track";
		// Defect Messages
		std::string tmpMessage;
		if(cfg.axleEnable)
		{
			tmpMessage = " Axle #axle";
		}
		trackMessages.defects.emplace_back("#tone hot journal" + tmpMessage, "hot journal" + tmpMessage, "Hot Journal" + tmpMessage, 500);
		trackMessages.defects.emplace_back("#tone dragging equipment near" + tmpMessage, "dragging equipment near" + tmpMessage, "Dragging Equipment" + tmpMessage, 100);
		trackMessages.defects.emplace_back("#tone high impact wheel detected" + tmpMessage, "high impact wheel detected" + tmpMessage, "High Impact Wheel" + tmpMessage, 200);

		// Create Footer
		tmpMessage.clear();
		if(cfg.axleEnable)
		{
			tmpMessage += " total axles #axles";
		}

		if(cfg.speedEnable)
		{
			tmpMessage += " train speed #speed";
		}

		if(cfg.temperatureEnable)
		{
			tmpMessage += " temperature #temp degrees";
		}

		// Clean Exit Message
		trackMessages.exitCleanMsg = trackMessages.entranceMsg + " no defects repeat no defects" + tmpMessage;

		// Defect Exit Message
		trackMessages.exitDefectMsg = trackMessages.entranceMsg + " you have a defect #defectlist" + tmpMessage + " detector out";

		// Detector Integrity Message
		trackMessages.integrityMsg = trackMessages.entranceMsg + " integrity failure" + tmpMessage ;
		trackMessages.integrityProbability = 50;
		
		// Too Slow Message
		trackMessages.tooSlowMsg = trackMessages.entranceMsg + " train 2 slow";
		
		// Detector Blocked Message
		trackMessages.detectorBlockedMsg = trackMessages.entranceMsg + " detector blocked";
	}


	// Check for external vocab
	// FIXME
	if(sdCardPresent)
	{
	}
	
	
	// If no SD vocab, load the internal ones
	if(!externalVocabPresent)
	{
		loadInternalVocab();
	}


	// Sort defect messages by probability
	sort(trackMessages.defects.begin(), trackMessages.defects.end(), [](DefectMessage a, DefectMessage b) {
		return a.probability < b.probability; // returns true if 'a' should come before 'b'
		});



	//  Initialize temperature after config is loaded, but before menu starts running
	temperatureMgr.begin();



	// Wait for splash screen timeout
	while(millis() < 3000)
	{
		esp_task_wdt_reset();
	}

	menuManager.begin();
	menuManager.process();  // Call once here to get things going


	Serial.println("ISE Defect Detector");
	Serial.print("Version: ");
	Serial.println(VERSION_STRING);
	Serial.print("Git Rev: ");
	Serial.println(GIT_REV);

	printMemoryUsage();
	printNVSStats();

	Serial.print("Random Number: ");
	Serial.println(rollDice());
	Serial.print('\n');




	// Print configuration values
	Serial.print('\n');
	printMessages(&trackMessages);
	Serial.print('\n');
	printConfiguration(&cfg);
	Serial.print('\n');
	











	esp_task_wdt_reset();

	audioInit();
	parserInit();
	axleInit();
	axleReset(0);
	axleReset(1);

	audioUnmute();

	while(1)
	{
		esp_task_wdt_reset();

		// Do things on 10ms interval
		if(timerTick)
		{
setTestPoint(TP2);
			timerTick = false;
			audioProcessVolume();
			displayPresent = lcd->readKeys();

			menuManager.process();
clrTestPoint(TP2);
			if(++centisecs >= 10)
			{
				centisecs -= 10;
				decisecsTick = true;
			}
		}
		
		// Do things on a 100ms interval
		if(decisecsTick)
		{
			decisecsTick = false;
			ioProcessInputs();

			data[0].irInput = getIrA();
			data[0].axleInput1 = getAxleA1();
			data[0].axleInput2 = getAxleA2();

			data[1].irInput = getIrB();
			data[1].axleInput1 = getAxleB1();
			data[1].axleInput2 = getAxleB2();

			// Refresh display if needed
			if(lcdRefresh)
			{
				Serial.println("*** Refreshing display ***");
				lcd->refresh();
				lcdRefresh = false;
			}
			else if(displayPresent && !oldDisplayPresent)
			{
				lcdRefresh = true;  // Refresh next time through to give things time to settle
			}
			oldDisplayPresent = displayPresent;

			if(++decisecs >= 10)
			{
				decisecs -= 10;
				secondsTick = true;
			}
		}

		// Do things on a 1s interval
		if(secondsTick)
		{
			secondsTick = false;
//			printMemoryUsage();
		}

		// Update temperature every 10 sec
		if( (millis()-temperatureUpdateTime) >= 10000 )
		{
			temperatureUpdateTime = millis();
			temperatureMgr.update();
//			Serial.println(temperatureMgr.getTemperature());
		}
		
		// Update the axle counts
		data[0].axleCountLive = axleGetCount(0);
		data[1].axleCountLive = axleGetCount(1);

		// Update State Machines
		for(uint32_t i = 0; i<NUM_TRACKS; i++)
		{
			irStateMachines[i].update();
			axleStateMachines[i].update();

			if(AxleState::RESET == axleStateMachines[i].getCurrentState())
				axleReset(i);

			else if(AxleState::TIMEOUT == axleStateMachines[i].getCurrentState() && !cfg.speedTypeEnter)
			{
				// Calculate exit speed
				// FIXME: add scale ratio and units
				data[i].speedFloat = 4943182.0/(axleGetExitDeltaMicros(i));
				data[i].speed = data[i].speedFloat + 0.5;
			}

			if((axleGetEntranceDeltaMicros(i) > 0) && cfg.speedTypeEnter && (0 == data[i].speed))
			{
				// Calculate entrance speed
				// FIXME: Add scale ratio and units
				// 87 * 1000000 us/s * 3600 s/hr               us * mile
				// ----------------------------- = 4,943,182 * ---------
				// 12 in/ft * 5280ft/mile                       in * hr
				data[i].speedFloat = 4943182.0/(axleGetEntranceDeltaMicros(i));
				data[i].speed = data[i].speedFloat + 0.5;
			}
			
			if(AxleState::TIMEOUT == axleStateMachines[i].getCurrentState())
			{
				// Print some data if leaving timeout state (-> reset)
				Serial.print("Track ");
				Serial.println((char)('A' + i));
				Serial.print("Total Axles: ");
				Serial.println(data[i].totalAxles);
				Serial.print("Speed: ");
				Serial.print(data[i].speed);
				Serial.print(" (");
				Serial.print(data[i].speedFloat);
				Serial.println(")");
			}

			//  Finally, update the main detector state machine
			detectorStateMachines[i].update();

			if(DetectorState::IDLE == detectorStateMachines[i].getCurrentState())
			{
				data[i].active = false;
			}
			else
			{
				data[i].active = true;
			}
		}

		// Check for serial input
		if(Serial.available() > 0)
		{
			uint8_t serialChar = Serial.read();
			switch(serialChar)
			{
				case '+':
					cfg.volumeStep++;
					audioSetVolumeStep(cfg.volumeStep);
					saveConfiguration(&cfg);
					break;
				case '-':
					cfg.volumeStep--;
					audioSetVolumeStep(cfg.volumeStep);
					saveConfiguration(&cfg);
					break;
				case 'q':
					restart = true;
					break;
				case '~':
					Serial.print("Clearing preferences...");
					resetConfiguration();
					break;
			}
		}






		// FIXME Send some test audio
/*
		switch(state)
		{
			case 0:
//				data.temperature = 97;
				data[0].defectAxle = 0;
				data[0].totalAxles = 0;
				data[0].speed = 0;
				if(millis() - startTime >= 1000)
					state++;
				break;


			case 1:
				msgPtr = &trackMessages.entranceMsg;
				Serial.print("Pre msg: ");
				Serial.println(msgPtr->c_str());
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				returnState = state + 1;
				state = 100;
				break;
			case 2:
				data[0].speed = 37;
				if(millis() - startTime >= 10000)
					state = 0;
//					state++;
				break;

			case 3:
				data[0].defectAxle = 29;
				msgPtr = &trackMessages.defects[0].detailMsg;
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				data[0].defects.emplace_back(*msg);
				delete msg;
				msgPtr = &trackMessages.defects[0].alertMsg;
				Serial.print("Pre msg: ");
				Serial.println(msgPtr->c_str());
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				returnState = state + 1;
				state = 100;
				break;
			case 4:
				if(millis() - startTime >= 7000)
					state++;
				break;

			case 5:
				data[0].defectAxle = 108;
				msgPtr = &trackMessages.defects[1].detailMsg;
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				data[0].defects.emplace_back(*msg);
				delete msg;
				msgPtr = &trackMessages.defects[1].alertMsg;
				Serial.print("Pre msg: ");
				Serial.println(msgPtr->c_str());
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				returnState = state + 1;
				state = 100;
				break;
			case 6:
				if(millis() - startTime >= 10000)
					state++;
				break;

			case 7:
				data[0].totalAxles = 184;
				msgPtr = &trackMessages.exitDefectMsg;
				Serial.print("Pre msg: ");
				Serial.println(msgPtr->c_str());
				msg = transformMessage(msgPtr, &cfg, &data[0], 0);
				returnState = state + 1;
				state = 100;
				break;
			case 8:
				if(millis() - startTime >= 30000)
					state++;
				break;

			case 9:
				startTime = millis();
				data[0].defects.clear();
				state++;
				break;
			case 10:
				if(millis() - startTime >= 1000)
					state++;
				break;
			case 11:
				Serial.println("Done.");
				printMemoryUsage();
				state = 0;
				break;



			case 100:
				if(parserQueueEmpty())
				{
					startTime = millis();
					printMemoryUsage();
					obj.msg = msg;
					obj.deleteWhenDone = true;
					Serial.print("Sending msg: ");
					Serial.println(obj.msg->c_str());
					parserQueuePush(&obj);
					state = returnState;
				}
				break;

			case 255:
			default:
				startTime = millis();
				state = 0;
				break;
		}
*/

/*
		if(millis() - axleTime >= 1000)
		{
			// 87 * 1000000 * 3600 / 12 / 5280 = 4,943,182
			axleTime = millis();
			Serial.print("Axle Count A: ");
			Serial.println(axleGetCount(0));
			Serial.print("Entrance Delta A: ");
			Serial.println(axleGetEntranceDeltaMicros(0));
			Serial.print("Exit Delta A: ");
			Serial.println(axleGetExitDeltaMicros(0));
			Serial.print("Entrance Speed A: ");
			if(axleGetEntranceDeltaMicros(0) > 0)
				Serial.print((49431820)/(axleGetEntranceDeltaMicros(0))/10.0,1);
			Serial.print('\n');
			Serial.print("Exit Speed A: ");
			if(axleGetExitDeltaMicros(0) > 0)
				Serial.print((49431820)/(axleGetExitDeltaMicros(0))/10.0,1);
			Serial.print('\n');
			Serial.print("Axle Count B: ");
			Serial.println(axleGetCount(1));
			Serial.print("Entrance Delta B: ");
			Serial.println(axleGetEntranceDeltaMicros(1));
			Serial.print("Exit Delta B: ");
			Serial.println(axleGetExitDeltaMicros(1));
			Serial.print("Entrance Speed B: ");
			if(axleGetEntranceDeltaMicros(1) > 0)
				Serial.print((49431820)/(axleGetEntranceDeltaMicros(1))/10.0,1);
			Serial.print('\n');
			Serial.print("Exit Speed B: ");
			if(axleGetExitDeltaMicros(1) > 0)
				Serial.print((49431820)/(axleGetExitDeltaMicros(1))/10.0,1);
			Serial.print('\n');
			Serial.println("---");
		}
*/

		if(sdCardInserted)
		{
			if(1 == gpio_get_level(SDDET))
			{
				// Card removed
				if(millis() > sdDetectTime + 500)  //  Need 500ms of continuous removal
				{
					Serial.println("SD Card Removed");
					sdCardInserted = false;
				}
			}
			else
			{
				sdDetectTime = millis();
			}
		}
		else
		{
			if(0 == gpio_get_level(SDDET))
			{
				// Card inserted
				if(millis() > sdDetectTime + 500)  //  Need 500ms of continuous insertion
				{
					Serial.println("SD Card Inserted");
					sdCardInserted = true;
				}
			}
			else
			{
				sdDetectTime = millis();
			}
		}


		if(restart)
		{
			restart = false;
			Serial.print("\n*** Restarting ***\n\n");

			//  Terminate the parser first, then the audio, due to queue dependencies
			parserTerminate();
			audioTerminate();
			axleTerminate();

			vocabDelete();
			sfxDelete();
			
			// FIXME: Clean up menus

			SD.end();

			break;	// Restart the loop() function
		}

	}

}
