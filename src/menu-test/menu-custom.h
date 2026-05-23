#include "menu.h"
class MenuHome : public Menu
{
	public:
		MenuHome(const std::string &n) : Menu(n) {}
		MenuEvent update() override
		{
			disp->backlightOff();
			disp->gotoxy(0, 0);
			disp->print("Home Screen");
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
