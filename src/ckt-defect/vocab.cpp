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


#include <SD.h>
#include "esp_task_wdt.h"
#include "vocab.h"

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

	File vocabDir = SD.open("/vocab");
	if (!vocabDir || !vocabDir.isDirectory())
	{
		return;
	}

	File entry = vocabDir.openNextFile();
	while (entry)
	{
		if (entry.isDirectory())
		{
			std::string entryName = entry.name();
			
			// Strip leading directory path if present
			size_t lastSlash = entryName.find_last_of("/\\");
			if (lastSlash != std::string::npos)
			{
				entryName = entryName.substr(lastSlash + 1);
			}

			if (!entryName.empty())
			{
				vocabsAvailable.push_back(entryName);
			}
		}
		entry.close();
		entry = vocabDir.openNextFile();
	}
	
	vocabDir.close();
}

struct WavData {
	uint32_t sampleRate;
	uint32_t wavDataSize;
	size_t dataStartPosition;
};

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

bool loadExternalVocab(const std::string& vocabSelected, const std::vector<std::string>& words)
{
	std::string vocabDirName = "/vocab/" + vocabSelected;
	Serial.print("Attempting to load external vocabulary from: ");
	Serial.println(vocabDirName.c_str());

	File vocabDir = SD.open(vocabDirName.c_str());
	if (!vocabDir || !vocabDir.isDirectory())
	{
		Serial.print("! Failed to open vocabulary directory or it is not a directory: ");
		Serial.println(vocabDirName.c_str());
		if (vocabDir)
		{
			vocabDir.close();
		}
		return false;
	}

	uint32_t loadedCount = 0;
	File entry = vocabDir.openNextFile();

	while (entry)
	{
		esp_task_wdt_reset();

		if (entry.isDirectory())
		{
			Serial.print("  Skipping subdirectory: ");
			Serial.println(entry.name());
		}
		else
		{
			std::string entryName = entry.name();

			// Strip leading path components if returned by the SD library
			size_t lastSlash = entryName.find_last_of("/\\");
			if (lastSlash != std::string::npos)
			{
				entryName = entryName.substr(lastSlash + 1);
			}

			// Validate .wav extension (case-insensitive)
			if (entryName.length() >= 5 && 
			    0 == strcasecmp(entryName.substr(entryName.length() - 4).c_str(), ".wav"))
			{
				// Extract basename (word name without directory or extension)
				std::string wordName = entryName.substr(0, entryName.length() - 4);
				
				Serial.print("Processing ");
				Serial.println(wordName.c_str());

				// Verify word exists in the unique words vector
				auto it = std::find(words.begin(), words.end(), wordName);
				if (it != words.end())
				{
					WavData wavData;

					// Validate WAV format standard properties
					if (validateWavFile(&entry, &wavData))
					{
						// Enforce 16kHz sample rate requirement
						if (wavData.sampleRate == 16000)
						{
							// Form full path relative to SD root (excluding leading slash since SdSound adds it)
							// Path format passed: "vocab/<vocabSelected>/<wordName>.wav"
							std::string relativePath = "vocab/" + vocabSelected + "/" + wordName + ".wav";

							Serial.print("+ Adding external WAV: /");
							Serial.print(relativePath.c_str());
							Serial.print(" [Word: '");
							Serial.print(wordName.c_str());
							Serial.println("']");

							// SdSound constructor formats:
							// - fileName  -> "/" + relativePath (Full path for SD.open)
							// - soundName -> wordName (Basename stripped at '.' for lookup)
							vocab.push_back(new SdSound(relativePath, wavData.wavDataSize, wavData.dataStartPosition, wavData.sampleRate));
							loadedCount++;
						}
						else
						{
							Serial.print("! Sample rate mismatch (must be 16000 Hz): ");
							Serial.print(entryName.c_str());
							Serial.print(" (Found: ");
							Serial.print(wavData.sampleRate);
							Serial.println(")");
						}
					}
				}
				else
				{
					Serial.print("  Skipping unneeded word file: ");
					Serial.println(entryName.c_str());
				}
			}
			else
			{
				Serial.print("  Ignoring non-WAV file: ");
				Serial.println(entryName.c_str());
			}
		}

		entry.close();
		entry = vocabDir.openNextFile();
	}

	vocabDir.close();

	Serial.print("External vocabulary load finished. Total valid words loaded: ");
	Serial.println(loadedCount);

	return (loadedCount > 0);
}

#include "vocab/include/0.h"
#include "vocab/include/1.h"
#include "vocab/include/2.h"
#include "vocab/include/3.h"
#include "vocab/include/4.h"
#include "vocab/include/5.h"
#include "vocab/include/6.h"
#include "vocab/include/7.h"
#include "vocab/include/8.h"
#include "vocab/include/9.h"
#include "vocab/include/a.h"
#include "vocab/include/alarm.h"
#include "vocab/include/alarms.h"
#include "vocab/include/axle.h"
#include "vocab/include/axles.h"
#include "vocab/include/blocked.h"
#include "vocab/include/defect.h"
#include "vocab/include/defects.h"
#include "vocab/include/degrees.h"
#include "vocab/include/detected.h"
#include "vocab/include/detector.h"
#include "vocab/include/dragging.h"
#include "vocab/include/east.h"
#include "vocab/include/eastbound.h"
#include "vocab/include/eighth.h"
#include "vocab/include/equipment.h"
#include "vocab/include/excessive.h"
#include "vocab/include/failure.h"
#include "vocab/include/fifth.h"
#include "vocab/include/first.h"
#include "vocab/include/fourth.h"
#include "vocab/include/have.h"
#include "vocab/include/high.h"
#include "vocab/include/hot.h"
#include "vocab/include/impact.h"
#include "vocab/include/integrity.h"
#include "vocab/include/journal.h"
#include "vocab/include/left.h"
#include "vocab/include/main.h"
#include "vocab/include/milepost.h"
#include "vocab/include/minus.h"
#include "vocab/include/near.h"
#include "vocab/include/ninth.h"
#include "vocab/include/no.h"
#include "vocab/include/north.h"
#include "vocab/include/northbound.h"
#include "vocab/include/out.h"
#include "vocab/include/point.h"
#include "vocab/include/rail.h"
#include "vocab/include/repeat.h"
#include "vocab/include/right.h"
#include "vocab/include/second.h"
#include "vocab/include/seventh.h"
#include "vocab/include/sixth.h"
#include "vocab/include/slow.h"
#include "vocab/include/south.h"
#include "vocab/include/southbound.h"
#include "vocab/include/speed.h"
#include "vocab/include/temperature.h"
#include "vocab/include/third.h"
#include "vocab/include/total.h"
#include "vocab/include/track.h"
#include "vocab/include/train.h"
#include "vocab/include/west.h"
#include "vocab/include/westbound.h"
#include "vocab/include/wheel.h"
#include "vocab/include/you.h"

void loadInternalVocab(void)
{
	vocab.push_back(new MemSound("0", vocab_0, vocab_0_len, 16000));
	vocab.push_back(new MemSound("1", vocab_1, vocab_1_len, 16000));
	vocab.push_back(new MemSound("2", vocab_2, vocab_2_len, 16000));
	vocab.push_back(new MemSound("3", vocab_3, vocab_3_len, 16000));
	vocab.push_back(new MemSound("4", vocab_4, vocab_4_len, 16000));
	vocab.push_back(new MemSound("5", vocab_5, vocab_5_len, 16000));
	vocab.push_back(new MemSound("6", vocab_6, vocab_6_len, 16000));
	vocab.push_back(new MemSound("7", vocab_7, vocab_7_len, 16000));
	vocab.push_back(new MemSound("8", vocab_8, vocab_8_len, 16000));
	vocab.push_back(new MemSound("9", vocab_9, vocab_9_len, 16000));
	vocab.push_back(new MemSound("a", vocab_a, vocab_a_len, 16000));
	vocab.push_back(new MemSound("alarm", vocab_alarm, vocab_alarm_len, 16000));
	vocab.push_back(new MemSound("alarms", vocab_alarms, vocab_alarms_len, 16000));
	vocab.push_back(new MemSound("axle", vocab_axle, vocab_axle_len, 16000));
	vocab.push_back(new MemSound("axles", vocab_axles, vocab_axles_len, 16000));
	vocab.push_back(new MemSound("blocked", vocab_blocked, vocab_blocked_len, 16000));
	vocab.push_back(new MemSound("defect", vocab_defect, vocab_defect_len, 16000));
	vocab.push_back(new MemSound("defects", vocab_defects, vocab_defects_len, 16000));
	vocab.push_back(new MemSound("degrees", vocab_degrees, vocab_degrees_len, 16000));
	vocab.push_back(new MemSound("detected", vocab_detected, vocab_detected_len, 16000));
	vocab.push_back(new MemSound("detector", vocab_detector, vocab_detector_len, 16000));
	vocab.push_back(new MemSound("dragging", vocab_dragging, vocab_dragging_len, 16000));
	vocab.push_back(new MemSound("east", vocab_east, vocab_east_len, 16000));
	vocab.push_back(new MemSound("eastbound", vocab_eastbound, vocab_eastbound_len, 16000));
	vocab.push_back(new MemSound("eighth", vocab_eighth, vocab_eighth_len, 16000));
	vocab.push_back(new MemSound("equipment", vocab_equipment, vocab_equipment_len, 16000));
	vocab.push_back(new MemSound("excessive", vocab_excessive, vocab_excessive_len, 16000));
	vocab.push_back(new MemSound("failure", vocab_failure, vocab_failure_len, 16000));
	vocab.push_back(new MemSound("fifth", vocab_fifth, vocab_fifth_len, 16000));
	vocab.push_back(new MemSound("first", vocab_first, vocab_first_len, 16000));
	vocab.push_back(new MemSound("fourth", vocab_fourth, vocab_fourth_len, 16000));
	vocab.push_back(new MemSound("have", vocab_have, vocab_have_len, 16000));
	vocab.push_back(new MemSound("high", vocab_high, vocab_high_len, 16000));
	vocab.push_back(new MemSound("hot", vocab_hot, vocab_hot_len, 16000));
	vocab.push_back(new MemSound("impact", vocab_impact, vocab_impact_len, 16000));
	vocab.push_back(new MemSound("integrity", vocab_integrity, vocab_integrity_len, 16000));
	vocab.push_back(new MemSound("journal", vocab_journal, vocab_journal_len, 16000));
	vocab.push_back(new MemSound("left", vocab_left, vocab_left_len, 16000));
	vocab.push_back(new MemSound("main", vocab_main, vocab_main_len, 16000));
	vocab.push_back(new MemSound("milepost", vocab_milepost, vocab_milepost_len, 16000));
	vocab.push_back(new MemSound("minus", vocab_minus, vocab_minus_len, 16000));
	vocab.push_back(new MemSound("near", vocab_near, vocab_near_len, 16000));
	vocab.push_back(new MemSound("ninth", vocab_ninth, vocab_ninth_len, 16000));
	vocab.push_back(new MemSound("no", vocab_no, vocab_no_len, 16000));
	vocab.push_back(new MemSound("north", vocab_north, vocab_north_len, 16000));
	vocab.push_back(new MemSound("northbound", vocab_northbound, vocab_northbound_len, 16000));
	vocab.push_back(new MemSound("out", vocab_out, vocab_out_len, 16000));
	vocab.push_back(new MemSound("point", vocab_point, vocab_point_len, 16000));
	vocab.push_back(new MemSound("rail", vocab_rail, vocab_rail_len, 16000));
	vocab.push_back(new MemSound("repeat", vocab_repeat, vocab_repeat_len, 16000));
	vocab.push_back(new MemSound("right", vocab_right, vocab_right_len, 16000));
	vocab.push_back(new MemSound("second", vocab_second, vocab_second_len, 16000));
	vocab.push_back(new MemSound("seventh", vocab_seventh, vocab_seventh_len, 16000));
	vocab.push_back(new MemSound("sixth", vocab_sixth, vocab_sixth_len, 16000));
	vocab.push_back(new MemSound("slow", vocab_slow, vocab_slow_len, 16000));
	vocab.push_back(new MemSound("south", vocab_south, vocab_south_len, 16000));
	vocab.push_back(new MemSound("southbound", vocab_southbound, vocab_southbound_len, 16000));
	vocab.push_back(new MemSound("speed", vocab_speed, vocab_speed_len, 16000));
	vocab.push_back(new MemSound("temperature", vocab_temperature, vocab_temperature_len, 16000));
	vocab.push_back(new MemSound("third", vocab_third, vocab_third_len, 16000));
	vocab.push_back(new MemSound("total", vocab_total, vocab_total_len, 16000));
	vocab.push_back(new MemSound("track", vocab_track, vocab_track_len, 16000));
	vocab.push_back(new MemSound("train", vocab_train, vocab_train_len, 16000));
	vocab.push_back(new MemSound("west", vocab_west, vocab_west_len, 16000));
	vocab.push_back(new MemSound("westbound", vocab_westbound, vocab_westbound_len, 16000));
	vocab.push_back(new MemSound("wheel", vocab_wheel, vocab_wheel_len, 16000));
	vocab.push_back(new MemSound("you", vocab_you, vocab_you_len, 16000));
}
