/*************************************************************************
Title:    Defect Detector
Authors:  Michael Petersen <railfan@drgw.net>
File:     ckt-defect.c
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2026 Michael Petersen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <SPI.h>
#include <SD.h>
#include <vector>
#include <algorithm>
#include <string>
#include <strings.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

#include "common.h"
#include "configuration.h"
#include "io.h"
#include "sound.h"
#include "audio.h"
#include "messages.h"

// 3 sec watchdog 
#define TWDT_TIMEOUT_MS    3000

extern QueueHandle_t wavSoundQueue;

bool restart = false;

struct WavData {
	uint32_t sampleRate;
	uint32_t wavDataSize;
	size_t dataStartPosition;
};



uint8_t debounce(uint8_t debouncedState, uint8_t newInputs)
{
	static uint8_t clock_A = 0, clock_B = 0;
	uint8_t delta = newInputs ^ debouncedState; // Find all of the changes
	uint8_t changes;

	clock_A ^= clock_B; //Increment the counters
	clock_B  = ~clock_B;

	clock_A &= delta; //Reset the counters if no changes
	clock_B &= delta; //were detected.

	changes = ~((~delta) | clock_A | clock_B);
	debouncedState ^= changes;
	return(debouncedState);
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


void printMemoryUsage(void)
{
	Serial.printf("\n[SYS]: stack: %u heap: %u\n\n", uxTaskGetStackHighWaterMark(NULL), xPortGetFreeHeapSize());
}


hw_timer_t * timer = NULL;
volatile bool timerTick = false;

void IRAM_ATTR tickTimer(void)
{
	timerTick = true;
}

void setup()
{
	// Open serial communications and wait for port to open:
	Serial.begin();

	pinMode((gpio_num_t)SDDET, INPUT_PULLUP);

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


void loop()
{
	File rootDir;
	unsigned long sdDetectTime = 0;
	
	DetectorConfiguration cfg;

	MessageBundle trackMessages[2];  // Declare two bundles of messages, one for each track

	esp_task_wdt_reset();

	Serial.println("ISE Defect Detector");

	Serial.print("Version: ");
	Serial.println(VERSION_STRING);

	Serial.print("Git Rev: ");
	Serial.println(GIT_REV, HEX);

	// Read NVM configuration
	loadConfiguration(&cfg);
	audioSetVolumeStep(cfg.volumeStep);

	printMemoryUsage();

	// Set some defaults
	audioSetVolumeUpCoef(10);
	audioSetVolumeDownCoef(8);

	// Check for config file and load data from it if present
	// FIXME

	trackMessages[0].entranceMsg = "Equipment Defect Detector";
	trackMessages[0].defects.emplace_back("defect 1 alertMsg", "defect 1 detailMsg", "defect 1 summaryMsg", 500);
	trackMessages[0].defects.emplace_back("defect 2 alertMsg", "defect 2 detailMsg", "defect 2 summaryMsg", 100);
	trackMessages[0].defects.emplace_back("defect 3 alertMsg", "defect 3 detailMsg", "defect 3 summaryMsg", 300);
	trackMessages[0].integrityMsg = "This is an integrity message";

	Serial.println(trackMessages[0].entranceMsg.c_str());
	Serial.println(trackMessages[0].integrityMsg.c_str());
	for(uint32_t i=0; i<trackMessages[0].defects.size(); i++)
	{
		Serial.println(trackMessages[0].defects[i].summaryMsg.c_str());
	}

	sort(trackMessages[0].defects.begin(), trackMessages[0].defects.end(), [](DefectMessage a, DefectMessage b) {
		return a.probability < b.probability; // returns true if 'a' should come before 'b'
		});
	
	for(uint32_t i=0; i<trackMessages[0].defects.size(); i++)
	{
		Serial.println(trackMessages[0].defects[i].summaryMsg.c_str());
	}



// FIXME *********************************
	bool ambientMode = false;
	std::vector<Sound *> ambientSounds;
	WavSound wavSound;
// ***************************************




	
	SPIClass vspi = SPIClass(FSPI);
	vspi.begin(SDCLK, SDMISO, SDMOSI, SDCS);
	pinMode(SDCS, OUTPUT);

	// Check SD card
	if(SD.begin(SDCS, vspi))
	{
		// Check for and read config file
		File f = SD.open("/config.txt");
		if (f)
		{
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

		if((rootDir = SD.open("/ambient")))
		{
			// Ambient mode, find WAV files
			Serial.println("\nFound ambient directory");
			findWavFiles(&rootDir, "ambient/", &ambientSounds);
			rootDir.close();
			if(ambientSounds.size() > 0)
			{
				// Only set Ambient mode if sounds are found
				ambientMode = true;
			}
		}

		esp_task_wdt_reset();
	}

	// Print configuration values
	printConfiguration(&cfg);

	Serial.println("");

	esp_task_wdt_reset();


// FIXME *********************************
	if(ambientMode)
	{
		Serial.print("Using SD card sounds (");
		Serial.print(ambientSounds.size());
		Serial.println(")");
	}
	else
	{
		Serial.println("No valid sounds on SD card!");
		// Blink blue / orange
		while(1)
		{
			if(0 == gpio_get_level((gpio_num_t)SDDET))
			{
				// Card inserted
				if(millis() > sdDetectTime + 500)  //  Need 500ms of continuous insertion
				{
					Serial.println("SD Card Inserted");
					restart = true;
					break;
				}
			}
			else
			{
				sdDetectTime = millis();
			}
		}
	}
// FIXME *********************************



	audioInit();

	while(1)
	{
		esp_task_wdt_reset();

		// Do things on 10ms interval
		if(timerTick)
		{
			timerTick = false;
			audioProcessVolume();
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
					resetConfiguration(&cfg);
					break;
			}
		}

		// FIXME Send some test audio
		if(0 == uxQueueMessagesWaiting(wavSoundQueue))
		{
			printMemoryUsage();

			audioUnmute();

			uint32_t sampleNum = random(0, ambientSounds.size());
			Serial.print("Queueing... ");
			Serial.println(sampleNum);
			wavSound.wav = ambientSounds[sampleNum];
			wavSound.seamlessPlay = false;
			xQueueSend(wavSoundQueue, &wavSound, portMAX_DELAY);
		}

		if(1 == gpio_get_level((gpio_num_t)SDDET))
		{
			// Card removed
			if(millis() > sdDetectTime + 500)  //  Need 500ms of continuous removal
			{
				Serial.println("SD Card Removed");
				restart = true;
			}
		}
		else
		{
			sdDetectTime = millis();
		}


		if(restart)
		{
			restart = false;
			Serial.print("\n*** Restarting ***\n\n");

			audioTerminate();

// FIXME Send some test audio
			for(uint32_t i=0; i<ambientSounds.size(); i++)
			{
				delete ambientSounds[i];
			}
			ambientSounds.clear();
// FIXME Send some test audio

			SD.end();

			break;	// Restart the loop() function
		}

	}

}
