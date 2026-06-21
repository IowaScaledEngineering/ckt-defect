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
	// Speed Menus
	// ==========================================
	auto menuSpeedConfig = std::make_shared<MenuListSelector>("Speed Config");
	//mainSel->addChild(menuSpeedConfig);  --> Done down below to put the menu after Axle Config

	auto updateGlobalSpeedMenuVisibility = [&cfg, menuSpeedConfig]()
	{
		if (cfg.axleEnable)
		{
			menuSpeedConfig->unhide();
		}
		else
		{
			menuSpeedConfig->hide();
		}
	};

	auto menuMinSpeed = std::make_shared<MenuNumberDial>(
		"Minimum Speed",
		&cfg.minSpeed,
		false,
		0,   // min
		50,  // max
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	
	auto updateMinSpeedMenuVisibility = [&cfg, menuMinSpeed]()
	{
		if (cfg.minSpeedEnable)
		{
			menuMinSpeed->unhide();
		}
		else
		{
			menuMinSpeed->hide();
		}
	};

	auto menuMinSpeedEn = std::make_shared<MenuBoolSelector>(
		"Min Speed Enable",
		&cfg.minSpeedEnable,
		false, 
		"On", "ON", 
		"Off", "OFF",
		[updateMinSpeedMenuVisibility, &cfg]() { saveConfiguration(&cfg); updateMinSpeedMenuVisibility(); }
	);

	auto menuSpeedType = std::make_shared<MenuBoolSelector>(
		"Speed Type",
		&cfg.entranceSpeed,
		false, 
		"Entrance", "ENTR",
		"Exit", "EXIT", 
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto updateSpeedMenuVisibility = [&cfg, menuSpeedType, menuMinSpeedEn, menuMinSpeed]()
	{
		if (cfg.speedEnable)
		{
			menuSpeedType->unhide();
			menuMinSpeedEn->unhide();
			menuMinSpeed->unhide();
		}
		else
		{
			menuSpeedType->hide();
			menuMinSpeedEn->hide();
			menuMinSpeed->hide();
		}
	};

	auto menuSpeedEn = std::make_shared<MenuBoolSelector>(
		"Speed Enable",
		&cfg.speedEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF",
		[updateSpeedMenuVisibility, &cfg]() { saveConfiguration(&cfg); updateSpeedMenuVisibility(); }
	);

	updateSpeedMenuVisibility();
	menuSpeedConfig->addChild(menuSpeedEn);
	menuSpeedConfig->addChild(menuSpeedType);
	menuSpeedConfig->addChild(menuMinSpeedEn);
	menuSpeedConfig->addChild(menuMinSpeed);

	// ==========================================
	// Axle Menus
	// ==========================================
	auto menuAxleConfig = std::make_shared<MenuListSelector>("Axle Config");
	mainSel->addChild(menuAxleConfig);
	mainSel->addChild(menuSpeedConfig);

	auto menuEntranceAxles = std::make_shared<MenuNumberDial>(
		"Entrance Axles",
		&cfg.entranceAxles,
		false,
		0,   // min
		10,  // max
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto menuMinAxles = std::make_shared<MenuNumberDial>(
		"Minimum Axles",
		&cfg.minAxles,
		false,
		0,   // min
		100,  // max
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);

	auto updateAxleMenuVisibility = [&cfg, menuEntranceAxles, menuMinAxles]()
	{
		if (cfg.axleEnable)
		{
			menuEntranceAxles->unhide();
			menuMinAxles->unhide();
		}
		else
		{
			menuEntranceAxles->hide();
			menuMinAxles->hide();
		}
	};

	auto menuAxleEn = std::make_shared<MenuBoolSelector>(
		"Axle Count Enable",
		&cfg.axleEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF",
		[updateAxleMenuVisibility, updateGlobalSpeedMenuVisibility, &cfg]() { saveConfiguration(&cfg); updateAxleMenuVisibility(); updateGlobalSpeedMenuVisibility(); }
	);

	updateAxleMenuVisibility();
	updateGlobalSpeedMenuVisibility();
	menuAxleConfig->addChild(menuAxleEn);
	menuAxleConfig->addChild(menuMinAxles);
	menuAxleConfig->addChild(menuEntranceAxles);

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