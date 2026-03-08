/*************************************************************************
Title:    Sound Functions
Authors:  Michael Petersen <railfan@drgw.net>
File:     sound.cpp
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

#include <SD.h>
#include "sound.h"

Sound::~Sound()
{
}

void Sound::open(void)
{
}

int16_t Sound::getNextSample(void)
{
	return 0;
}

void Sound::close(void)
{
}

size_t Sound::available(void)
{
	if(dataSize > byteCount)
		return(dataSize - byteCount);
	else
		return 0;
}

uint32_t Sound::getSampleRate(void)
{
	return sampleRate;
}





SdSound::SdSound(const std::string& fname, size_t numBytes, size_t offset, uint16_t sr)
{
	fileName = "/" + fname;
	soundName = fname;
	soundName.erase(soundName.find('.'));   // Find the . and remove it and everything after
	dataOffset = offset;
	dataSize = numBytes;
	sampleRate = sr;
}
SdSound::~SdSound()
{
}
void SdSound::open(void)
{
	wavFile = SD.open(fileName.c_str());
	wavFile.seek(dataOffset);
	Serial.print("Open: ");
	Serial.println(soundName.c_str());
	byteCount = 0;
	fileBufferLength = 0;
	fileBufferPosition = 0;
}
size_t SdSound::fileBufferAvailable(void)
{
	if(fileBufferLength > fileBufferPosition)
		return(fileBufferLength - fileBufferPosition);
	else
		return 0;
}
int16_t SdSound::getNextSample(void)
{
	size_t bytesToRead, bytesRead;

	if( (fileBufferAvailable() < 2) && available() )
	{
		// We're out of bytes (or had an odd number for some strange reason)
		// and there's more of the file to grab, so grab it
		if(available() < FILE_BUFFER_SIZE)
		{
			bytesToRead = available();
		}
		else
		{
			bytesToRead = FILE_BUFFER_SIZE;
		}
		bytesRead = wavFile.read(fileBuffer, bytesToRead);
		fileBufferLength = bytesRead;
		fileBufferPosition = 0;
	}

	if(fileBufferAvailable() >= 2)
	{
		// We have at least 2 bytes in the local buffer
		sampleValue = *((int16_t *)(fileBuffer+fileBufferPosition));
		fileBufferPosition += 2;
		byteCount += 2;
		return sampleValue;
	}
	else
	{
		// We got called even though there was nothing to send
		return 0;
	}
}
void SdSound::close(void)
{
	wavFile.close();
}






MemSound::MemSound(const std::string& name, const uint8_t *sound, size_t numBytes, uint16_t sr)
{
	soundName = name;
	dataPtr = sound;
	dataSize = numBytes;
	sampleRate = sr;
}
MemSound::~MemSound()
{
	// No need to free dataPtr since it points to const data
}
void MemSound::open(void)
{
	Serial.print("OpenMem: ");
	Serial.println(soundName.c_str());
	byteCount = 0;
}
int16_t MemSound::getNextSample(void)
{
	if(available() >= 2)
	{
		sampleValue = *((int16_t *)(dataPtr+byteCount));
		byteCount += 2;
		return sampleValue;
	}
	else
	{
		return 0;
	}
}
void MemSound::close(void)
{
	return;
}
