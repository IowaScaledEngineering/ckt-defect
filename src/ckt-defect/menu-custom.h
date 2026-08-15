#pragma once

#include "src/menu/menu.h"
#include "configuration.h"
#include "data.h"
#include <Arduino.h>
#include "sound.h"
#include "audio.h"
#include <vector>

enum class MenuHomeState {
        STANDBY,
        ACTIVE,
        MESSAGE,
        WAIT,
};

class MenuHome : public Menu
{
	private:
		bool backlightState = false;
		bool delayBacklightOff = false;
		unsigned long backlightDelayStartTime;
		unsigned long waitStartTime;
		const DetectorConfiguration &cfg;
		DataBundle* data;
		MenuHomeState state;
		void renderHomeUI(const std::string& statusText, bool showLightButton);
		void renderMessage(const std::string& message);
		std::string dispString;
		std::string lastDisplayedMessage;
	public:
		MenuHome(const std::string &n, const DetectorConfiguration &c, DataBundle* d) 
			: Menu(n), cfg(c), data(d) {}
		void onEnter() override;
		MenuEvent update() override;
};

class MenuVolume : public Menu
{
	private:
		int32_t currentVal;   // Internal percentage (0 to maxPercent)
		uint32_t stepSize;    // Percentage increment per step
		bool allowOver;       // Whether values > 100% are allowed
		uint32_t maxPercent;  // Maximum allowed percentage

	public:
		// Constructor using std::function callbacks with configurable range options
		MenuVolume(const std::string &name, 
		           uint32_t step,
		           bool allowBoost,
		           uint32_t maxBoostPercent,
		           std::function<uint32_t()> getter,
		           std::function<void(uint32_t)> setter, 
		           std::function<void()> onSave = nullptr)
		    : Menu(name), 
		      stepSize(step > 0 ? step : 5), 
		      allowOver(allowBoost), 
		      maxPercent(allowBoost ? maxBoostPercent : 100)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			saveCallback = std::move(onSave);
		}
		
		void onEnter() override;
		MenuEvent update() override;
};

class MenuVocabTest : public Menu
{
private:
    size_t currentIndex;

public:
    MenuVocabTest(const std::string &name)
        : Menu(name), currentIndex(0) {}

    void onEnter() override;
    MenuEvent update() override;
};

