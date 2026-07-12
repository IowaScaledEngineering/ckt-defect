#include "menu-custom.h"
#include "common.h"
#include "temperature.h"
#include "parser.h"
#include <format>

void MenuHome::onEnter()
{
	Menu::onEnter(); // Call the base implementation to clear display, clear button repeat states, etc.
	backlightDelayStartTime = millis();
	if(disp->getBacklight())
	{
		// Backlight already on
		delayBacklightOff = true;
	}

	byte bulb[8] = {
	  0b01110,
	  0b10001,
	  0b10001,
	  0b01010,
	  0b01110,
	  0b01110,
	  0b00100,
	  0b00000,
	};
	
	disp->createCustomChar(0, bulb);
}

void MenuHome::renderHomeUI(const std::string& statusText, bool showLightButton)
{
	TemperatureManager* tempMgr = TemperatureManager::getInstance();
	std::string tmpString1;
	std::string tmpString2;

	// 1. Milepost
	disp->gotoxy(0, 0);
	tmpString1 = "Milepost ";
	tmpString1 += intToString(cfg.milepost, 4, 1);
	tmpString2 = centerString(tmpString1, 20);
	disp->print(tmpString2.c_str());

	// 2. Status Text
	disp->gotoxy(0, 1);
	tmpString2 = centerString(statusText, 20);
	disp->print(tmpString2.c_str());

	// 3. Temperature
	disp->gotoxy(0, 2);
	tmpString1 = intToString(tempMgr->getTemperature() + 0.5, 3, 0);
	tmpString1 += 0xDF;
	tmpString1 += cfg.temperatureUnitsF ? 'F' : 'C';
	tmpString2 = centerString(tmpString1, 20);
	disp->print(tmpString2.c_str());

	// 4. Light button
	disp->gotoxy(0, 3);
	if (showLightButton)
	{
		disp->print((char)0);  // bulb icon
		disp->print(disp->getBacklight() ? "OFF" : "ON ");
	}
	else
	{
		disp->print("    "); // Clear out the lower-left corner if needed
	}
}

MenuEvent MenuHome::update()
{
	std::string tmpString;
	
	if(backlightState)
	{
		disp->backlightOn();
	}
	else
	{
		if(!delayBacklightOff || (millis() - backlightDelayStartTime > 3000))
		{
			disp->backlightOff();
			backlightState = false;
			delayBacklightOff = false;
		}
	}

	switch(state)
	{
		case MenuHomeState::STANDBY:
			renderHomeUI("STANDBY", true);

			if(data[0].active || data[1].active)
			{
				state = MenuHomeState::ACTIVE;
			}

			break;

		case MenuHomeState::ACTIVE:
			backlightState = true;
			renderHomeUI("ACTIVE", false);

			dispString = getDisplayMessage();
			if(!dispString.empty())
			{
				state = MenuHomeState::MESSAGE;
			}
			else if(!data[0].active && !data[1].active)
			{
				backlightState = false;
				backlightDelayStartTime = millis();
				delayBacklightOff = true;
				state = MenuHomeState::STANDBY;
			}

			break;

		case MenuHomeState::MESSAGE:
			if(!dispString.empty())
			{
				size_t startPos = 0;
				uint8_t lineCount = 0;

				// We process up to 3 lines max to avoid spilling onto row 3 (which holds the "MENU" prompt)
				while (startPos < dispString.length() && lineCount < 3)
				{
					size_t nextNewline = dispString.find('\n', startPos);
					std::string currentLine;
					
					if (nextNewline == std::string::npos)
					{
						currentLine = dispString.substr(startPos);
						startPos = dispString.length(); // Break loop next cycle
					}
					else
					{
						currentLine = dispString.substr(startPos, nextNewline - startPos);
						startPos = nextNewline + 1;
					}

					// Format individual line: center and truncate to width of 20
					tmpString = centerString(currentLine, 20);
					tmpString.resize(20);

					// Print directly to the designated cursor row
					disp->gotoxy(0, lineCount);
					disp->print(tmpString.c_str());

					lineCount++;
				}

				// Clear out any remaining rows (up to row 2) if the message was short
				// so that residual data from an older screen state isn't left behind
				while (lineCount < 3)
				{
					disp->gotoxy(0, lineCount);
					disp->print("                    "); // 20 spaces
					lineCount++;
				}
			}
			else
			{
				if(!data[0].active && !data[1].active)
				{
					waitStartTime = millis();
					state = MenuHomeState::WAIT;
				}
			}
			dispString = getDisplayMessage();  // Fetch for next time around

			break;

		case MenuHomeState::WAIT:
			if(millis() - waitStartTime >= 5000)  // FIXME: make timeout programmable
			{
				backlightState = false;
				backlightDelayStartTime = millis();
				delayBacklightOff = true;
				state = MenuHomeState::STANDBY;
			}

			break;
	}


	// Draw menu button
	disp->gotoxy(16,3);
	disp->print("MENU");

	DisplayEvent ev;
	if(getMenuInputEvent(&ev))
	{
		if(ev.type == DisplayEventType::KEY_PRESS)
		{
			switch(ev.keyNum)
			{
				case 1: // Toggle Backlight
					if(MenuHomeState::STANDBY == state)
					{
						if(delayBacklightOff)
							backlightState = false;
						else
							backlightState = !backlightState;
						delayBacklightOff = false;  // Force immediate change
					}
					break;

				case 4: // Menu
					return MenuEvent::FORWARD;
			}
		}
		else if(ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}
	return MenuEvent::NOOP;
}


void MenuVolume::onEnter()
{
	Menu::onEnter(); // Call the base implementation to clear display, clear button repeat states, etc.
	originalVal = getValue();

	// Hardcoded map clamp: input 0-30 converted to percentage 0-150%
	uint32_t rawVal = std::clamp<uint32_t>(getValue(), 0U, 30U);
	
	// Every 1 raw unit = 5% (since 20 units = 100%)
	currentVal = (int32_t)(rawVal * 5);

	// Initialize custom block pieces (5x8 pixels)
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

MenuEvent MenuVolume::update()
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

	// Double check boundaries (0% to 150%)
	uint32_t percentage = std::clamp<int32_t>(currentVal, 0, 150);

	// --- Visual Bar Render Math (Line 1) ---
	// Visual rendering caps out cleanly at 100%
	uint32_t visualPercentage = std::min<uint32_t>(percentage, 100U);
	
	// 15 slots * 5 columns per slot = 75 discrete steps total
	uint32_t totalSteps = (visualPercentage * 75) / 100;
	uint32_t fullBlocks = totalSteps / 5;
	uint32_t partialBlockWidth = totalSteps % 5;
	uint32_t emptyBlocks = 15 - fullBlocks - (partialBlockWidth > 0 ? 1 : 0);

	disp->gotoxy(1, 1);
	disp->print("[");

	for(uint32_t i = 0; i < fullBlocks; i++)
	{
		disp->print((char)0x05);
	}
	if(partialBlockWidth > 0)
	{
		disp->print((char)partialBlockWidth);
	}
	for(uint32_t i = 0; i < emptyBlocks; i++)
	{
		disp->print(' ');
	}
	disp->print("]");

	// --- Custom Value Text Formatting (Line 2) ---
	std::string textStr;
	if (percentage > 100)
	{
		textStr = std::format("! {}% !", percentage);
	}
	else
	{
		textStr = std::format("{}%", percentage);
	}
	
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
				case 1: // Decrement by hardcoded 5%
					currentVal -= 5;
					if(currentVal < 0)
						currentVal = 0;
					handleButtonPress(1);
					if(realTime)
					{
						setValue((uint32_t)(currentVal / 5));
					}
					break;

				case 2: // Increment by hardcoded 5%
					currentVal += 5;
					if(currentVal > 150)
						currentVal = 150;
					handleButtonPress(2);
					if(realTime)
					{
						setValue((uint32_t)(currentVal / 5));
					}
					break;

				case 3: // Save Action
					applyChange((uint32_t)(percentage / 5));
					return MenuEvent::BACK;

				case 4: // Cancel Action
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
