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

class MenuNavHome : public Menu
{
	public:
		MenuNavHome(const std::string &n) : Menu(n) {}
		MenuEvent update() override
		{
			disp->gotoxy(0, 0);
			disp->print("Navigator");
			disp->gotoxy(0, 3);
			disp->print("STRT");
			disp->gotoxy(16, 3);
			disp->print("BACK");

			DisplayEvent ev;
			// Retrieve the event ONCE and verify it's a key down action
			if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
			{
				if(ev.keyNum == 4)
					return MenuEvent::BACK;
				else if(ev.keyNum == 1)
					return MenuEvent::FORWARD; // Successfully triggers navigation to Menu A
			}
			return MenuEvent::NOOP;
		}
};

class MenuNav : public Menu
{
	public:
		MenuNav(const std::string &n) : Menu(n) {}
		MenuEvent update() override
		{
			disp->gotoxy(0, 0);
			disp->print(menuName);

			disp->gotoxy(0, 3);
			disp->print("PREV");
			disp->gotoxy(5, 3);
			disp->print("NEXT");
			disp->gotoxy(16, 3);
			disp->print("BACK");

			DisplayEvent ev;
			// Fixed the same event-swallowing bug here as well
			if(disp->getEvent(&ev) && ev.type == DisplayEventType::KEY_DOWN)
			{
				if(ev.keyNum == 4)
					return MenuEvent::BACK;
				else if(ev.keyNum == 1)
					return MenuEvent::PREV;
				else if(ev.keyNum == 2)
					return MenuEvent::NEXT;
			}
			return MenuEvent::NOOP;
		}
};

class MenuVolume : public Menu
{
	private:
		int32_t currentVal; // Internal percentage (0 to 150)
		uint8_t state = 0;

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

		MenuEvent update() override;
};
