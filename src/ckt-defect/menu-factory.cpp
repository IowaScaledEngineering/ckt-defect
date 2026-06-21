#include "menu-factory.h"
#include <vector>
#include <string>

// Include the specific menu implementation headers
#include "src/menu/menu-mgr.h"
#include "menu-custom.h"
#include "audio.h"

struct ManagedMenus {
	std::shared_ptr<Menu> milepost;
	std::shared_ptr<Menu> trackNameA;
	std::shared_ptr<Menu> trackNameB;
	std::shared_ptr<Menu> speedConfig;
	std::shared_ptr<Menu> speedType;
	std::shared_ptr<Menu> minSpeedEn;
	std::shared_ptr<Menu> minSpeed;
	std::shared_ptr<Menu> minAxles;
	std::shared_ptr<Menu> entranceAxles;
};

void updateAllMenuVisibility(const DetectorConfiguration &cfg, const ManagedMenus &menus)
{
	// Milepost Visibility
	if (cfg.milepostEnable) { if (menus.milepost) menus.milepost->unhide(); }
	else                    { if (menus.milepost) menus.milepost->hide(); }

	// Track Name Visibility
	if (cfg.trackNameEnable) {
		if (menus.trackNameA) menus.trackNameA->unhide();
		if (menus.trackNameB) menus.trackNameB->unhide();
	} else {
		if (menus.trackNameA) menus.trackNameA->hide();
		if (menus.trackNameB) menus.trackNameB->hide();
	}

	// Axle Config Visibility & Child Menu Item Visibility
	if (cfg.axleEnable) {
		if (menus.speedConfig)   menus.speedConfig->unhide();   
		if (menus.minAxles)      menus.minAxles->unhide();      
		if (menus.entranceAxles) menus.entranceAxles->unhide();
	} else {
		if (menus.speedConfig)   menus.speedConfig->hide();
		if (menus.minAxles)      menus.minAxles->hide();        
		if (menus.entranceAxles) menus.entranceAxles->hide();
	}

	// Individual Speed Item Visibilities 
	if (cfg.speedEnable) {
		if (menus.speedType)   menus.speedType->unhide();
		if (menus.minSpeedEn)  menus.minSpeedEn->unhide(); 
		if (menus.minSpeed)    menus.minSpeed->unhide();      
	} else {
		if (menus.speedType)   menus.speedType->hide();
		if (menus.minSpeedEn)  menus.minSpeedEn->hide();   
		if (menus.minSpeed)    menus.minSpeed->hide();
	}

	// Sub-dependence: Min Speed layout override
	if (!cfg.minSpeedEnable) {
		if (menus.minSpeed) menus.minSpeed->hide();
	}
}

std::shared_ptr<Menu> createAppMenu(DetectorConfiguration &cfg, DisplayLcd *lcd)
{
	// Create the root home menu and main branch
	auto home = std::make_shared<MenuHome>("Home", cfg);
	auto mainSel = std::make_shared<MenuListSelector>("Main");
	home->addChild(mainSel);

	// ==========================================
	// Non-Toggle Menu Objects
	// ==========================================

	// Milepost
	auto menuMilepostConfig = std::make_shared<MenuListSelector>("Milepost Config");
	auto menuMilepostEn = std::make_shared<MenuBoolSelector>(
		"Milepost Enable",
		&cfg.milepostEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuMilepost = std::make_shared<MenuDigitThumbwheel>(
		"Milepost",
		&cfg.milepost,
		false,
		4,
		1,
		true,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Track
	auto menuTrackConfig = std::make_shared<MenuListSelector>("Track Config");
	auto menuTrackNameEn = std::make_shared<MenuBoolSelector>(
		"Track Name Enable",
		&cfg.trackNameEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
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
	
	// Speed
	auto menuSpeedConfig = std::make_shared<MenuListSelector>("Speed Config");
	auto menuSpeedEn = std::make_shared<MenuBoolSelector>(
		"Speed Enable",
		&cfg.speedEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuSpeedType = std::make_shared<MenuBoolSelector>(
		"Speed Type",
		&cfg.entranceSpeed,
		false, 
		"Entrance", "ENTR",
		"Exit", "EXIT", 
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMinSpeedEn = std::make_shared<MenuBoolSelector>(
		"Min Speed Enable",
		&cfg.minSpeedEnable,
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuMinSpeed = std::make_shared<MenuNumberDial>(
		"Minimum Speed",
		&cfg.minSpeed,
		false,
		0,   // min
		50,  // max
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	
	// Axle
	auto menuAxleConfig = std::make_shared<MenuListSelector>("Axle Config");
	auto menuAxleEn = std::make_shared<MenuBoolSelector>(
		"Axle Count Enable",
		&cfg.axleEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
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

	// System	
	auto menuSysConfig = std::make_shared<MenuListSelector>("System Config");
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

	// Package up all managed controls into our visibility group
	ManagedMenus managed = {
		menuMilepost, menuTrackNameA, menuTrackNameB, 
		menuSpeedConfig, menuSpeedType, menuMinSpeedEn, menuMinSpeed, 
		menuMinAxles, menuEntranceAxles
	};

	// ==========================================
	// Assign Button Callbacks
	// ==========================================
	menuMilepostEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTrackNameEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuMinSpeedEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuSpeedEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuAxleEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
		
	// ==========================================
	// Assemble Menus
	// ==========================================
	mainSel->addChild(menuMilepostConfig);
	menuMilepostConfig->addChild(menuMilepostEn);
	menuMilepostConfig->addChild(menuMilepost);

	mainSel->addChild(menuTrackConfig);
	menuTrackConfig->addChild(menuTrackNameEn);
	menuTrackConfig->addChild(menuTrackNameA);
	menuTrackConfig->addChild(menuTrackNameB);

	mainSel->addChild(menuAxleConfig);
	menuAxleConfig->addChild(menuAxleEn);
	menuAxleConfig->addChild(menuMinAxles);
	menuAxleConfig->addChild(menuEntranceAxles);

	mainSel->addChild(menuSpeedConfig);
	menuSpeedConfig->addChild(menuSpeedEn);
	menuSpeedConfig->addChild(menuSpeedType);
	menuSpeedConfig->addChild(menuMinSpeedEn);
	menuSpeedConfig->addChild(menuMinSpeed);

	mainSel->addChild(menuSysConfig);
	menuSysConfig->addChild(menuBacklightLevel);
	menuSysConfig->addChild(menuVolume);

	updateAllMenuVisibility(cfg, managed);
	
	return home;
}
