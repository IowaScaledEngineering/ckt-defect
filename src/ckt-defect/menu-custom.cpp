#include "menu-custom.h"
#include "common.h"
#include "temperature.h"
#include <format>

void MenuHome::onEnter()
{
	Menu::onEnter(); // Call the base implementation to clear display, clear button repeat states, etc.
	menuEnterTime = millis();
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

MenuEvent MenuHome::update()
{
	if(backlightState)
	{
		disp->backlightOn();
	}
	else
	{
		if(!delayBacklightOff || (millis() - menuEnterTime > 3000))
		{
			disp->backlightOff();
			backlightState = false;
			delayBacklightOff = false;
		}
	}
	disp->gotoxy(2, 0);
	disp->print("Milepost ");
	disp->print(intToString(cfg.milepost, 4, 1).c_str());

	if(!data[0].active && !data[1].active)
	{
		disp->gotoxy(0, 1);
		disp->print("      STANDBY       ");
		disp->gotoxy(7, 2);
		TemperatureManager* tempMgr = TemperatureManager::getInstance();
		disp->print(intToString(tempMgr->getTemperature()+0.5, 3, 0).c_str());
		disp->print(0xDF);  // degrees
		disp->print(cfg.temperatureUnitsF ? 'F' : 'C');
	}
	else
	{
		disp->gotoxy(0, 1);
		disp->print("       ACTIVE       ");
		disp->gotoxy(7, 2);
		TemperatureManager* tempMgr = TemperatureManager::getInstance();
		disp->print(intToString(tempMgr->getTemperature()+0.5, 3, 0).c_str());
		disp->print(0xDF);  // degrees
		disp->print(cfg.temperatureUnitsF ? 'F' : 'C');
		backlightState = true;
	}

	disp->gotoxy(0,3);
	disp->print((char)0);  // bulb
	if(disp->getBacklight())
	{
		disp->print("OFF");
	}
	else
	{
		disp->print("ON ");
	}
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
					if(delayBacklightOff)
						backlightState = false;
					else
						backlightState = !backlightState;
					delayBacklightOff = false;  // Force immediate change
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
