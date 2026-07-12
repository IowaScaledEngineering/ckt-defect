#include "src/menu/menu.h"
#include "configuration.h"
#include "data.h"
#include <Arduino.h>

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
		std::string dispString;
	public:
		MenuHome(const std::string &n, const DetectorConfiguration &c, DataBundle* d) 
			: Menu(n), cfg(c), data(d) {}
		void onEnter() override;
		MenuEvent update() override;
};

class MenuVolume : public Menu
{
	private:
		int32_t currentVal; // Internal percentage (0 to 150)

	public:
		// Constructor using std::function callbacks
		MenuVolume(const std::string &name, std::function<uint32_t()> getter,
		                          std::function<void(uint32_t)> setter, bool realTimeUpdate, std::function<void()> onSave = nullptr)
		    : Menu(name)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}
		
		void onEnter() override;
		MenuEvent update() override;
};
