#include "display-term.h"
#include "menu-custom.h"
#include "menu-mgr.h"
#include "menu.h"
#include <csignal>
#include <unistd.h>

volatile sig_atomic_t run = 1;
void handle_interrupt(int sig) { run = 0; }

int main()
{
	signal(SIGINT, handle_interrupt);
	DisplayTerm *disp = new DisplayTerm();
	Menu::setDisplay(disp); // Registers the hardware display context globally for the UI

	uint32_t val2_1 = 100;
	uint32_t val2_2 = 100;
	uint32_t val2_3 = 100;
	uint32_t val2_4 = 100;
	uint32_t val2_5 = 100;
	uint32_t val2_6 = 100;
	uint32_t val3_1 = 100;
	uint32_t val3_2 = 100;
	uint32_t val4 = 100;
	bool val5 = false;
	uint32_t val6 = 3;
	uint32_t val7 = 100;
	uint32_t valFloat = 4725;

	std::vector<std::string> options = {
	    "Arizona", "Alaska", "Colorado", "Florida", "Iowa", "Kansas", "Nebraska", "Wyoming",
	};

	auto home = std::make_shared<MenuHome>("Home");
	auto mainSel = std::make_shared<MenuListSelector>("Main");
	home->addChild(mainSel);

	auto menu1 = std::make_shared<MenuDigitThumbwheel>("Digit Thumbwheel", &valFloat, 5, 1, true);
	auto menu2 = std::make_shared<MenuListSelector>("Menu 2");
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.1", &val2_1, 3, 0, false));
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.2", &val2_2, 3, 0, false));
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.3", &val2_3, 3, 0, false));
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.4", &val2_4, 3, 0, false));
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.5", &val2_5, 3, 0, false));
	menu2->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 2.6", &val2_6, 3, 0, false));

	auto menu3 = std::make_shared<MenuListSelector>("Menu 3");
	menu3->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 3.1", &val3_1, 3, 0, true));
	menu3->addChild(std::make_shared<MenuDigitThumbwheel>("Menu 3.2", &val3_2, 3, 0, true));

	auto menu4 = std::make_shared<MenuNumberDial>("Number Dial", &val4, 0, 120, "sec");
	auto menu5 = std::make_shared<MenuBoolSelector>("Bool Select", &val5, "Enable", "ENBL", "Disable", "DSBL");
	auto menu6 = std::make_shared<MenuOptionSelector>("Option Select", &val6, options);
	auto menu7 = std::make_shared<MenuDigitThumbwheel>("Menu 7", &val7, 3, 0, false);

	mainSel->addChild(menu1);
	mainSel->addChild(menu2);
	mainSel->addChild(menu3);
	mainSel->addChild(menu4);
	mainSel->addChild(menu5);
	mainSel->addChild(menu6);
	mainSel->addChild(menu7);

	auto menu8 = std::make_shared<MenuNavHome>("Navigator");
	mainSel->addChild(menu8);

	auto menuA = std::make_shared<MenuNav>("Menu A");
	auto menuB = std::make_shared<MenuNav>("Menu B");
	auto menuC = std::make_shared<MenuNav>("Menu C");
	auto menuD = std::make_shared<MenuNav>("Menu D");
	auto menuE = std::make_shared<MenuNav>("Menu E");
	menu8->addChild(menuA);
	menu8->addChild(menuB);
	menu8->addChild(menuC);
	menu8->addChild(menuD);
	menu8->addChild(menuE);

	MenuManager mgr(disp, home);

	while(run)
	{
		disp->readKeys();
		mgr.process();
		usleep(100000);
	}
	delete disp;
	return 0;
}
