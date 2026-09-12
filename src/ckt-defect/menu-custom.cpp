#include "menu-custom.h"
#include "common.h"
#include "temperature.h"
#include "parser.h"
#include "audio.h"
#include "vocab.h"
#include <format>

void restartWithDelay(uint32_t delay_ms, Display* disp)
{
	// Display reset message and clear buttons if display reference is valid
	if (disp)
	{
		disp->gotoxy(0, 1);
		disp->print("                    ");
		disp->gotoxy(0, 2);
		disp->print(centerString("Restarting...", 20).c_str());
		disp->gotoxy(0, 3);
		disp->print("                    ");
	}

	// Non-blocking wait while yielding to watchdog/other tasks
	vTaskDelay(pdMS_TO_TICKS(delay_ms));

	// Trigger system restart
	esp_restart();
}

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

void MenuHome::renderHomeUI(const std::string& statusText, bool showLightButton, bool showRepeatButton)
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

	// 5. Repeat button prompt (ONLY shown if message exists and timeout has not expired)
	disp->gotoxy(6, 3);
	bool repeatValid = !data->lastSpokenMsg.empty();
	if (cfg.msgRepeatTimeout > 0 && (millis() - data->lastSpokenMsgTime) >= (cfg.msgRepeatTimeout * 1000U))
	{
		repeatValid = false;
	}

	if (showRepeatButton && repeatValid)
	{
		disp->print("RPT ");
	}
	else
	{
		disp->print("    ");
	}
}

void MenuHome::renderMessage(const std::string& message)
{
	if(message.empty())
		return;

	std::string tmpString;
	size_t startPos = 0;
	uint8_t lineCount = 0;

	// We process up to 3 lines max to avoid spilling onto row 3 (which holds the "MENU" prompt)
	while (startPos < message.length() && lineCount < 3)
	{
		size_t nextNewline = message.find('\n', startPos);
		std::string currentLine;
		
		if (nextNewline == std::string::npos)
		{
			currentLine = message.substr(startPos);
			startPos = message.length(); // Break loop next cycle
		}
		else
		{
			currentLine = message.substr(startPos, nextNewline - startPos);
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

MenuEvent MenuHome::update()
{
	std::string tmpString;
	bool active = data[0].active || data[1].active;

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
			renderHomeUI("STANDBY", true, true); // Allow RPT in STANDBY (if within timeout)

			if(active)
			{
				state = MenuHomeState::ACTIVE;
			}

			break;

		case MenuHomeState::ACTIVE:
			backlightState = true;
			renderHomeUI("ACTIVE", false, false); // Suppress RPT in ACTIVE

			dispString = getDisplayMessage();
			if(!dispString.empty())
			{
				state = MenuHomeState::MESSAGE;
			}
			else if(!active)
			{
				if (parserQueueEmpty() && audioQueueEmpty())
				{
					backlightState = false;
					backlightDelayStartTime = millis();
					delayBacklightOff = true;
					state = MenuHomeState::STANDBY;
				}
			}

			break;

		case MenuHomeState::MESSAGE:
			if(!dispString.empty())
			{
				renderMessage(dispString);
				lastDisplayedMessage = dispString;
			}
			else
			{
				renderMessage(lastDisplayedMessage);
				if(!active)
				{
					if (parserQueueEmpty() && audioQueueEmpty())
					{
						waitStartTime = millis();
						state = MenuHomeState::WAIT;
					}
				}
			}
			dispString = getDisplayMessage();  // Fetch for next time around

			break;

		case MenuHomeState::WAIT:
			renderMessage(lastDisplayedMessage);

			// Draw RPT button prompt at coords (6, 3) for button 2 during WAIT
			// Timeout does not apply during WAIT; only check if message exists
			disp->gotoxy(6, 3);
			if (!data->lastSpokenMsg.empty())
			{
				disp->print("RPT ");
			}
			else
			{
				disp->print("    ");
			}

			if( (millis() - waitStartTime) >= (cfg.summaryDisplayTime * 1000) )
			{
				backlightState = false;
				backlightDelayStartTime = millis();
				delayBacklightOff = true;
				data->lastSpokenMsgTime = millis(); // Set repeat timeout timestamp upon entering STANDBY
				state = MenuHomeState::STANDBY;
			}

			break;
		
		default:
			// In case we get lost...
			state = MenuHomeState::STANDBY;
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
				case 1: // Toggle Backlight (STANDBY)
					if(MenuHomeState::STANDBY == state)
					{
						if(delayBacklightOff)
							backlightState = false;
						else
							backlightState = !backlightState;
						delayBacklightOff = false;  // Force immediate change
					}
					break;

				case 2: // Repeat Message (STANDBY or WAIT)
					if(MenuHomeState::STANDBY == state || MenuHomeState::WAIT == state)
					{
						bool repeatValid = !data->lastSpokenMsg.empty();

						// Apply timeout check ONLY when in STANDBY mode
						if(MenuHomeState::STANDBY == state)
						{
							if(cfg.msgRepeatTimeout > 0 && (millis() - data->lastSpokenMsgTime) >= (cfg.msgRepeatTimeout * 1000U))
							{
								repeatValid = false;
							}
						}

						if(repeatValid)
						{
							ParserObject* obj = new ParserObject();
							obj->msg = data->lastSpokenMsg;
							parserQueuePush(obj);

							waitStartTime = millis(); // Reset wait timer on repeat press
							data->lastSpokenMsgTime = millis(); // Reset repeat timeout timer
						}
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

	// Calculate maximum raw units allowed based on dynamic settings
	uint32_t effectiveMaxPercent = allowOver ? maxPercent : 100U;
	uint32_t maxRaw = effectiveMaxPercent / stepSize;

	// Clamp the raw input value to valid range
	uint32_t rawVal = std::clamp<uint32_t>(getValue(), 0U, maxRaw);
	
	// Convert raw units to percentage
	currentVal = (int32_t)(rawVal * stepSize);

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
	disp->gotoxy(16, 3);
	disp->print("BACK");

	// Double check boundaries based on configuration
	uint32_t limitPercent = allowOver ? maxPercent : 100U;
	uint32_t percentage = std::clamp<int32_t>(currentVal, 0, limitPercent);

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
				case 1: // Decrement by stepSize
					currentVal -= stepSize;
					if(currentVal < 0)
						currentVal = 0;
					handleButtonPress(1);
					setValue((uint32_t)(currentVal / stepSize));
					break;

				case 2: // Increment by stepSize
					currentVal += stepSize;
					if(currentVal > (int32_t)limitPercent)
						currentVal = limitPercent;
					handleButtonPress(2);
					setValue((uint32_t)(currentVal / stepSize));
					break;

				case 4: // Back Action (Applies save changes & triggers callback)
					applyChange((uint32_t)(percentage / stepSize));
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

void MenuVocabTest::onEnter()
{
	Menu::onEnter(); // Clear display and reset button states
	currentIndex = 0; // Reset vector position when entering
}

MenuEvent MenuVocabTest::update()
{
	disp->backlightOn();

	// Line 0 (Row 0): Menu Name
	disp->gotoxy(0, 0);
	disp->print(menuName);

	// Line 1 (Row 1): Current Sound Name
	disp->gotoxy(0, 1);
	if (vocabGetSize() > 0)
	{
		std::string name = vocabGetName(currentIndex);
		disp->print(centerString(name, 20).c_str());
	}
	else
	{
		disp->print(centerString("< EMPTY >", 20).c_str());
	}

	// Line 3 (Row 3): Button Labels
	disp->gotoxy(0, 3);
	disp->print("PREV");
	disp->gotoxy(5, 3);
	disp->print("NEXT");
	disp->gotoxy(11, 3);
	disp->print("PLAY");
	disp->gotoxy(16, 3);
	disp->print("BACK");

	DisplayEvent ev;
	if (getMenuInputEvent(&ev))
	{
		if (ev.type == DisplayEventType::KEY_PRESS)
		{
			switch (ev.keyNum)
			{
				case 1: // Button 1: Previous item (wrap around)
					handleButtonPress(1);
					if (vocabGetSize() > 0)
					{
						if (currentIndex == 0)
							currentIndex = vocabGetSize() - 1;
						else
							currentIndex--;
					}
					break;

				case 2: // Button 2: Next item (wrap around)
					handleButtonPress(2);
					if (vocabGetSize() > 0)
					{
						currentIndex = (currentIndex + 1) % vocabGetSize();
					}
					break;

				case 3: // Button 3: PLAY current sound
					handleButtonPress(3);
					if (vocabGetSize() > 0)
					{
						WavSound wavSound;
						wavSound.wav = vocabGetWord(currentIndex);
						wavSound.seamlessPlay = false;
						audioQueuePush(&wavSound);
					}
					break;

				case 4: // Button 4: BACK
					return MenuEvent::BACK;
			}
		}
		else if (ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}

	return MenuEvent::NOOP;
}

void MenuReset::onEnter()
{
	Menu::onEnter(); // Clear display and reset button states
	pressCount = 0;
}

MenuEvent MenuReset::update()
{
	disp->backlightOn();

	// Line 0 (Row 0): Menu Name
	disp->gotoxy(0, 0);
	disp->print(menuName);

	// Line 1 (Row 1): Instruction prompt centered
	disp->gotoxy(0, 1);
	disp->print("Press left button 5");
	disp->gotoxy(0, 2);
	disp->print("times to reset");

	// Line 3 (Row 3): Count indicator on leftmost button and BACK
	disp->gotoxy(2, 3);
	std::string countStr = std::to_string(5 - pressCount);
	disp->print(countStr.c_str());

	disp->gotoxy(16, 3);
	disp->print("BACK");

	DisplayEvent ev;
	if (getMenuInputEvent(&ev))
	{
		if (ev.type == DisplayEventType::KEY_PRESS)
		{
			switch (ev.keyNum)
			{
				case 1: // Leftmost Button
					handleButtonPress(1);
					pressCount++;
					if (pressCount >= 5)
					{
						// FIXME: Force to non-SD card mode?
						resetConfiguration();
						loadConfiguration(&cfg);
						saveConfiguration(&cfg);
						restartWithDelay(2000, disp);
					}
					break;

				case 4: // Button 4: BACK
					return MenuEvent::BACK;
			}
		}
		else if (ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}

	return MenuEvent::NOOP;
}

void MenuVocabSelect::onEnter()
{
	Menu::onEnter();

	// Build options list starting with Internal ("")
	options.clear();
	options.push_back(""); // Internal
	options.insert(options.end(), cfg.vocabsAvailable.begin(), cfg.vocabsAvailable.end());

	// Find initial index matching vocabInUse
	currentVal = 0;
	for (size_t i = 0; i < options.size(); i++)
	{
		if (options[i] == cfg.vocabInUse)
		{
			currentVal = static_cast<uint32_t>(i);
			break;
		}
	}
	originalVal = currentVal;
	topIndex = 0;
}

MenuEvent MenuVocabSelect::update()
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

	// Scroll management for 2 visible rows
	if (currentVal < topIndex)
	{
		topIndex = currentVal;
	}
	else if (currentVal >= topIndex + 2)
	{
		topIndex = currentVal - 1;
	}

	if (options.size() <= 2)
	{
		topIndex = 0;
	}
	else if (topIndex > options.size() - 2)
	{
		topIndex = static_cast<uint32_t>(options.size() - 2);
	}

	// Render choices across lines 1 and 2
	for (uint32_t i = 0; i < 2; i++)
	{
		uint32_t optIdx = topIndex + i;
		disp->gotoxy(0, i + 1);

		if (optIdx < options.size())
		{
			disp->print(currentVal == optIdx ? "[*] " : "[ ] ");
			disp->gotoxy(4, i + 1);

			// Get option name or default to "Internal"
			std::string optionText = options[optIdx].empty() ? "Internal" : options[optIdx];
			optionText = optionText.substr(0, 16);

			disp->print(optionText);
			
			// Pad remaining characters on the line with spaces
			int remaining = 16 - static_cast<int>(optionText.length());
			if (remaining > 0)
			{
				disp->print(std::string(remaining, ' '));
			}
		}
		else
		{
			disp->print("                    ");
		}
	}
	
	DisplayEvent ev;
	if (getMenuInputEvent(&ev))
	{
		if (ev.type == DisplayEventType::KEY_PRESS)
		{
			switch (ev.keyNum)
			{
				case 1: // UP
					if (currentVal > 0)
						currentVal--;
					handleButtonPress(1);
					break;

				case 2: // DOWN
					if (currentVal < options.size() - 1)
						currentVal++;
					handleButtonPress(2);
					break;

				case 3: // SAVE
					cfg.vocabInUse = options[currentVal];
					saveConfiguration(&cfg);
					restartWithDelay(2000, disp);
					break;

				case 4: // CNCL
					return MenuEvent::BACK;
			}
		}
		else if (ev.type == DisplayEventType::KEY_RELEASE)
		{
			handleButtonRelease(ev.keyNum);
		}
	}

	return MenuEvent::NOOP;
}

