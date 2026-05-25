#include "src/menu/menu.h"
#include <Arduino.h>

class MenuHome : public Menu
{
	public:
		MenuHome(const std::string &n) : Menu(n) {}
		MenuEvent update() override
		{
			byte smiley[8] = {
			  0b00000,
			  0b00000,
			  0b01010,
			  0b00000,
			  0b00000,
			  0b10001,
			  0b01110,
			  0b00000
			};
			
			disp->createCustomChar(3, smiley);

			disp->backlightOff();
			disp->gotoxy(0, 0);
			disp->print("Home Screen ");
			disp->print(0x03);
			disp->gotoxy(16, 3);
			disp->print("MENU");
			DisplayEvent ev;
			if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN && ev.keyNum == 4)
				return MenuEvent::FORWARD;
			return MenuEvent::NOOP;
		}
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
