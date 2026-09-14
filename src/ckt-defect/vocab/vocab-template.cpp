/*************************************************************************
Title:    Defect Detector Vocabulary
Authors:  Michael Petersen <railfan@drgw.net>
File:     vocab.cpp
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


/***************  WARNING  ***************/
/* This file is auto generated           */
/* Edit vocab/vocab-template.cpp instead */
/***************  WARNING  ***************/


#include <algorithm>
#include <cstring>
#include <Arduino.h>
#include "ff.h"
#include "esp_task_wdt.h"
#include "vocab.h"
#include "display-lcd.h"

std::vector<Sound *> vocab;

void vocabDelete(void)
{
	for(uint32_t i=0; i<vocab.size(); i++)
	{
		delete vocab[i];
	}
	vocab.clear();
}

size_t vocabGetSize(void)
{
	return vocab.size();
}

std::string vocabGetName(uint32_t index)
{
	if(index < vocabGetSize())
	{
		return vocab[index]->getName();
	}
	else
	{
		return "";
	}
}

Sound* vocabGetWord(const std::string& word)
{
	auto it = std::find_if(vocab.begin(), vocab.end(),
		[&word](const Sound* obj)
		{
			return obj->getName() == word;
		}
	);

	if (it != vocab.end())
	{
		return *it;
	}
	else
	{
		return NULL;
	}
}

Sound* vocabGetWord(const uint32_t index)
{
	if(index < vocabGetSize())
	{
		return vocab[index];
	}
	else
	{
		return NULL;
	}
}

void vocabFindAvailable(std::vector<std::string>& vocabsAvailable)
{
	vocabsAvailable.clear();

	FF_DIR *dir = (FF_DIR *)malloc(sizeof(FF_DIR));
	FILINFO *fno = (FILINFO *)malloc(sizeof(FILINFO));

	if (!dir || !fno)
	{
		if (dir) free(dir);
		if (fno) free(fno);
		return;
	}

	FRESULT res = f_opendir(dir, "0:vocab");
	if (res != FR_OK)
	{
		res = f_opendir(dir, "vocab");
	}

	if (res != FR_OK)
	{
		free(dir);
		free(fno);
		return;
	}

	for (;;)
	{
		esp_task_wdt_reset();

		if (f_readdir(dir, fno) != FR_OK || fno->fname[0] == 0)
		{
			break;
		}

		if (fno->fattrib & AM_DIR)
		{
			if (strcmp(fno->fname, ".") != 0 && strcmp(fno->fname, "..") != 0)
			{
				vocabsAvailable.push_back(fno->fname);
			}
		}
	}

	f_closedir(dir);
	free(dir);
	free(fno);
}

// Helper function to perform pattern matching for WAV chunk headers using raw FatFs
static bool fatFsFindChunk(FIL *wavFile, const char* chunkId)
{
	char buffer[4];
	UINT bytesRead;
	
	// Scan through file looking for 4-byte chunk identifier
	while (f_read(wavFile, buffer, 4, &bytesRead) == FR_OK && bytesRead == 4)
	{
		esp_task_wdt_reset();

		if (memcmp(buffer, chunkId, 4) == 0)
		{
			return true;
		}
		// Step back 3 bytes to handle non-aligned pattern matches
		f_lseek(wavFile, f_tell(wavFile) - 3);
	}
	return false;
}

bool validateWavFile(FIL *wavFile, const char* fileName, struct WavData *wavData)
{
	size_t fileNameLength = strlen(fileName);
	if(fileNameLength < 5)
		return false;

	const char *extension = &fileName[fileNameLength - 4];
	if(strcasecmp(extension, ".wav"))
	{
		return false;
	}

	UINT bytesRead;
	uint16_t channels;
	uint16_t bitsPerSample;
	uint32_t sampleRate;
	uint32_t wavDataSize;

	f_lseek(wavFile, 0);

	if(!fatFsFindChunk(wavFile, "fmt "))
	{
		Serial.print("! No fmt section: ");
		Serial.println(fileName);
		return false;
	}

	// Seek to number of channels offset (skip 4 bytes chunk size + 2 bytes format tag)
	f_lseek(wavFile, f_tell(wavFile) + 6);
	f_read(wavFile, &channels, 2, &bytesRead);

	if(channels > 1)
	{
		Serial.print("! Not mono: ");
		Serial.println(fileName);
		return false;
	}

	f_read(wavFile, &sampleRate, 4, &bytesRead);
	wavData->sampleRate = sampleRate;

	if((8000 != sampleRate) && (16000 != sampleRate) && (32000 != sampleRate) && (44100 != sampleRate))
	{
		Serial.print("! Incorrect sample rate: ");
		Serial.println(fileName);
		return false;
	}

	// Seek to bits per sample (skip 6 bytes: ByteRate [4], BlockAlign [2])
	f_lseek(wavFile, f_tell(wavFile) + 6);
	f_read(wavFile, &bitsPerSample, 2, &bytesRead);

	if(16 != bitsPerSample)
	{
		Serial.print("! Not 16-bit: ");
		Serial.println(fileName);
		return false;
	}

	if(!fatFsFindChunk(wavFile, "data"))
	{
		Serial.print("! No data section: ");
		Serial.println(fileName);
		return false;
	}

	f_read(wavFile, &wavDataSize, 4, &bytesRead);
	wavData->wavDataSize = wavDataSize;
	wavData->dataStartPosition = f_tell(wavFile);

	return true;
}

bool loadExternalVocab(DisplayLcd *lcd, const std::string& vocabSelected, const std::vector<std::string>& words)
{
	lcd->clear();

	uint32_t totalStartTime = millis();
	std::string vocabDirName = "0:vocab/" + vocabSelected;
	Serial.print("Attempting to load external vocabulary from: ");
	Serial.println(vocabDirName.c_str());

	FF_DIR *dir = (FF_DIR *)malloc(sizeof(FF_DIR));
	FIL *wavFile = (FIL *)malloc(sizeof(FIL));

	if (!dir || !wavFile)
	{
		Serial.println("! Failed to allocate FatFs buffer memory");
		if (dir) free(dir);
		if (wavFile) free(wavFile);
		return false;
	}

	FRESULT res = f_opendir(dir, vocabDirName.c_str());
	if (res != FR_OK)
	{
		vocabDirName = "vocab/" + vocabSelected;
		res = f_opendir(dir, vocabDirName.c_str());
	}

	if (res != FR_OK)
	{
		Serial.print("! Failed to open vocabulary directory: ");
		Serial.print(vocabDirName.c_str());
		Serial.printf(" (FatFs error code: %d)\n", res);
		free(dir);
		free(wavFile);
		return false;
	}
	f_closedir(dir);
	free(dir);

	uint32_t loadedCount = 0;
	uint32_t openTime = 0;
	uint32_t validateTime = 0;
	uint32_t pushTime = 0;

	int totalWords = (int)words.size();
	int width = std::to_string(totalWords).length();
	
	for (const auto& word : words)
	{
		esp_task_wdt_reset();

		std::string fileName = word + ".wav";
		std::string fullPath = vocabDirName + "/" + fileName;

		uint32_t t1 = micros();
		FRESULT fopen_res = f_open(wavFile, fullPath.c_str(), FA_READ);
		openTime += (micros() - t1);

		bool justLoaded = false;
		
		if (fopen_res == FR_OK)
		{
			WavData wavData;
			uint32_t t2 = micros();
			bool isValid = validateWavFile(wavFile, fileName.c_str(), &wavData);
			validateTime += (micros() - t2);

			if (isValid)
			{
				if (wavData.sampleRate == 16000)
				{
					DWORD startCluster = wavFile->obj.sclust;

					Serial.print("+ Adding external WAV: ");
					Serial.println(fullPath.c_str());

					uint32_t t3 = micros();
					vocab.push_back(new SdSound(fullPath, wavData.wavDataSize, wavData.dataStartPosition, wavData.sampleRate, startCluster));
					pushTime += (micros() - t3);
					loadedCount++;
					justLoaded = true;
				}
			}
			f_close(wavFile);
		}

		char szBuf[32];
		snprintf(szBuf, sizeof(szBuf), "Loading WAV %*u/%u", width, (unsigned int)(loadedCount), (unsigned int)totalWords);
		lcd->gotoxy(0, 0);
		lcd->print(szBuf);
		lcd->gotoxy(0, 1);
		if (justLoaded)
		{
			lcd->print(centerString(word, 20));
		}
	}

	free(wavFile);

	lcd->gotoxy(0, 1);
	lcd->print(std::string(20, ' '));
	lcd->gotoxy(0, 2);
	if (loadedCount > 0)
	{
		lcd->print("Using " + vocabSelected);
	}
	else
	{
		lcd->print("Using Internal Vocab");
	}

	uint32_t totalDuration = millis() - totalStartTime;
	Serial.println("--- Profiling Results for loadExternalVocab ---");
	Serial.printf("Total Time: %lu ms\n", totalDuration);
	Serial.printf("File Open Time: %lu us (%lu ms)\n", openTime, openTime / 1000);
	Serial.printf("Validation Time: %lu us (%lu ms)\n", validateTime, validateTime / 1000);
	Serial.printf("Vector Push Time: %lu us (%lu ms)\n", pushTime, pushTime / 1000);
	Serial.print("External vocabulary load finished. Total valid words loaded: ");
	Serial.println(loadedCount);

	return (loadedCount > 0);
}

