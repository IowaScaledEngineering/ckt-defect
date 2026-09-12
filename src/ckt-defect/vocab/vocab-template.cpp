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

void vocabFindAvailable(DetectorConfiguration& cfg)
{
	cfg.vocabsAvailable.clear();

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
				cfg.vocabsAvailable.push_back(entryName);
			}
		}
		entry.close();
		entry = vocabDir.openNextFile();
	}
	
	vocabDir.close();
}

