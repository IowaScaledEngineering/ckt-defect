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
	std::shared_ptr<Menu> minAxles;
	std::shared_ptr<Menu> entranceAxles;
	std::shared_ptr<Menu> speedConfig;
	std::shared_ptr<Menu> speedUnits;
	std::shared_ptr<Menu> speedType;
	std::shared_ptr<Menu> minSpeed;
	std::shared_ptr<Menu> tempUnits;
	std::shared_ptr<Menu> tempType;
	std::shared_ptr<Menu> minTemp;
	std::shared_ptr<Menu> maxTemp;
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
		if (menus.speedUnits)   menus.speedUnits->unhide();
		if (menus.speedType)   menus.speedType->unhide();
		if (menus.minSpeed)    menus.minSpeed->unhide();      
	} else {
		if (menus.speedUnits)   menus.speedUnits->hide();
		if (menus.speedType)   menus.speedType->hide();
		if (menus.minSpeed)    menus.minSpeed->hide();
	}

	// Temperature
	if(cfg.temperatureEnable)
	{
		if(menus.tempUnits) menus.tempUnits->unhide();
		if(menus.tempType)  menus.tempType->unhide();
		if(!cfg.temperatureReal)
		{
			if(menus.minTemp)   menus.minTemp->unhide();
			if(menus.maxTemp)   menus.maxTemp->unhide();
		}
		else
		{
			if(menus.minTemp)   menus.minTemp->hide();
			if(menus.maxTemp)   menus.maxTemp->hide();
		}
	}
	else
	{
		if(menus.tempUnits) menus.tempUnits->hide();
		if(menus.tempType)  menus.tempType->hide();
		if(menus.minTemp)   menus.minTemp->hide();
		if(menus.maxTemp)   menus.maxTemp->hide();
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
		"Enter/Exit",
		&cfg.speedTypeEnter,
		false, 
		"Entrance Speed", "ENTR",
		"Exit Speed", "EXIT", 
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMinSpeed = std::make_shared<MenuNumberDial>(
		"Minimum Speed",
		&cfg.minSpeed,
		false,
		0,   // min
		50,  // max
		cfg.speedUnitsMph ? "mph" : "kph",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuSpeedUnits = std::make_shared<MenuBoolSelector>(  // Declare after menus that need units updated
		"Units",
		&cfg.speedUnitsMph,
		false, 
		"Miles/Hour", "MPH",
		"Kilometers/Hour", "KPH", 
		[&cfg, menuMinSpeed]() { saveConfiguration(&cfg); menuMinSpeed->setUnits(cfg.speedUnitsMph ? "mph" : "kph"); }
	);

	// Timeout
	auto menuDetectorTimeout = std::make_shared<MenuNumberDial>(
		"Detector Timeout",
		&cfg.detectorTimeout,
		false,
		2,   // min
		30,  // max
		"sec",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	
	// Temperature
	auto menuTemperatureConfig = std::make_shared<MenuListSelector>("Temperature Config");
	auto menuTemperatureEn = std::make_shared<MenuBoolSelector>(
		"Temperature Enable",
		&cfg.temperatureEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuTemperatureType = std::make_shared<MenuBoolSelector>(
		"Real/Simulated",
		&cfg.temperatureReal,
		false, 
		"Real", "REAL",
		"Simulated", "SIM" 
	);
	std::string degF("\xDF" "F", 2);
	std::string degC("\xDF" "C", 2);
	auto menuMinTemperature = std::make_shared<MenuNumberDial>(
		"Minimum Temp",
		&cfg.minTemperature,
		false,
		-100,   // min
		150,  // max
		cfg.temperatureUnitsF ? degF : degC,
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMaxTemperature = std::make_shared<MenuNumberDial>(
		"Maximum Temp",
		&cfg.maxTemperature,
		false,
		-100,   // min
		150,  // max
		cfg.temperatureUnitsF ? degF : degC,
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuTemperatureUnits = std::make_shared<MenuBoolSelector>(  // Declare after menus that need units updated
		"Units",
		&cfg.temperatureUnitsF,
		false, 
		"Fahrenheit", degF,
		"Celsius", degC, 
		[&cfg, menuMinTemperature, menuMaxTemperature, degF, degC]() { saveConfiguration(&cfg); menuMinTemperature->setUnits(cfg.temperatureUnitsF ? degF : degC); menuMaxTemperature->setUnits(cfg.temperatureUnitsF ? degF : degC); }
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
		menuMinAxles, menuEntranceAxles,
		menuSpeedConfig, menuSpeedUnits, menuSpeedType, menuMinSpeed,
		menuTemperatureUnits, menuTemperatureType, menuMinTemperature, menuMaxTemperature
	};

	// ==========================================
	// Assign Button Callbacks
	// ==========================================
	menuMilepostEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTrackNameEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); updateTrackNames(&cfg); });
	menuAxleEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuSpeedEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTemperatureEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTemperatureType->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
		
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
	menuSpeedConfig->addChild(menuSpeedUnits);
	menuSpeedConfig->addChild(menuSpeedType);
	menuSpeedConfig->addChild(menuMinSpeed);

	mainSel->addChild(menuDetectorTimeout);

	mainSel->addChild(menuTemperatureConfig);
	menuTemperatureConfig->addChild(menuTemperatureEn);
	menuTemperatureConfig->addChild(menuTemperatureUnits);
	menuTemperatureConfig->addChild(menuTemperatureType);
	menuTemperatureConfig->addChild(menuMinTemperature);
	menuTemperatureConfig->addChild(menuMaxTemperature);

	mainSel->addChild(menuSysConfig);
	menuSysConfig->addChild(menuBacklightLevel);
	menuSysConfig->addChild(menuVolume);

	updateAllMenuVisibility(cfg, managed);
	
	return home;
}
