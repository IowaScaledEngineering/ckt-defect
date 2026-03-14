/*************************************************************************
Title:    Sound Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     sound.h
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

#pragma once

#include <SD.h>
#include <string>

#define FILE_BUFFER_SIZE 2048

class Sound
{
	protected:
		size_t dataSize;
		size_t byteCount;
		uint32_t sampleRate;
		std::string soundName;

	public:
		virtual ~Sound();
		virtual void open(void);
		virtual int16_t getNextSample(void);
		virtual void close(void);
		size_t available(void);
		uint32_t getSampleRate(void);
		std::string getName(void) const;
};

class SdSound : public Sound
{
	std::string fileName;
	size_t dataOffset;
	File wavFile;
	uint8_t fileBuffer[FILE_BUFFER_SIZE];
	size_t fileBufferLength;  // might be less than actual buffer size
	size_t fileBufferPosition;
	int16_t sampleValue;

	public:
		SdSound(const std::string& fname, size_t numBytes, size_t offset, uint16_t sr);
		~SdSound();
		void open(void);
		size_t fileBufferAvailable(void);
		int16_t getNextSample(void);
		void close(void);
};

class MemSound : public Sound
{
	const uint8_t *dataPtr;
	uint8_t soundNum;
	int16_t sampleValue;

	public:
		MemSound(const std::string& name, const uint8_t *sound, size_t numBytes, uint16_t sr);
		~MemSound();
		void open(void);
		int16_t getNextSample(void);
		void close(void);
};

class ToneSound : public Sound
{
	int16_t sampleValue;
	bool invert;

	public:
		ToneSound(size_t samples, uint16_t sr);
		~ToneSound();
		void open(void);
		int16_t getNextSample(void);
		void close(void);
};
