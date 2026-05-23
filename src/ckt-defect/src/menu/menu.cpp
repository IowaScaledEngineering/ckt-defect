#include "menu.h"
#include <algorithm>
#include <format>

MenuEvent MenuListSelector::update()
{
	// Filter visible children
	std::vector<std::shared_ptr<Menu>> visibleChildren;
	for(auto &child : children)
	{
		if(child->isVisible())
		{
			visibleChildren.push_back(child);
		}
	}

	if(visibleChildren.empty())
		return MenuEvent::BACK;

	// Clamp selector if the number of visible items changed
	if(selector >= visibleChildren.size())
	{
		selector = visibleChildren.empty() ? 0 : (uint32_t)visibleChildren.size() - 1;
	}

	disp->backlightOn();
	disp->gotoxy(1, 3);
	disp->print("UP");
	disp->gotoxy(5, 3);
	disp->print("DOWN");
	disp->gotoxy(11, 3);
	disp->print("SLCT");
	disp->gotoxy(16, 3);
	disp->print("BACK");

	// Calculate scrolling window to keep selector in the middle (row 1)
	int32_t firstMenu;
	uint32_t totalVisible = (uint32_t)visibleChildren.size();

	if(totalVisible <= 3)
	{
		firstMenu = 0;
	}
	else
	{
		// Attempt to put the selector at row index 1 (the middle)
		firstMenu = (int32_t)selector - 1;
		if(firstMenu < 0)
			firstMenu = 0;
		else if(firstMenu > (int32_t)totalVisible - 3)
			firstMenu = (int32_t)totalVisible - 3;
	}

	// Render the 3-row visible window
	for(uint32_t i = 0; i < 3; i++)
	{
		uint32_t menuIdx = (uint32_t)firstMenu + i;
		disp->gotoxy(0, i);

		if(menuIdx < totalVisible)
		{
			disp->print(selector == menuIdx ? '>' : ' ');
			disp->gotoxy(2, i);
			
			// Smart clear: print name, then explicitly blank only the remaining slots
			std::string name = visibleChildren[menuIdx]->getName();
			disp->print(name);
			
			// Available space for the item text is 18 characters (20 total columns - 2 used by prefix cursor)
			int remaining = 18 - (int)name.length();
			if (remaining > 0)
			{
				std::string remainderClear(remaining, ' ');
				disp->print(remainderClear);
			}
		}
		else
		{
			// Clear the line if there are fewer than 3 items to show
			disp->print("                    ");
		}
	}

	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		if(ev.keyNum == 1 && selector > 0)
			selector--;
		else if(ev.keyNum == 2 && selector < totalVisible - 1)
			selector++;
		else if(ev.keyNum == 3)
		{
			return MenuEvent::FORWARD;
		}
		else if(ev.keyNum == 4)
		{
			return MenuEvent::BACK;
		}
	}
	return MenuEvent::NOOP;
}

MenuEvent MenuDigitThumbwheel::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);
	disp->gotoxy(1, 3);
	disp->print("++");
	disp->gotoxy(6, 3);
	disp->print(">>");
	disp->gotoxy(10, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	if(state == 0)
	{
		// modStr stores raw digits for logic
		modStr = std::format("{:0{}d}", *valPtr, iDigits + fDigits);
		curDigit = 0;
		state = 1;
	}

	// 1. Prepare display string with leading zero suppression
	std::string displayStr = modStr;
	bool leading = true;
	// We want to keep at least the last digit of the integer part (index: modStr.length() - fDigits - 1)
	int keepIndex = (int)modStr.length() - (int)fDigits - 1;

	if (suppressLeadingZeros)
	{
		for(int i = 0; i < keepIndex; i++)
		{
			if(leading && displayStr[i] == '0')
			{
				displayStr[i] = ' '; // Replace leading zero with space
			}
			else
			{
				leading = false; // Found a non-zero, stop suppressing
			}
		}
	}

	// 2. Display logic
	disp->gotoxy(1, 1);
	disp->print("[");
	for(uint32_t i = 0; i < displayStr.length(); i++)
	{
		if(fDigits > 0 && i == (displayStr.length() - fDigits))
			disp->print('.');
		disp->print(displayStr[i]);
	}
	disp->print("]");

	// 3. Cursor logic (stays aligned with digits)
	disp->gotoxy(2, 2);
	for(uint32_t i = 0; i < modStr.length(); i++)
	{
		if(fDigits > 0 && i == (modStr.length() - fDigits))
			disp->print(' ');
		disp->print(i == curDigit ? '^' : ' ');
	}

	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		if(ev.keyNum == 1) // Increment
		{
			modStr[curDigit] = ((modStr[curDigit] - '0' + 1) % 10) + '0';
		}
		else if(ev.keyNum == 2) // Move Cursor
		{
			curDigit = (curDigit + 1) % modStr.length();
		}
		else if(ev.keyNum == 3) // Save
		{
			*valPtr = std::stoul(modStr);
			state = 0;
			return MenuEvent::BACK;
		}
		else if(ev.keyNum == 4) // Back
		{
			state = 0;
			return MenuEvent::BACK;
		}
	}
	return MenuEvent::NOOP;
}

MenuEvent MenuNumberDial::update()
{
	disp->backlightOn();

	disp->gotoxy(0, 0);
	disp->print(menuName);

	// Draw Button Labels at bottom row (row 3)
	disp->gotoxy(1, 3);
	disp->print("--");
	disp->gotoxy(6, 3);
	disp->print("++");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	if(state == 0)
	{
		// Sync with current variable and clamp within range
		currentVal = std::clamp(*valPtr, minVal, maxVal);

		state = 1;
	}

	// 1. Determine number of digits for the current value
	std::string valStr = std::to_string(currentVal);
	int currentDigits = (int)valStr.length();

	// 2. Calculate padding: total field size minus current digits
	int padCount = std::max(0, maxDigits - currentDigits);
	std::string padding(padCount, ' ');

	// 3. Render
	disp->gotoxy(2, 1);
	disp->print(padding); // Print the spaces FIRST for right-justification
	disp->print(valStr);  // Then the number
	if(units.length())
	{
		disp->print(" " + units);
	}

	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		switch(ev.keyNum)
		{
			case 1: // --
				if(currentVal > minVal)
					currentVal--;
				break;
			case 2: // ++
				if(currentVal < maxVal)
					currentVal++;
				break;
			case 3: // SAVE - Update original pointer and exit
				*valPtr = currentVal;
				state = 0;
				return MenuEvent::BACK;
			case 4: // CNCL - Exit without updating
				state = 0;
				return MenuEvent::BACK;
		}
	}

	return MenuEvent::NOOP;
}

MenuEvent MenuBoolSelector::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	// Draw Button Labels at bottom row (row 3)
	disp->gotoxy(1, 3);
	disp->print(btn1Name);
	disp->gotoxy(6, 3);
	disp->print(btn2Name);
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	if(state == 0)
	{
		// Sync with current variable state
		currentVal = *valPtr;

		state = 1;
	}

	// 1. Render Option 1 (associated with true) on line 1
	disp->gotoxy(0, 1);
	disp->print(currentVal ? "[*] " : "[ ] ");
	disp->print(opt1Name);
	disp->print("    "); // Clear any trailing artifacts

	// 2. Render Option 2 (associated with false) on line 2
	disp->gotoxy(0, 2);
	disp->print(!currentVal ? "[*] " : "[ ] ");
	disp->print(opt2Name);
	disp->print("    "); // Clear any trailing artifacts

	// 3. Process Input Events
	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		switch(ev.keyNum)
		{
			case 1: // Select Option 1 (true)
				currentVal = true;
				break;
			case 2: // Select Option 2 (false)
				currentVal = false;
				break;
			case 3: // SAVE - Commit change to pointer and back out
				*valPtr = currentVal;
				state = 0;
				return MenuEvent::BACK;
			case 4: // CNCL - Revert/Ignore change and back out
				state = 0;
				return MenuEvent::BACK;
		}
	}

	return MenuEvent::NOOP;
}

MenuEvent MenuOptionSelector::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	// Layout buttons: UP (1), DOWN (2), SAVE (3), CNCL (4)
	disp->gotoxy(1, 3);
	disp->print("UP");
	disp->gotoxy(6, 3);
	disp->print("DOWN");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	if(state == 0)
	{
		// Sync with current variable state
		currentVal = *valPtr;

		// If the pointer is beyond the end of the list, go to the last item
		if(currentVal >= options.size())
		{
			currentVal = options.empty() ? 0 : (uint32_t)options.size() - 1;
		}

		topIndex = 0;
		state = 1;
	}

	// 1. Calculate the 2-row scrolling window
	if(currentVal < topIndex)
	{
		topIndex = currentVal;
	}
	else if(currentVal >= topIndex + 2)
	{
		topIndex = currentVal - 1;
	}

	// Clamp window bounds safely
	if(options.size() <= 2)
	{
		topIndex = 0;
	}
	else if(topIndex > options.size() - 2)
	{
		topIndex = (uint32_t)options.size() - 2;
	}

	// 2. Render the 2-row visible window (Row 1 and Row 2)
	for(uint32_t i = 0; i < 2; i++)
	{
		uint32_t optIdx = topIndex + i;
		disp->gotoxy(0, i + 1);

		if(optIdx < options.size())
		{
			disp->print(currentVal == optIdx ? "[*] " : "[ ] ");
			disp->gotoxy(4, i + 1);
			
			// Smart clear: print selection option string, then pad only remaining space
			std::string optionText = options[optIdx];
			disp->print(optionText);
			
			// Available space for the option string is 16 characters (20 columns total - 4 used by prefix marker)
			int remaining = 16 - (int)optionText.length();
			if (remaining > 0)
			{
				std::string remainderClear(remaining, ' ');
				disp->print(remainderClear);
			}
		}
		else
		{
			// Clear the line if there are no items to show
			disp->print("                    ");
		}
	}

	// 3. Process Input Events
	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		switch(ev.keyNum)
		{
			case 1: // UP
				if(currentVal > 0)
					currentVal--;
				break;
			case 2: // DOWN
				if(currentVal < options.size() - 1)
					currentVal++;
				break;
			case 3: // SAVE
				*valPtr = currentVal;
				state = 0;
				return MenuEvent::BACK;
			case 4: // CNCL
				state = 0;
				return MenuEvent::BACK;
		}
	}

	return MenuEvent::NOOP;
}

MenuEvent MenuBrightness::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName); // "Brightness Level"

	// Draw Button Labels at bottom row (row 3)
	disp->gotoxy(1, 3);
	disp->print("--");
	disp->gotoxy(6, 3);
	disp->print("++");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	if(state == 0)
	{
		// Query current runtime value directly via agnostic data API
		uint8_t rawVal = disp->getBrightness();
		originalVal = rawVal;
		
		// Map the raw value (0-255) to the nearest discrete menu step index (0-15).
		// Uses efficient 8-bit rounding math: step = (rawVal + 8) / 17
		currentLevel = (static_cast<uint16_t>(rawVal) + 8) / 17;
		if (currentLevel > 15) currentLevel = 15;

		// Initialize only standard full block characters
		uint8_t fullBlock[8] = {
			0b11111,
			0b11111,
			0b11111,
			0b11111,
			0b11111,
			0b11111,
			0b11111,
			0b00000
		};
		disp->createCustomChar(2, fullBlock);

		state = 1;
	}

	// --- Visual Bar Mapping Logic ---
	// 15 positions in the bar correspond to levels 1 through 15
	uint8_t fullCharsNeeded = currentLevel;
	uint8_t emptyCharsNeeded = 15 - fullCharsNeeded;

	disp->gotoxy(0,2);
	disp->print('0' + ((currentLevel/100)%10));
	disp->print('0' + ((currentLevel/10)%10));
	disp->print('0' + (currentLevel%10));

	// Render the Bar Graph on Line 1 (Second row)
	disp->gotoxy(1, 1);
	disp->print("["); // Opening Bracket

	// Print Full Blocks
	for(uint8_t i = 0; i < fullCharsNeeded; i++)
	{
		disp->print((char)0x02);
	}
	// Fill remainder of the 15-character block with spaces
	for(uint8_t i = 0; i < emptyCharsNeeded; i++)
	{
		disp->print(' ');
	}

	disp->print("]"); // Closing Bracket

	// --- Input & Value Calculations ---
	DisplayEvent ev;
	if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
	{
		switch(ev.keyNum)
		{
			case 1: // Button 1: Decrease Level
				if(currentLevel > 0)
				{
					currentLevel--;
					// Convert step index directly to agnostic 0-255 domain
					disp->setBrightness(currentLevel * 17);
				}
				break;

			case 2: // Button 2: Increase Level
				if(currentLevel < 15)
				{
					currentLevel++;
					// Convert step index directly to agnostic 0-255 domain
					disp->setBrightness(currentLevel * 17);
				}
				break;

			case 3: // Button 3: SAVE
				disp->setBrightness(currentLevel * 17); // Commit final value
				state = 0;
				return MenuEvent::BACK;

			case 4: // Button 4: CNCL
				disp->setBrightness(originalVal); // Safely restore un-snapped cached entry setting
				state = 0;
				return MenuEvent::BACK;
		}
	}

	return MenuEvent::NOOP;
}
