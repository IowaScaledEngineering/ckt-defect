#include "menu.h"
#include <algorithm>
#include <format>

// --- Centralized Base Class Input Helper Mechanisms ---

bool Menu::getMenuInputEvent(DisplayEvent *ev)
{
	bool gotEvent = disp->getEvent(ev);

	// Handle button-hold timing evaluation if no fresh event occurred
	if(!gotEvent && getMillis != nullptr && lastButtonNum != 0)
	{
		uint32_t currentDelay = isHolding ? holdDelayMs : initialHoldDelayMs;
		if((getMillis() - lastButtonPressTime) >= currentDelay)
		{
			ev->type = DisplayEventType::KEY_PRESS;
			ev->keyNum = lastButtonNum;
			gotEvent = true;
			isHolding = true;
			lastButtonPressTime = getMillis(); // Reset interval for subsequent continuous repeats
		}
	}
	return gotEvent;
}

void Menu::handleButtonPress(int keyNum)
{
	if(getMillis != nullptr) 
	{ 
		lastButtonNum = keyNum; 
		lastButtonPressTime = getMillis(); 
	}
}

void Menu::handleButtonRelease(int keyNum)
{
	if(keyNum == lastButtonNum)
	{
		lastButtonNum = 0;
		isHolding = false;
	}
}

uint32_t Menu::getValue()
{
	if(getFunc32) return getFunc32();
	if(valPtr32) return *valPtr32;
	if(getFuncBool) return getFuncBool() ? 1 : 0;
	if(valPtrBool) return *valPtrBool ? 1 : 0;
	return 0;
}

void Menu::setValue(uint32_t value)
{
	if(setFunc32) setFunc32(value);
	else if(valPtr32) *valPtr32 = value;
	else if(setFuncBool) setFuncBool(value != 0);
	else if(valPtrBool) *valPtrBool = (value != 0);
}

void Menu::applyChange(uint32_t value)
{
	lastButtonNum = 0;
	isHolding = false;
	setValue(value);
	if(saveCallback)
	{
		saveCallback();
	}
}

void Menu::cancel()
{
	lastButtonNum = 0;
	isHolding = false;
	if(realTime)
	{
		setValue(originalVal);
	}
}

// --- Menu UI Subclass Implementations ---

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
			firstMenu = (int32_t)totalVisible - 3; // Pin window to the bottom if we reach the end
	}

	// Render the 3 visible menu slots
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

			// Overwrite any residual artifact characters on the row
			int remaining = 18 - (int)name.length();
			if(remaining > 0)
			{
				disp->print(std::string(remaining, ' '));
			}
		}
		else
		{
			// Blank line if there are no more menu choices
			disp->print("                    ");
		}
	}

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			if(ev.keyNum == 1 && selector > 0)
			{
				selector--;
				handleButtonPress(1);
			}
			else if(ev.keyNum == 2 && selector < totalVisible - 1)
			{
				selector++;
				handleButtonPress(2);
			}
			else if(ev.keyNum == 3)
			{
				lastButtonNum = 0;
				isHolding = false;
				return MenuEvent::FORWARD;
			}
			else if(ev.keyNum == 4)
			{
				lastButtonNum = 0;
				isHolding = false;
				return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}

void MenuDigitThumbwheel::onEnter()
{
	Menu::onEnter();
	uint32_t initVal = getValue();
	originalVal = initVal;
	modStr = std::format("{:0{}d}", initVal, iDigits + fDigits);
	curDigit = 0;
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

	std::string displayStr = modStr;
	bool leading = true;
	int keepIndex = (int)modStr.length() - (int)fDigits - 1;

	if(suppressLeadingZeros)
	{
		for(int i = 0; i < keepIndex; i++)
		{
			if(leading && displayStr[i] == '0')
			{
				displayStr[i] = ' ';
			}
			else
			{
				leading = false;
			}
		}
	}

	disp->gotoxy(1, 1);
	disp->print("[");
	for(uint32_t i = 0; i < displayStr.length(); i++)
	{
		if(fDigits > 0 && i == (displayStr.length() - fDigits))
			disp->print('.');
		disp->print(displayStr[i]);
	}
	disp->print("]");

	disp->gotoxy(2, 2);
	for(uint32_t i = 0; i < modStr.length(); i++)
	{
		if(fDigits > 0 && i == (modStr.length() - fDigits))
			disp->print(' ');
		disp->print(i == curDigit ? '^' : ' ');
	}

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			if(ev.keyNum == 1)
			{
				modStr[curDigit] = ((modStr[curDigit] - '0' + 1) % 10) + '0';
				handleButtonPress(1);
				if(realTime)
				{
					setValue(std::stoul(modStr));
				}
			}
			else if(ev.keyNum == 2)
			{
				curDigit = (curDigit + 1) % modStr.length();
				handleButtonPress(2);
			}
			else if(ev.keyNum == 3)
			{
				applyChange(std::stoul(modStr));
				return MenuEvent::BACK;
			}
			else if(ev.keyNum == 4)
			{
				cancel();
				return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}

void MenuNumberDial::onEnter()
{
	Menu::onEnter();
	uint32_t initVal = getValue();
	currentVal = std::clamp(initVal, minVal, maxVal);
	originalVal = currentVal;
}

MenuEvent MenuNumberDial::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	disp->gotoxy(1, 3);
	disp->print("--");
	disp->gotoxy(6, 3);
	disp->print("++");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	// 1. Determine number of digits for the current value
	std::string valStr = std::to_string(currentVal);
	int currentDigits = (int)valStr.length();

	// 2. Calculate padding: total field size minus current digits
	int padCount = std::max(0, maxDigits - currentDigits);
	std::string padding(padCount, ' ');

	// 3. Render
	disp->gotoxy(2, 1);
	disp->print(padding);
	disp->print(valStr);
	if(units.length())
	{
		disp->print(" " + units);
	}

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			switch(ev.keyNum)
			{
				case 1:
					if(currentVal > minVal)
						currentVal--;
					handleButtonPress(1);
					if(realTime)
					{
						setValue(currentVal);
					}
					break;
				case 2:
					if(currentVal < maxVal)
						currentVal++;
					handleButtonPress(2);
					if(realTime)
					{
						setValue(currentVal);
					}
					break;
				case 3:
					applyChange(currentVal);
					return MenuEvent::BACK;
				case 4:
					cancel();
					return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}

void MenuBoolSelector::onEnter()
{
	Menu::onEnter();
	currentVal = (getValue() != 0);
	originalVal = currentVal;
}

MenuEvent MenuBoolSelector::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	disp->gotoxy(1, 3);
	disp->print(btn1Name);
	disp->gotoxy(6, 3);
	disp->print(btn2Name);
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

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

	DisplayEvent ev;
	if(getMenuInputEvent(&ev)) // Now robustly tracks hold configurations consistently
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			switch(ev.keyNum)
			{
				case 1:
					currentVal = true;
					handleButtonPress(1);
					if(realTime)
					{
						setValue(1);
					}
					break;
				case 2:
					currentVal = false;
					handleButtonPress(2);
					if(realTime)
					{
						setValue(0);
					}
					break;
				case 3:
					applyChange(currentVal ? 1 : 0);
					return MenuEvent::BACK;
				case 4:
					cancel();
					return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}

void MenuOptionSelector::onEnter()
{
	Menu::onEnter();
	currentVal = getValue();
	originalVal = currentVal;

	// If the value is beyond the end of the list, go to the last item
	if(currentVal >= options.size())
	{
		currentVal = options.empty() ? 0 : (uint32_t)options.size() - 1;
	}
	topIndex = 0;
}

MenuEvent MenuOptionSelector::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	disp->gotoxy(1, 3);
	disp->print("UP");
	disp->gotoxy(6, 3);
	disp->print("DOWN");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	// Scroll management to keep focus visible inside the available 2 lines
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

	// Render the 2 available rows for options
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

			// Available space for the option string is 16 characters (20 columns total - 4 used by prefix
			// marker)
			int remaining = 16 - (int)optionText.length();
			if(remaining > 0)
			{
				disp->print(std::string(remaining, ' '));
			}
		}
		else
		{
			// Clear the line if there are no items to show
			disp->print("                    ");
		}
	}

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			switch(ev.keyNum)
			{
				case 1:
					if(currentVal > 0)
						currentVal--;
					handleButtonPress(1);
					if(realTime)
					{
						setValue(currentVal);
					}
					break;
				case 2:
					if(currentVal < options.size() - 1)
						currentVal++;
					handleButtonPress(2);
					if(realTime)
					{
						setValue(currentVal);
					}
					break;
				case 3:
					applyChange(currentVal);
					return MenuEvent::BACK;
				case 4:
					cancel();
					return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}

void MenuPercentageBar::onEnter()
{
	Menu::onEnter();
	originalVal = getValue();

	// 1. Convert raw value to percentage with proper mathematical rounding (+ maxVal/2)
	uint32_t rawVal = std::clamp<uint32_t>(getValue(), 0U, maxVal);
	uint32_t initialPct = (uint32_t)((((uint64_t)rawVal * 100) + (maxVal / 2)) / maxVal);

	if(stepVal > 0)
	{
		uint32_t remainder = initialPct % stepVal;
		if(remainder >= (stepVal + 1) / 2)
		{
			initialPct += (stepVal - remainder); // Round up
		}
		else
		{
			initialPct -= remainder; // Round down
		}
	}

	// Final safety clamp to percentage boundaries
	currentVal = (int32_t)std::clamp<uint32_t>(initialPct, 0U, 100U);

	// Initialize the 5 custom fractional-width column bar pieces (5x8 pixels)
	uint8_t bar1[8] = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000};
	uint8_t bar2[8] = {0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b00000};
	uint8_t bar3[8] = {0b11100, 0b11100, 0b11100, 0b11100, 0b11100, 0b11100, 0b11100, 0b00000};
	uint8_t bar4[8] = {0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b00000};
	uint8_t bar5[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000};

	disp->createCustomChar(1, bar1);
	disp->createCustomChar(2, bar2);
	disp->createCustomChar(3, bar3);
	disp->createCustomChar(4, bar4);
	disp->createCustomChar(5, bar5);
}

MenuEvent MenuPercentageBar::update()
{
	disp->backlightOn();
	disp->gotoxy(0, 0);
	disp->print(menuName);

	disp->gotoxy(1, 3);
	disp->print("--");
	disp->gotoxy(6, 3);
	disp->print("++");
	disp->gotoxy(11, 3);
	disp->print("SAVE");
	disp->gotoxy(16, 3);
	disp->print("CNCL");

	// Dynamic calculation of percentage bounds based on inputs
	uint32_t percentage = std::clamp<int32_t>(currentVal, 0, 100);

	// --- Visual Bar Render Math (Line 1) ---
	// 15 display slots * 5 internal columns per slot = 75 discrete steps total.
	// We map the 0-100 percentage scale smoothly down to the 0-75 bar step scale.
	uint32_t totalSteps = (percentage * 75) / 100;
	uint32_t fullBlocks = totalSteps / 5;
	uint32_t partialBlockWidth = totalSteps % 5;
	uint32_t emptyBlocks = 15 - fullBlocks - (partialBlockWidth > 0 ? 1 : 0);

	disp->gotoxy(1, 1);
	disp->print("[");

	// 1. Draw solid complete 5/5 block segments
	for(uint32_t i = 0; i < fullBlocks; i++)
	{
		disp->print((char)0x05);
	}

	// 2. Draw fractional remainder block segments if applicable
	if(partialBlockWidth > 0)
	{
		disp->print((char)partialBlockWidth);
	}

	// 3. Draw remaining space padding to keep layout stable
	for(uint32_t i = 0; i < emptyBlocks; i++)
	{
		disp->print(' ');
	}
	disp->print("]");

	// Center aligned value text on row 2
	std::string textStr = std::format("{}%", percentage);
	int padLeft = (20 - (int)textStr.length()) / 2;

	disp->gotoxy(0, 2);
	disp->print(std::string(padLeft, ' ') + textStr + std::string(20 - padLeft - textStr.length(), ' '));

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			switch(ev.keyNum)
			{
				case 1:
					currentVal -= (int32_t)stepVal;
					if(currentVal < 0)
						currentVal = 0;
					handleButtonPress(1);
					if(realTime)
					{
						setValue((uint32_t)(((uint64_t)currentVal * maxVal) / 100));
					}
					break;

				case 2:
					currentVal += (int32_t)stepVal;
					if(currentVal > 100)
						currentVal = 100;
					handleButtonPress(2);
					if(realTime)
					{
						setValue((uint32_t)(((uint64_t)currentVal * maxVal) / 100));
					}
					break;

				case 3:
					applyChange((uint32_t)(((uint64_t)percentage * maxVal) / 100));
					return MenuEvent::BACK;

				case 4:
					cancel();
					return MenuEvent::BACK;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}