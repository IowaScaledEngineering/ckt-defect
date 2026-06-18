#include "menu-factory.h"
#include <vector>
#include <string>

// Include the specific menu implementation headers
#include "src/menu/menu-mgr.h"
#include "menu-custom.h"
#include "audio.h"

std::shared_ptr<Menu> createAppMenu(DetectorConfiguration &cfg, DisplayLcd *lcd)
{
	// Create the root home menu and main branch
	auto home = std::make_shared<MenuHome>("Home", cfg);
	auto mainSel = std::make_shared<MenuListSelector>("Main");
	home->addChild(mainSel);

	// ==========================================
	// Milepost Menus
	// ==========================================
	auto menuMilepostConfig = std::make_shared<MenuListSelector>("Milepost Config");
	mainSel->addChild(menuMilepostConfig);

	auto menuMilepost = std::make_shared<MenuDigitThumbwheel>(
		"Milepost",
		&cfg.milepost,
		false,
		4,
		1,
		true,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto updateMilepostMenuVisibility = [&cfg, menuMilepost]()
	{
		if (cfg.milepostEnable)
		{
			menuMilepost->unhide();
		}
		else
		{
			menuMilepost->hide();
		}
	};

	auto menuMilepostEn = std::make_shared<MenuBoolSelector>(
		"Milepost Enable",
		&cfg.milepostEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF",
		[updateMilepostMenuVisibility, &cfg]() { saveConfiguration(&cfg); updateMilepostMenuVisibility(); }
	);

	updateMilepostMenuVisibility();
	menuMilepostConfig->addChild(menuMilepostEn);
	menuMilepostConfig->addChild(menuMilepost);

	// ==========================================
	// Track Menus
	// ==========================================
	auto menuTrackConfig = std::make_shared<MenuListSelector>("Track Config");
	mainSel->addChild(menuTrackConfig);

	auto menuTrackNameA = std::make_shared<MenuOptionSelector>(
		"Track A Name", 
		&cfg.trackNameId[0],
		false,
		trackNames,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto menuTrackNameB = std::make_shared<MenuOptionSelector>(
		"Track B Name", 
		&cfg.trackNameId[1],
		false,
		trackNames,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto updateTrackNameMenuVisibility = [&cfg, menuTrackNameA, menuTrackNameB]()
	{
		if (cfg.trackNameEnable)
		{
			menuTrackNameA->unhide();
			menuTrackNameB->unhide();
		}
		else
		{
			menuTrackNameA->hide();
			menuTrackNameB->hide();
		}
	};

	auto menuTrackNameEn = std::make_shared<MenuBoolSelector>(
		"Track Name Enable",
		&cfg.trackNameEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF",
		[updateTrackNameMenuVisibility, &cfg]() { saveConfiguration(&cfg); updateTrackNameMenuVisibility(); }
	);

	updateTrackNameMenuVisibility();
	menuTrackConfig->addChild(menuTrackNameEn);
	menuTrackConfig->addChild(menuTrackNameA);
	menuTrackConfig->addChild(menuTrackNameB);

	// ==========================================
	// System Menus
	// ==========================================
	auto menuSysConfig = std::make_shared<MenuListSelector>("System Config");
	mainSel->addChild(menuSysConfig);

	auto menuBacklightLevel = std::make_shared<MenuPercentageBar>(
		"Backlight Level", 
		[lcd]() { return lcd->getBrightness(); },
		[lcd](uint32_t val) { lcd->setBrightness(val); },
		true,
		255, 
		10,
		[lcd, &cfg]() { cfg.lcdBrightness = lcd->getBrightness(); saveConfiguration(&cfg); }
	);

	auto menuVolume = std::make_shared<MenuVolume>(
		"Audio Volume",
		audioGetVolumeStep,
		audioSetVolumeStep,
		true,
		[lcd, &cfg]() { cfg.volumeStep = audioGetVolumeStep(); saveConfiguration(&cfg); }
	);

	menuSysConfig->addChild(menuBacklightLevel);
	menuSysConfig->addChild(menuVolume);

	// ==========================================
	// Test Menus (FIXME: remove later)
	// ==========================================
	static uint32_t valFloat = 4725;
	static uint32_t val2 = 100;
	static bool val3 = false;
	static uint32_t val4 = 3;
	
	std::vector<std::string> options = {
		"Arizona", "Alaska", "Colorado", "Florida", "Iowa", "Kansas", "Nebraska", "Wyoming",
	};

	auto menu1 = std::make_shared<MenuDigitThumbwheel>("Digit Thumbwheel", &valFloat, false, 5, 1, true);
	auto menu2 = std::make_shared<MenuNumberDial>("Number Dial", &val2, false, 0, 120, "sec");
	auto menu3 = std::make_shared<MenuBoolSelector>("Bool Select", &val3, false, "Enable", "ENBL", "Disable", "DSBL");
	auto menu4 = std::make_shared<MenuOptionSelector>("Option Select", &val4, false, options);

	mainSel->addChild(menu1);
	mainSel->addChild(menu2);
	mainSel->addChild(menu3);
	mainSel->addChild(menu4);

	return home;
}