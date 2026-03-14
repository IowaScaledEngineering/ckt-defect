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
#include "vocab/include/equipment.h"
#include "vocab/include/failure.h"
#include "vocab/include/h.h"
#include "vocab/include/have.h"
#include "vocab/include/high.h"
#include "vocab/include/hot.h"
#include "vocab/include/impact.h"
#include "vocab/include/integrity.h"
#include "vocab/include/journal.h"
#include "vocab/include/m.h"
#include "vocab/include/milepost.h"
#include "vocab/include/near.h"
#include "vocab/include/no.h"
#include "vocab/include/north.h"
#include "vocab/include/out.h"
#include "vocab/include/p.h"
#include "vocab/include/point.h"
#include "vocab/include/repeat.h"
#include "vocab/include/slow.h"
#include "vocab/include/south.h"
#include "vocab/include/speed.h"
#include "vocab/include/temperature.h"
#include "vocab/include/total.h"
#include "vocab/include/track.h"
#include "vocab/include/train.h"
#include "vocab/include/west.h"
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
	vocab.push_back(new MemSound("equipment", vocab_equipment, vocab_equipment_len, 16000));
	vocab.push_back(new MemSound("failure", vocab_failure, vocab_failure_len, 16000));
	vocab.push_back(new MemSound("h", vocab_h, vocab_h_len, 16000));
	vocab.push_back(new MemSound("have", vocab_have, vocab_have_len, 16000));
	vocab.push_back(new MemSound("high", vocab_high, vocab_high_len, 16000));
	vocab.push_back(new MemSound("hot", vocab_hot, vocab_hot_len, 16000));
	vocab.push_back(new MemSound("impact", vocab_impact, vocab_impact_len, 16000));
	vocab.push_back(new MemSound("integrity", vocab_integrity, vocab_integrity_len, 16000));
	vocab.push_back(new MemSound("journal", vocab_journal, vocab_journal_len, 16000));
	vocab.push_back(new MemSound("m", vocab_m, vocab_m_len, 16000));
	vocab.push_back(new MemSound("milepost", vocab_milepost, vocab_milepost_len, 16000));
	vocab.push_back(new MemSound("near", vocab_near, vocab_near_len, 16000));
	vocab.push_back(new MemSound("no", vocab_no, vocab_no_len, 16000));
	vocab.push_back(new MemSound("north", vocab_north, vocab_north_len, 16000));
	vocab.push_back(new MemSound("out", vocab_out, vocab_out_len, 16000));
	vocab.push_back(new MemSound("p", vocab_p, vocab_p_len, 16000));
	vocab.push_back(new MemSound("point", vocab_point, vocab_point_len, 16000));
	vocab.push_back(new MemSound("repeat", vocab_repeat, vocab_repeat_len, 16000));
	vocab.push_back(new MemSound("slow", vocab_slow, vocab_slow_len, 16000));
	vocab.push_back(new MemSound("south", vocab_south, vocab_south_len, 16000));
	vocab.push_back(new MemSound("speed", vocab_speed, vocab_speed_len, 16000));
	vocab.push_back(new MemSound("temperature", vocab_temperature, vocab_temperature_len, 16000));
	vocab.push_back(new MemSound("total", vocab_total, vocab_total_len, 16000));
	vocab.push_back(new MemSound("track", vocab_track, vocab_track_len, 16000));
	vocab.push_back(new MemSound("train", vocab_train, vocab_train_len, 16000));
	vocab.push_back(new MemSound("west", vocab_west, vocab_west_len, 16000));
	vocab.push_back(new MemSound("wheel", vocab_wheel, vocab_wheel_len, 16000));
	vocab.push_back(new MemSound("you", vocab_you, vocab_you_len, 16000));
}
