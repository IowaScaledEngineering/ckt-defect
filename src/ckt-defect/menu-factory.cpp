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
	std::shared_ptr<Menu> directionName1;
	std::shared_ptr<Menu> directionName2;
	std::shared_ptr<Menu> railName;
	std::shared_ptr<Menu> hotJournalRate;
	std::shared_ptr<Menu> hotWheelRate;
	std::shared_ptr<Menu> highImpactWheelRate;
	std::shared_ptr<Menu> draggingEquipmentRate;
	std::shared_ptr<Menu> highLoadRate;
	std::shared_ptr<Menu> wideLoadRate;
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
		if (menus.minAxles)      menus.minAxles->unhide();      
		if (menus.entranceAxles) menus.entranceAxles->unhide();
	} else {
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

	// Direction Visibility
	if (cfg.directionEnable) {
		if (menus.directionName1) menus.directionName1->unhide();
		if (menus.directionName2) menus.directionName2->unhide();
	} else {
		if (menus.directionName1) menus.directionName1->hide();
		if (menus.directionName2) menus.directionName2->hide();
	}

	// Rail Name Visibility
	if (cfg.railNameEnable) {
		if (menus.railName) menus.railName->unhide();
	} else {
		if (menus.railName) menus.railName->hide();
	}

	// Defect Rate Visibilities
	if (cfg.defectHotJournalEnable) {
		if (menus.hotJournalRate) menus.hotJournalRate->unhide();
	} else {
		if (menus.hotJournalRate) menus.hotJournalRate->hide();
	}

	if (cfg.defectHotWheelEnable) {
		if (menus.hotWheelRate) menus.hotWheelRate->unhide();
	} else {
		if (menus.hotWheelRate) menus.hotWheelRate->hide();
	}

	if (cfg.defectHighImpactWheelEnable) {
		if (menus.highImpactWheelRate) menus.highImpactWheelRate->unhide();
	} else {
		if (menus.highImpactWheelRate) menus.highImpactWheelRate->hide();
	}

	if (cfg.defectDraggingEquipmentEnable) {
		if (menus.draggingEquipmentRate) menus.draggingEquipmentRate->unhide();
	} else {
		if (menus.draggingEquipmentRate) menus.draggingEquipmentRate->hide();
	}

	if (cfg.defectHighLoadEnable) {
		if (menus.highLoadRate) menus.highLoadRate->unhide();
	} else {
		if (menus.highLoadRate) menus.highLoadRate->hide();
	}

	if (cfg.defectWideLoadEnable) {
		if (menus.wideLoadRate) menus.wideLoadRate->unhide();
	} else {
		if (menus.wideLoadRate) menus.wideLoadRate->hide();
	}
}

std::shared_ptr<Menu> createAppMenu(DetectorConfiguration &cfg, DisplayLcd *lcd, DataBundle* data, MessageBundle &trackMessages)
{
	// Create the root home menu and main branch
	auto home = std::make_shared<MenuHome>("Home", cfg, data);
	auto mainSel = std::make_shared<MenuListSelector>("Main");
	home->addChild(mainSel);

	// ==========================================
	// Non-Toggle Menu Objects
	// ==========================================

	// Milepost
	auto menuMilepostConfig = std::make_shared<MenuListSelector>("Milepost");
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
	auto menuTrackConfig = std::make_shared<MenuListSelector>("Track Name");
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
		[&cfg]() { saveConfiguration(&cfg); updateTrackNames(&cfg); }
	);

	auto menuTrackNameB = std::make_shared<MenuOptionSelector>(
		"Track B Name", 
		&cfg.trackNameId[1],
		false,
		trackNames,
		[&cfg]() { saveConfiguration(&cfg); updateTrackNames(&cfg); }
	);
	
	// Axle
	auto menuAxleConfig = std::make_shared<MenuListSelector>("Axle Count");
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
		1,   //step
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMinAxles = std::make_shared<MenuNumberDial>(
		"Minimum Axles",
		&cfg.minAxles,
		false,
		0,   // min
		100,  // max
		1,   //step
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Speed
	auto menuSpeedConfig = std::make_shared<MenuListSelector>("Speed");
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
		1,   //step
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
	auto menuSpeedScale = std::make_shared<MenuDigitThumbwheel>(
		"Speed Scale",
		&cfg.speedScale,
		false,
		3,
		1,
		true,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Direction
	auto menuDirectionConfig = std::make_shared<MenuListSelector>("Direction");
	auto menuDirectionEn = std::make_shared<MenuBoolSelector>(
		"Direction Enable",
		&cfg.directionEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuDirectionName1 = std::make_shared<MenuOptionSelector>(
		"Direction 1 Name", 
		&cfg.direction1NameId,
		false,
		directionNames,
		[&cfg]() { saveConfiguration(&cfg); updateDirectionNames(&cfg); }
	);
	auto menuDirectionName2 = std::make_shared<MenuOptionSelector>(
		"Direction 2 Name", 
		&cfg.direction2NameId,
		false,
		directionNames,
		[&cfg]() { saveConfiguration(&cfg); updateDirectionNames(&cfg); }
	);
	auto menuTriggerDir1 = std::make_shared<MenuBoolSelector>(
		"Trigger Dir 1 Only",
		&cfg.triggerDirection1Only,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuTriggerDir2 = std::make_shared<MenuBoolSelector>(
		"Trigger Dir 2 Only",
		&cfg.triggerDirection2Only,
		false,
		"On", "ON",
		"Off", "OFF"
	);

	// Rail Name
	auto menuRailConfig = std::make_shared<MenuListSelector>("Rail Name");
	auto menuRailNameEn = std::make_shared<MenuBoolSelector>(
		"Rail Name Enable",
		&cfg.railNameEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuRailName = std::make_shared<MenuOptionSelector>(
		"Rail Name", 
		&cfg.railNameId,
		false,
		railNames,
		[&cfg]() { saveConfiguration(&cfg); updateRailNames(&cfg); }
	);

	// Defect
	auto menuDefects = std::make_shared<MenuListSelector>("Defects");

	// Hot Journal
	auto menuHotJournal = std::make_shared<MenuListSelector>("Hot Journal");
	auto menuHotJournalEn = std::make_shared<MenuBoolSelector>(
		"Hot Journal Enable",
		&cfg.defectHotJournalEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuHotJournalRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHotJournalAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Hot Wheel
	auto menuHotWheel = std::make_shared<MenuListSelector>("Hot Wheel");
	auto menuHotWheelEn = std::make_shared<MenuBoolSelector>(
		"Hot Wheel Enable",
		&cfg.defectHotWheelEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuHotWheelRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHotWheelAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// High Impact Wheel
	auto menuHighImpactWheel = std::make_shared<MenuListSelector>("High Impact Wheel");
	auto menuHighImpactWheelEn = std::make_shared<MenuBoolSelector>(
		"HI Wheel Enable",
		&cfg.defectHighImpactWheelEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuHighImpactWheelRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHighImpactWheelAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Dragging Equipment
	auto menuDraggingEquipment = std::make_shared<MenuListSelector>("Dragging Equip");
	auto menuDraggingEquipmentEn = std::make_shared<MenuBoolSelector>(
		"Drag Equip Enable",
		&cfg.defectDraggingEquipmentEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuDraggingEquipmentRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectDraggingEquipmentAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// High Load
	auto menuHighLoad = std::make_shared<MenuListSelector>("High Load");
	auto menuHighLoadEn = std::make_shared<MenuBoolSelector>(
		"High Load Enable",
		&cfg.defectHighLoadEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuHighLoadRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHighLoadAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Wide Load
	auto menuWideLoad = std::make_shared<MenuListSelector>("Wide Load");
	auto menuWideLoadEn = std::make_shared<MenuBoolSelector>(
		"Wide Load Enable",
		&cfg.defectWideLoadEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	auto menuWideLoadRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectWideLoadAxleRate,
		false,
		6,
		0,
		false,
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Messages
	auto menuMessages = std::make_shared<MenuListSelector>("Messages");
	auto menuEntranceMessage = std::make_shared<MenuBoolSelector>(
		"Entrance Message",
		&cfg.entranceMessageEnable,
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	auto menuAlertMessage = std::make_shared<MenuBoolSelector>(
		"Live Defect Msgs",
		&cfg.alertMessageEnable,
		false, 
		"On", "ON", 
		"Off", "OFF"
	);

	// Exit Message
	auto menuExitMessage = std::make_shared<MenuListSelector>("Exit Message");
	auto menuTalkDefectOnly = std::make_shared<MenuBoolSelector>(
		"Talk Defect Only",
		&cfg.talkOnDefectOnly,
		false, 
		"On", "ON", 
		"Off", "OFF"
	);

	auto menuMaxDefects = std::make_shared<MenuNumberDial>(
		"Max Defects",
		&cfg.maxDefects,
		false,
		1,   // min
		10,  // max
		1,   //step
		"defects",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuOrdinalEnable = std::make_shared<MenuBoolSelector>(
		"Ordinal Enable",
		&cfg.ordinalDefectList,
		false, 
		"On", "ON", 
		"Off", "OFF"
	);

	// Temperature
	auto menuTemperatureConfig = std::make_shared<MenuListSelector>("Temperature");
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
		[&cfg]() { return getMinTemperature(&cfg); },
		[&cfg](int16_t val) { setMinTemperature(&cfg, val); },
		false,
		-99,   // min
		150,  // max
		1,   //step
		cfg.temperatureUnitsF ? degF : degC,
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMaxTemperature = std::make_shared<MenuNumberDial>(
		"Maximum Temp",
		[&cfg]() { return getMaxTemperature(&cfg); },
		[&cfg](int16_t val) { setMaxTemperature(&cfg, val); },
		false,
		-99,   // min
		150,  // max
		1,   //step
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

	// Operation Mode
	auto menuOperationMode = std::make_shared<MenuBoolSelector>(
		"Operation Mode",
		&cfg.infrastructureMode,
		false,
		"Infrastructure", "INFR",
		"Defect Detect", "DFCT",
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Timing
	auto menuTimingConfig = std::make_shared<MenuListSelector>("Timing");
	auto menuDetectorTimeout = std::make_shared<MenuNumberDial>(
		"Exit Timeout",
		&cfg.detectorTimeout,
		false,
		2,   // min
		30,  // max
		1,   //step
		"sec",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuExitDisplayTimeout = std::make_shared<MenuNumberDial>(
		"Summary Display",
		&cfg.exitDisplayTimeout,
		false,
		2,   // min
		60,  // max
		1,   //step
		"sec",
		[&cfg]() { saveConfiguration(&cfg); }
	);

	// Audio
	auto menuAudio = std::make_shared<MenuListSelector>("Audio");
	auto menuVolume = std::make_shared<MenuVolume>(
		"Audio Volume",
		5,
		true,
		150,
		audioGetVolumeStep,
		audioSetVolumeStep,
		[lcd, &cfg]() { cfg.volumeStep = audioGetVolumeStep(); saveConfiguration(&cfg); }
	);
	auto menuNoise = std::make_shared<MenuVolume>(
		"White Noise",
		10,
		false,
		100,
		audioGetNoiseStep,
		audioSetNoiseStep,
		[lcd, &cfg]() { cfg.noiseStep = audioGetNoiseStep(); saveConfiguration(&cfg); }
	);
	auto menuPopcorn = std::make_shared<MenuVolume>(
		"Popcorn Noise",
		10,
		false,
		100,
		audioGetPopcornStep,
		audioSetPopcornStep,
		[lcd, &cfg]() { cfg.popcornStep = audioGetPopcornStep(); saveConfiguration(&cfg); }
	);
	
	// System	
	auto menuSysConfig = std::make_shared<MenuListSelector>("System");
	auto menuBacklightLevel = std::make_shared<MenuPercentageBar>(
		"Backlight Level", 
		[lcd]() { return lcd->getBrightness(); },
		[lcd](uint32_t val) { lcd->setBrightness(val); },
		true,
		255, 
		10,
		[lcd, &cfg]() { cfg.lcdBrightness = lcd->getBrightness(); saveConfiguration(&cfg); }
	);
	auto menuPttDelay = std::make_shared<MenuNumberDial>(
		"PTT Delay",
		audioGetPttDelay,
		audioSetPttDelay,
		false,
		0,     // min
		3000,  // max
		100,   // step
		"ms",
		[&cfg]() { cfg.pttDelay = audioGetPttDelay()/100; saveConfiguration(&cfg); }
	);

	// Diagnostic	
	auto menuDiagnostics = std::make_shared<MenuListSelector>("Diagnostics");
	auto menuVocabTest = std::make_shared<MenuVocabTest>("Vocab Test");

	// Package up all managed controls into our visibility group
	ManagedMenus managed = {
		menuMilepost, menuTrackNameA, menuTrackNameB, 
		menuMinAxles, menuEntranceAxles,
		menuSpeedConfig, menuSpeedUnits, menuSpeedType, menuMinSpeed,
		menuTemperatureUnits, menuTemperatureType, menuMinTemperature, menuMaxTemperature,
		menuDirectionName1, menuDirectionName2,
		menuRailName,
		menuHotJournalRate, menuHotWheelRate, menuHighImpactWheelRate,
		menuDraggingEquipmentRate, menuHighLoadRate, menuWideLoadRate
	};

	// ==========================================
	// Assign Button Callbacks
	// ==========================================
	menuMilepostEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTrackNameEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); updateTrackNames(&cfg); });
	menuAxleEn->setSaveCallback([&cfg, managed, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); setDefaultMessages(trackMessages, cfg); });
	menuSpeedEn->setSaveCallback([&cfg, managed, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); setDefaultMessages(trackMessages, cfg); });
	menuTemperatureEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuTemperatureType->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuDirectionEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); updateDirectionNames(&cfg); });
	menuRailNameEn->setSaveCallback([&cfg, managed, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); updateRailNames(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuEntranceMessage->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuAlertMessage->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuTalkDefectOnly->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });

	// Defect Enable Callbacks
	menuHotJournalEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuHotWheelEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuHighImpactWheelEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuDraggingEquipmentEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuHighLoadEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });
	menuWideLoadEn->setSaveCallback([&cfg, managed]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg, managed); });

	// Trigger 1 and Trigger 2 mutual-exclusion callbacks
	menuTriggerDir1->setSaveCallback([&cfg]() {
		if (cfg.triggerDirection1Only) {
			cfg.triggerDirection2Only = false;
		}
		saveConfiguration(&cfg);
	});

	menuTriggerDir2->setSaveCallback([&cfg]() {
		if (cfg.triggerDirection2Only) {
			cfg.triggerDirection1Only = false;
		}
		saveConfiguration(&cfg);
	});
		
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
	menuSpeedConfig->addChild(menuSpeedScale);
	menuSpeedConfig->addChild(menuSpeedType);
	menuSpeedConfig->addChild(menuMinSpeed);

	mainSel->addChild(menuDirectionConfig);
	menuDirectionConfig->addChild(menuDirectionEn);
	menuDirectionConfig->addChild(menuDirectionName1);
	menuDirectionConfig->addChild(menuDirectionName2);
	menuDirectionConfig->addChild(menuTriggerDir1);
	menuDirectionConfig->addChild(menuTriggerDir2);

	mainSel->addChild(menuRailConfig);
	menuRailConfig->addChild(menuRailNameEn);
	menuRailConfig->addChild(menuRailName);

	mainSel->addChild(menuDefects);
	menuDefects->addChild(menuHotJournal);
	menuHotJournal->addChild(menuHotJournalEn);
	menuHotJournal->addChild(menuHotJournalRate);

	menuDefects->addChild(menuHotWheel);
	menuHotWheel->addChild(menuHotWheelEn);
	menuHotWheel->addChild(menuHotWheelRate);

	menuDefects->addChild(menuHighImpactWheel);
	menuHighImpactWheel->addChild(menuHighImpactWheelEn);
	menuHighImpactWheel->addChild(menuHighImpactWheelRate);

	menuDefects->addChild(menuDraggingEquipment);
	menuDraggingEquipment->addChild(menuDraggingEquipmentEn);
	menuDraggingEquipment->addChild(menuDraggingEquipmentRate);

	menuDefects->addChild(menuHighLoad);
	menuHighLoad->addChild(menuHighLoadEn);
	menuHighLoad->addChild(menuHighLoadRate);

	menuDefects->addChild(menuWideLoad);
	menuWideLoad->addChild(menuWideLoadEn);
	menuWideLoad->addChild(menuWideLoadRate);

	mainSel->addChild(menuMessages);
	menuMessages->addChild(menuEntranceMessage);
	menuMessages->addChild(menuAlertMessage);

	menuMessages->addChild(menuExitMessage);
	menuExitMessage->addChild(menuTalkDefectOnly);
	menuExitMessage->addChild(menuMaxDefects);
	menuExitMessage->addChild(menuOrdinalEnable);

	mainSel->addChild(menuTemperatureConfig);
	menuTemperatureConfig->addChild(menuTemperatureEn);
	menuTemperatureConfig->addChild(menuTemperatureUnits);
	menuTemperatureConfig->addChild(menuTemperatureType);
	menuTemperatureConfig->addChild(menuMinTemperature);
	menuTemperatureConfig->addChild(menuMaxTemperature);

	mainSel->addChild(menuOperationMode);

	mainSel->addChild(menuTimingConfig);
	menuTimingConfig->addChild(menuDetectorTimeout);
	menuTimingConfig->addChild(menuExitDisplayTimeout);

	mainSel->addChild(menuAudio);
	menuAudio->addChild(menuVolume);
	menuAudio->addChild(menuNoise);
	menuAudio->addChild(menuPopcorn);

	mainSel->addChild(menuSysConfig);
	menuSysConfig->addChild(menuBacklightLevel);
	menuSysConfig->addChild(menuPttDelay);

	mainSel->addChild(menuDiagnostics);
	menuDiagnostics->addChild(menuVocabTest);

	updateAllMenuVisibility(cfg, managed);
	
	return home;
}
