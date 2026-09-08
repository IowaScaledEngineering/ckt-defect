#include "menu-factory.h"
#include <vector>
#include <string>

// Include the specific menu implementation headers
#include "src/menu/menu-mgr.h"
#include "menu-custom.h"
#include "audio.h"

namespace {
// File-scoped menu references restricted to this translation unit
std::shared_ptr<Menu> menuMilepost;
std::shared_ptr<Menu> menuTrackNameA;
std::shared_ptr<Menu> menuTrackNameB;
std::shared_ptr<Menu> menuMinAxles;
std::shared_ptr<Menu> menuEntranceAxles;
std::shared_ptr<Menu> menuSpeedUnits;
std::shared_ptr<Menu> menuSpeedType;
std::shared_ptr<Menu> menuMinSpeed;
std::shared_ptr<Menu> menuTemperatureUnits;
std::shared_ptr<Menu> menuTemperatureType;
std::shared_ptr<Menu> menuMinTemperature;
std::shared_ptr<Menu> menuMaxTemperature;
std::shared_ptr<Menu> menuDirectionName1;
std::shared_ptr<Menu> menuDirectionName2;
std::shared_ptr<Menu> menuRailName;
std::shared_ptr<Menu> menuHotJournalRate;
std::shared_ptr<Menu> menuHotWheelRate;
std::shared_ptr<Menu> menuHighImpactWheelRate;
std::shared_ptr<Menu> menuDraggingEquipmentRate;
std::shared_ptr<Menu> menuHighLoadRate;
std::shared_ptr<Menu> menuWideLoadRate;
} // namespace

void updateAllMenuVisibility(const DetectorConfiguration &cfg)
{
	// Milepost Visibility
	if (cfg.milepostEnable) { if (menuMilepost) menuMilepost->unhide(); }
	else                    { if (menuMilepost) menuMilepost->hide(); }

	// Track Name Visibility
	if (cfg.trackNameEnable) {
		if (menuTrackNameA) menuTrackNameA->unhide();
		if (menuTrackNameB) menuTrackNameB->unhide();
	} else {
		if (menuTrackNameA) menuTrackNameA->hide();
		if (menuTrackNameB) menuTrackNameB->hide();
	}

	// Axle Config Visibility & Child Menu Item Visibility
	if (cfg.axleEnable) {
		if (menuMinAxles)      menuMinAxles->unhide();      
		if (menuEntranceAxles) menuEntranceAxles->unhide();
	} else {
		if (menuMinAxles)      menuMinAxles->hide();        
		if (menuEntranceAxles) menuEntranceAxles->hide();
	}

	// Individual Speed Item Visibilities 
	if (cfg.speedEnable) {
		if (menuSpeedUnits)   menuSpeedUnits->unhide();
		if (menuSpeedType)    menuSpeedType->unhide();
		if (menuMinSpeed)     menuMinSpeed->unhide();      
	} else {
		if (menuSpeedUnits)   menuSpeedUnits->hide();
		if (menuSpeedType)    menuSpeedType->hide();
		if (menuMinSpeed)     menuMinSpeed->hide();
	}

	// Temperature
	if (cfg.temperatureEnable)
	{
		if (menuTemperatureUnits) menuTemperatureUnits->unhide();
		if (menuTemperatureType)  menuTemperatureType->unhide();
		if (!cfg.temperatureReal)
		{
			if (menuMinTemperature) menuMinTemperature->unhide();
			if (menuMaxTemperature) menuMaxTemperature->unhide();
		}
		else
		{
			if (menuMinTemperature) menuMinTemperature->hide();
			if (menuMaxTemperature) menuMaxTemperature->hide();
		}
	}
	else
	{
		if (menuTemperatureUnits) menuTemperatureUnits->hide();
		if (menuTemperatureType)  menuTemperatureType->hide();
		if (menuMinTemperature)   menuMinTemperature->hide();
		if (menuMaxTemperature)   menuMaxTemperature->hide();
	}

	// Direction Visibility
	if (cfg.directionEnable) {
		if (menuDirectionName1) menuDirectionName1->unhide();
		if (menuDirectionName2) menuDirectionName2->unhide();
	} else {
		if (menuDirectionName1) menuDirectionName1->hide();
		if (menuDirectionName2) menuDirectionName2->hide();
	}

	// Rail Name Visibility
	if (cfg.railNameEnable) {
		if (menuRailName) menuRailName->unhide();
	} else {
		if (menuRailName) menuRailName->hide();
	}

	// Defect Rate Visibilities
	if (cfg.defectHotJournalEnable) {
		if (menuHotJournalRate) menuHotJournalRate->unhide();
	} else {
		if (menuHotJournalRate) menuHotJournalRate->hide();
	}

	if (cfg.defectHotWheelEnable) {
		if (menuHotWheelRate) menuHotWheelRate->unhide();
	} else {
		if (menuHotWheelRate) menuHotWheelRate->hide();
	}

	if (cfg.defectHighImpactWheelEnable) {
		if (menuHighImpactWheelRate) menuHighImpactWheelRate->unhide();
	} else {
		if (menuHighImpactWheelRate) menuHighImpactWheelRate->hide();
	}

	if (cfg.defectDraggingEquipmentEnable) {
		if (menuDraggingEquipmentRate) menuDraggingEquipmentRate->unhide();
	} else {
		if (menuDraggingEquipmentRate) menuDraggingEquipmentRate->hide();
	}

	if (cfg.defectHighLoadEnable) {
		if (menuHighLoadRate) menuHighLoadRate->unhide();
	} else {
		if (menuHighLoadRate) menuHighLoadRate->hide();
	}

	if (cfg.defectWideLoadEnable) {
		if (menuWideLoadRate) menuWideLoadRate->unhide();
	} else {
		if (menuWideLoadRate) menuWideLoadRate->hide();
	}
}

std::shared_ptr<Menu> createAppMenu(DetectorConfiguration &cfg, DisplayLcd *lcd, DataBundle* data, MessageBundle &trackMessages)
{
	// Create the root home menu and main branch
	auto home = std::make_shared<MenuHome>("Home", cfg, data);
	auto mainSel = std::make_shared<MenuListSelector>("Main");
	home->addChild(mainSel);

	// Milepost
	auto menuMilepostConfig = std::make_shared<MenuListSelector>("Milepost");
	auto menuMilepostEn = std::make_shared<MenuBoolSelector>(
		"Milepost Enable",
		&cfg.milepostEnable, 
		false, 
		"On", "ON", 
		"Off", "OFF"
	);
	menuMilepost = std::make_shared<MenuDigitThumbwheel>(
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
	menuTrackNameA = std::make_shared<MenuOptionSelector>(
		"Track A Name", 
		&cfg.trackNameId[0],
		false,
		trackNames,
		[&cfg]() { saveConfiguration(&cfg); updateTrackNames(&cfg); }
	);

	menuTrackNameB = std::make_shared<MenuOptionSelector>(
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
	menuEntranceAxles = std::make_shared<MenuNumberDial>(
		"Entrance Axles",
		&cfg.entranceAxles,
		false,
		0,   // min
		10,  // max
		1,   // step
		"",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	menuMinAxles = std::make_shared<MenuNumberDial>(
		"Minimum Axles",
		&cfg.minAxles,
		false,
		0,   // min
		100, // max
		1,   // step
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
	menuSpeedType = std::make_shared<MenuBoolSelector>(
		"Enter/Exit",
		&cfg.speedTypeEnter,
		false, 
		"Entrance Speed", "ENTR",
		"Exit Speed", "EXIT", 
		[&cfg]() { saveConfiguration(&cfg); }
	);
	menuMinSpeed = std::make_shared<MenuNumberDial>(
		"Minimum Speed",
		&cfg.minSpeed,
		false,
		0,   // min
		50,  // max
		1,   // step
		cfg.speedUnitsMph ? "mph" : "kph",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	menuSpeedUnits = std::make_shared<MenuBoolSelector>(
		"Units",
		&cfg.speedUnitsMph,
		false, 
		"Miles/Hour", "MPH",
		"Kilometers/Hour", "KPH", 
		[&cfg]() { saveConfiguration(&cfg); std::static_pointer_cast<MenuNumberDial>(menuMinSpeed)->setUnits(cfg.speedUnitsMph ? "mph" : "kph"); }
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
	menuDirectionName1 = std::make_shared<MenuOptionSelector>(
		"Direction 1 Name", 
		&cfg.direction1NameId,
		false,
		directionNames,
		[&cfg]() { saveConfiguration(&cfg); updateDirectionNames(&cfg); }
	);
	menuDirectionName2 = std::make_shared<MenuOptionSelector>(
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
	menuRailName = std::make_shared<MenuOptionSelector>(
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
	menuHotJournalRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHotJournalAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectHotJournalAxleRate == 0) {
				cfg.defectHotJournalAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
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
	menuHotWheelRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHotWheelAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectHotWheelAxleRate == 0) {
				cfg.defectHotWheelAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
	);

	// High Impact Wheel
	auto menuHighImpactWheel = std::make_shared<MenuListSelector>("High Impact Wheel");
	auto menuHighImpactWheelEn = std::make_shared<MenuBoolSelector>(
		"High Impact Enable",
		&cfg.defectHighImpactWheelEnable,
		false,
		"On", "ON",
		"Off", "OFF"
	);
	menuHighImpactWheelRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHighImpactWheelAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectHighImpactWheelAxleRate == 0) {
				cfg.defectHighImpactWheelAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
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
	menuDraggingEquipmentRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectDraggingEquipmentAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectDraggingEquipmentAxleRate == 0) {
				cfg.defectDraggingEquipmentAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
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
	menuHighLoadRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectHighLoadAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectHighLoadAxleRate == 0) {
				cfg.defectHighLoadAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
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
	menuWideLoadRate = std::make_shared<MenuDigitThumbwheel>(
		"Axle Rate",
		&cfg.defectWideLoadAxleRate,
		false,
		6,
		0,
		true,
		[&cfg, &trackMessages]() {
			if (cfg.defectWideLoadAxleRate == 0) {
				cfg.defectWideLoadAxleRate = 1;
			}
			saveConfiguration(&cfg);
			setDefaultMessages(trackMessages, cfg);
		}
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
		9,   // max
		1,   // step
		"defects",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuOrdinalType = std::make_shared<MenuOptionSelector>(
		"Ordinal Type", 
		&cfg.ordinalTypeId,
		false,
		ordinalTypes,
		[&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); }
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
	menuTemperatureType = std::make_shared<MenuBoolSelector>(
		"Real/Simulated",
		&cfg.temperatureReal,
		false, 
		"Real", "REAL",
		"Simulated", "SIM" 
	);
	std::string degF("\xDF" "F", 2);
	std::string degC("\xDF" "C", 2);
	menuMinTemperature = std::make_shared<MenuNumberDial>(
		"Minimum Temp",
		[&cfg]() { return getMinTemperature(&cfg); },
		[&cfg](int16_t val) { setMinTemperature(&cfg, val); },
		false,
		-99,   // min
		150,  // max
		1,    // step
		cfg.temperatureUnitsF ? degF : degC,
		[&cfg]() { saveConfiguration(&cfg); }
	);
	menuMaxTemperature = std::make_shared<MenuNumberDial>(
		"Maximum Temp",
		[&cfg]() { return getMaxTemperature(&cfg); },
		[&cfg](int16_t val) { setMaxTemperature(&cfg, val); },
		false,
		-99,   // min
		150,  // max
		1,    // step
		cfg.temperatureUnitsF ? degF : degC,
		[&cfg]() { saveConfiguration(&cfg); }
	);
	menuTemperatureUnits = std::make_shared<MenuBoolSelector>(
		"Units",
		&cfg.temperatureUnitsF,
		false, 
		"Fahrenheit", degF,
		"Celsius", degC, 
		[&cfg, degF, degC]() {
			saveConfiguration(&cfg);
			std::static_pointer_cast<MenuNumberDial>(menuMinTemperature)->setUnits(cfg.temperatureUnitsF ? degF : degC);
			std::static_pointer_cast<MenuNumberDial>(menuMaxTemperature)->setUnits(cfg.temperatureUnitsF ? degF : degC);
		}
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
		"Detector Timeout",
		&cfg.detectorTimeout,
		false,
		2,   // min
		30,  // max
		1,   // step
		"sec",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuSummaryDisplayTime = std::make_shared<MenuNumberDial>(
		"Summary Disp Time",
		&cfg.summaryDisplayTime,
		false,
		2,   // min
		60,  // max
		1,   // step
		"sec",
		[&cfg]() { saveConfiguration(&cfg); }
	);
	auto menuMsgRepeatTimeout = std::make_shared<MenuNumberDial>(
		"Msg Repeat Timeout",
		&cfg.msgRepeatTimeout,
		false,
		2,   // min
		60,  // max
		1,   // step
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
	auto menuReset = std::make_shared<MenuReset>("Factory Reset", cfg);

	// ==========================================
	// Assign Button Callbacks
	// ==========================================
	menuMilepostEn->setSaveCallback([&cfg]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); });
	menuTrackNameEn->setSaveCallback([&cfg]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); updateTrackNames(&cfg); });
	menuAxleEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuSpeedEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuTemperatureEn->setSaveCallback([&cfg]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); });
	menuTemperatureType->setSaveCallback([&cfg]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); });
	menuDirectionEn->setSaveCallback([&cfg]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); updateDirectionNames(&cfg); });
	menuRailNameEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); updateRailNames(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuEntranceMessage->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuAlertMessage->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });
	menuTalkDefectOnly->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); setDefaultMessages(trackMessages, cfg); });

	// Defect Enable Callbacks
	menuHotJournalEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuHotWheelEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuHighImpactWheelEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuDraggingEquipmentEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuHighLoadEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });
	menuWideLoadEn->setSaveCallback([&cfg, &trackMessages]() { saveConfiguration(&cfg); updateAllMenuVisibility(cfg); setDefaultMessages(trackMessages, cfg); });

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
	menuExitMessage->addChild(menuOrdinalType);

	mainSel->addChild(menuTemperatureConfig);
	menuTemperatureConfig->addChild(menuTemperatureEn);
	menuTemperatureConfig->addChild(menuTemperatureUnits);
	menuTemperatureConfig->addChild(menuTemperatureType);
	menuTemperatureConfig->addChild(menuMinTemperature);
	menuTemperatureConfig->addChild(menuMaxTemperature);

	mainSel->addChild(menuOperationMode);

	mainSel->addChild(menuTimingConfig);
	menuTimingConfig->addChild(menuDetectorTimeout);
	menuTimingConfig->addChild(menuSummaryDisplayTime);
	menuTimingConfig->addChild(menuMsgRepeatTimeout);

	mainSel->addChild(menuAudio);
	menuAudio->addChild(menuVolume);
	menuAudio->addChild(menuNoise);
	menuAudio->addChild(menuPopcorn);

	mainSel->addChild(menuSysConfig);
	menuSysConfig->addChild(menuBacklightLevel);
	menuSysConfig->addChild(menuPttDelay);

	mainSel->addChild(menuDiagnostics);
	menuDiagnostics->addChild(menuVocabTest);
	menuDiagnostics->addChild(menuReset);

	updateAllMenuVisibility(cfg);
	
	return home;
}