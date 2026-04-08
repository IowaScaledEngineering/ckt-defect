/*************************************************************************
Title:    Defect Detector Special Effects
Authors:  Michael Petersen <railfan@drgw.net>
File:     sfx.cpp
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

#include "sfx.h"

std::vector<Sound *> sfx;

void sfxDelete(void)
{
	for(uint32_t i=0; i<sfx.size(); i++)
	{
		delete sfx[i];
	}
	sfx.clear();
}

size_t sfxGetSize(void)
{
	return sfx.size();
}

Sound* sfxGetSound(const std::string& name)
{
	auto it = std::find_if(sfx.begin(), sfx.end(),
		[&name](const Sound* obj)
		{
			return obj->getName() == name;
		}
	);

	if (it != sfx.end())
	{
		return *it;
	}
	else
	{
		return NULL;
	}
}

#include "sfx/PneumaticHornDouble3-16k.h"

void loadSfx(void)
{
	sfx.push_back(new MemSound("horn", sfx_horn, sfx_horn_len, 16000));
}
