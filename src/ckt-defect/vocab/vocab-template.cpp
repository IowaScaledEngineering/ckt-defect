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

#include "vocab.h"

std::vector<Sound *> vocab;

void vocabReset(void)
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

