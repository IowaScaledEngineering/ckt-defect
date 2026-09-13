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
#include "ff.h"
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <math.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

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

// Custom line reader replacing f_gets to avoid linker dependency on FF_USE_STRFUNC
static bool fatFsReadLine(FIL *fp, char *buffer, size_t maxLen)
{
	size_t idx = 0;
	UINT bytesRead = 0;
	char c;

	while (idx < maxLen - 1)
	{
		if (f_read(fp, &c, 1, &bytesRead) != FR_OK || bytesRead == 0)
		{
			break; // EOF or error
		}

		if (c == '\r')
		{
			continue; // Skip carriage returns
		}

		if (c == '\n')
		{
			break; // End of line
		}

		buffer[idx++] = c;
	}

	buffer[idx] = '\0';
	return (idx > 0 || bytesRead > 0);
}

hw_timer_t * timer = NULL;
volatile bool timerTick = false;

void IRAM_ATTR tickTimer(void)
{
	timerTick = true;
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
	DetectorConfiguration cfg;
	MessageBundle trackMessages;
	DataBundle data[NUM_TRACKS];
	
	DisplayLcd *lcd = new DisplayLcd();
	bool displayPresent = true;  // Start assuming it's there so we don't take the refresh delay initially
	bool oldDisplayPresent = true;
	bool lcdRefresh = false;

	bool sdCardPresent = false;
	bool configFilePresent = false;
	cfg.externalVocabPresent = false;

	TemperatureManager temperatureMgr(&cfg);
	unsigned long temperatureUpdateTime = millis();

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
	updateRailNames(&cfg);

	audioSetVolumeStep(cfg.volumeStep);
	audioSetNoiseStep(cfg.noiseStep);
	audioSetPopcornStep(cfg.popcornStep);
	lcd->setBrightness(cfg.lcdBrightness);
	audioSetVolumeUpCoef(10);
	audioSetVolumeDownCoef(8);
	audioSetPttDelay(100 * cfg.pttDelay);


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
	auto menus = createAppMenu(cfg, lcd, data, trackMessages);

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

	// Wait for serial to initialize
	while(millis() < 3000)
	{
		esp_task_wdt_reset();
	}

	// Direct FatFs Mount via ESP-IDF VFS SDSPI Driver
	sdmmc_card_t *card = NULL;
	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = SPI2_HOST;

	// Ensure pins are correctly defined and reset
	gpio_reset_pin((gpio_num_t)SDMOSI);
	gpio_reset_pin((gpio_num_t)SDMISO);
	gpio_reset_pin((gpio_num_t)SDCLK);
	gpio_reset_pin((gpio_num_t)SDCS);

	spi_bus_config_t bus_cfg = {};
	bus_cfg.mosi_io_num = (gpio_num_t)SDMOSI;
	bus_cfg.miso_io_num = (gpio_num_t)SDMISO;
	bus_cfg.sclk_io_num = (gpio_num_t)SDCLK;
	bus_cfg.quadwp_io_num = -1;
	bus_cfg.quadhd_io_num = -1;
	bus_cfg.max_transfer_sz = 0; // Let driver calculate default max transfer size

	// Initialize SPI bus using SPI_DMA_CH_AUTO
	esp_err_t bus_err = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
	if (bus_err != ESP_OK && bus_err != ESP_ERR_INVALID_STATE)
	{
	    Serial.printf("SPI Bus Init Failed: %s (0x%X)\n", esp_err_to_name(bus_err), bus_err);
	}

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = (gpio_num_t)SDCS;
	slot_config.host_id = SPI2_HOST;

	esp_vfs_fat_mount_config_t mount_config = {};
	mount_config.format_if_mount_failed = false;
	mount_config.max_files = 5;
	mount_config.allocation_unit_size = 16 * 1024;

	// Declare a global or static FATFS object
	static FATFS sd_fatfs;

	// ... inside loop() where SD card is mounted ...
	esp_err_t mount_err = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
	if (mount_err == ESP_OK)
	{
	    // Mount volume "0:" explicitly for direct FatFs (f_*) functions
	    FRESULT fr = f_mount(&sd_fatfs, "0:", 1);
	    if (fr == FR_OK) {
		sdCardPresent = true;
		Serial.println("SD Card mounted successfully for FatFs\n");
	    } else {
		Serial.printf("f_mount failed with code: %d\n", fr);
	    }
	}

/*
	// Read config file using direct FatFs
	if(sdCardPresent)
	{
		FIL configFile;
		if (f_open(&configFile, "config.txt", FA_READ) == FR_OK)
		{
			configFilePresent = true;
			char lineBuffer[256];

			while (fatFsReadLine(&configFile, lineBuffer, sizeof(lineBuffer)))
			{
				char keyStr[128];
				char valueStr[128];
				bool kvFound = configKeyValueSplit(keyStr, sizeof(keyStr), valueStr, sizeof(valueStr), lineBuffer);
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
			f_close(&configFile);
		}

		esp_task_wdt_reset();
	}
*/

	// If no config file, set defaults
	if(!configFilePresent)
	{
		setDefaultMessages(trackMessages, cfg);
	}

	std::vector<std::string> words = getUniqueWords(trackMessages);

	// Check for external vocab
	if(sdCardPresent)
	{
		// Needed regardless for selection menu
		vocabFindAvailable(cfg.vocabsAvailable);
		cfg.externalVocabPresent = loadExternalVocab(cfg.vocabSelected, words);
	}
	
	// If no SD vocab, load the internal ones
	if(!cfg.externalVocabPresent)
	{
		loadInternalVocab();
	}


	// Sort defect messages by rate
	sort(trackMessages.defects.begin(), trackMessages.defects.end(), [](DefectMessage a, DefectMessage b) {
		return a.axleRate > b.axleRate; // returns true if 'a' should come before 'b'
		});



	//  Initialize temperature after config is loaded, but before menu starts running
	temperatureMgr.begin();



	menuManager.begin();
	menuManager.process();  // Call once here to get things going


	Serial.println("ISE Defect Detector");
	Serial.print("Version: ");
	Serial.println(VERSION_STRING);
	Serial.print("Git Rev: ");
	Serial.println(GIT_REV);

	printMemoryUsage();
	printNVSStats();

	// Print configuration values
	Serial.print('\n');
	printMessages(&trackMessages);
	Serial.print('\n');
	printConfiguration(&cfg);
	Serial.print('\n');

	Serial.println("--- Unique Words Found ---");
	for (const auto& word : words)
	{
		Serial.println(word.c_str());
	}
	Serial.println("--------------------------\n");

	Serial.println("------ Vocabs Found ------");
	for (const auto& v : cfg.vocabsAvailable)
	{
		Serial.println(v.c_str());
	}
	Serial.println("--------------------------");

	Serial.print("Vocab Selected: ");
	Serial.println(cfg.vocabSelected.c_str());
	Serial.print("External Vocab: ");
	Serial.println(cfg.externalVocabPresent);
	Serial.print('\n');

	esp_task_wdt_reset();

	audioInit();
	parserInit();
	axleInit();

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
				data[i].speedFloat = (cfg.speedScale / 10.0) * SPEED_COEF / (axleGetExitDeltaMicros(i));
				data[i].speed = data[i].speedFloat + 0.5;
			}

			if((axleGetDirection(i) > 0) && (0 == data[i].direction))
			{
				// Save direction
				data[i].direction = axleGetDirection(i);
			}

			if((axleGetEntranceDeltaMicros(i) > 0) && cfg.speedTypeEnter && (0 == data[i].speed))
			{
				// Calculate entrance speed
				data[i].speedFloat = (cfg.speedScale / 10.0) * SPEED_COEF / (axleGetEntranceDeltaMicros(i));
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
				case '~':
					Serial.print("Clearing preferences...");
					resetConfiguration();
					break;
			}
		}




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

	}

}