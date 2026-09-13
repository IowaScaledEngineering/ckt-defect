/*************************************************************************
Title:    Defect Detector Vocabulary
Authors:  Michael Petersen <railfan@drgw.net>
File:     vocab.h
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

#pragma once

#include "ff.h"
#include "sound.h"
#include "configuration.h"

struct WavData {
	uint32_t sampleRate;
	uint32_t wavDataSize;
	size_t dataStartPosition;
};

void vocabDelete(void);
size_t vocabGetSize(void);
std::string vocabGetName(uint32_t index);
Sound* vocabGetWord(const std::string& word);
Sound* vocabGetWord(const uint32_t index);

bool validateWavFileFatFs(FIL *wavFile, const char* fileName, struct WavData *wavData);
void vocabFindAvailable(std::vector<std::string>& vocabsAvailable);
bool loadExternalVocab(const std::string& vocabSelected, const std::vector<std::string>& words);

void loadInternalVocab(void);

