#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include "configuration.h"
#include "messages.h"
#include "config-file.h"

// Stubs for common variables needed by configuration.h / messages.h
const std::vector<std::string> trackNames = {"Main 1", "Main 2", "Siding"};
const std::vector<std::string> directionNames = {"Eastbound", "Westbound"};
const std::vector<std::string> railNames = {"North/South", "East/West"};
const std::vector<std::string> ordinalTypes = {"1st", "First"};
const std::vector<std::string> ordinalStrings = {"first", "second"};

static int passedTests = 0;
static int failedTests = 0;

#define TEST_ASSERT(cond, msg) \
	do { \
		if (!(cond)) { \
			std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
			failedTests++; \
		} else { \
			passedTests++; \
		} \
	} while(0)

void testValidGlobalConfig()
{
	std::cout << "Running testValidGlobalConfig...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"# General Hardware & Identity\n"
		"trackName1 = Main 1\n"
		"trackName2 = Siding\n"
		"direction1Name = Eastbound\n"
		"direction2Name = Westbound\n"
		"railName = North/South\n"
		"vocabSelected = custom_voice\n"
		"integrityTrainRate = 250\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Should succeed on valid global config");
	TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track 1 flag set");
	TEST_ASSERT(cfg.trackName[0] == "Main 1", "Track 1 name matches");
	TEST_ASSERT(cfg.externalTrackNamePresent[1], "Track 2 flag set");
	TEST_ASSERT(cfg.trackName[1] == "Siding", "Track 2 name matches");
	TEST_ASSERT(cfg.externalDirection1NamePresent, "Dir 1 flag set");
	TEST_ASSERT(cfg.direction1Name == "Eastbound", "Dir 1 name matches");
	TEST_ASSERT(cfg.externalDirection2NamePresent, "Dir 2 flag set");
	TEST_ASSERT(cfg.direction2Name == "Westbound", "Dir 2 name matches");
	TEST_ASSERT(cfg.externalRailNamePresent, "Rail flag set");
	TEST_ASSERT(cfg.railName == "North/South", "Rail name matches");
	TEST_ASSERT(cfg.externalVocabSelectedPresent, "Vocab flag set");
	TEST_ASSERT(cfg.vocabSelected == "custom_voice", "Vocab matches");
	TEST_ASSERT(cfg.externalIntegrityTrainRatePresent, "Integrity rate flag set");
	TEST_ASSERT(msgs.integrityTrainRate == 250, "Integrity rate matches");
	TEST_ASSERT(!cfg.externalMessagesPresent, "No messages defined yet");
}

void testTrackNameAliases()
{
	std::cout << "Running testTrackNameAliases...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"trackNameA = Main A\n"
		"TRACKNAMEB = Main B\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Should succeed with trackNameA and trackNameB aliases");
	TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track A flag set");
	TEST_ASSERT(cfg.trackName[0] == "Main A", "Track A value matches");
	TEST_ASSERT(cfg.externalTrackNamePresent[1], "Track B flag set");
	TEST_ASSERT(cfg.trackName[1] == "Main B", "Track B value matches");
}

void testValidMessagesAndDefects()
{
	std::cout << "Running testValidMessagesAndDefects...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"entranceMsg = Equipment Defect Detector MP #milepost #track\n"
		"exitCleanMsg = No defects total axles #axles detector out\n"
		"exitCleanDisplayMsg = #track\\NO DEFECTS\\AXLES:#axles:3\n"
		"exitDefectMsg = You have a defect #defectlist\n"
		"exitDefectDisplayMsg = #track\\DEFECT!\\AXLES:#axles:3\n"
		"integrityMsg = Integrity failure\n"
		"integrityDisplayMsg = #track\\INTEGRITY FAILURE\\\n"
		"minAxleMsg = Minimum axles not met\n"
		"minAxleDisplayMsg = #track\\TOO FEW AXLES\\\n"
		"tooSlowMsg = Train speed too slow\n"
		"tooSlowDisplayMsg = #track\\TOO SLOW\\\n"
		"detectorBlockedMsg = Detector blocked\n"
		"detectorBlockedDisplayMsg = #track\\BLOCKED\\\n"
		"excessAlarmsMsg = Excessive alarms\n"
		"\n"
		"[defect]\n"
		"alertMsg   = #tone #track hot journal Axle #axle\n"
		"displayMsg = #track\\Hot Journal\\Axle #axle\n"
		"detailMsg  = #ord hot journal Axle #axle\n"
		"axleRate   = 125\n"
		"\n"
		"[defect]\n"
		"alertMsg   = #tone #track dragging equipment\n"
		"displayMsg = #track\\Dragging Equip\\\n"
		"detailMsg  = #ord dragging equipment\n"
		"axleRate   = 250\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Should succeed with valid messages and defects");
	TEST_ASSERT(cfg.externalMessagesPresent, "externalMessagesPresent must be true");
	TEST_ASSERT(msgs.entranceMsg == "Equipment Defect Detector MP #milepost #track", "entranceMsg matches");
	TEST_ASSERT(msgs.exitCleanMsg == "No defects total axles #axles detector out", "exitCleanMsg matches");
	TEST_ASSERT(msgs.excessAlarmsMsg == "Excessive alarms", "excessAlarmsMsg matches");
	TEST_ASSERT(msgs.defects.size() == 2, "Exactly 2 defects added");
	TEST_ASSERT(msgs.defects[0].axleRate == 125, "Defect 1 rate is 125");
	TEST_ASSERT(msgs.defects[0].alertMsg == "#tone #track hot journal Axle #axle", "Defect 1 alertMsg");
	TEST_ASSERT(msgs.defects[1].axleRate == 250, "Defect 2 rate is 250");
	TEST_ASSERT(msgs.defects[1].detailMsg == "#ord dragging equipment", "Defect 2 detailMsg");
}

void testCommentsAndWhitespace()
{
	std::cout << "Running testCommentsAndWhitespace...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"   # Full line comment with hash\n"
		"   ; Full line comment with semicolon\n"
		"\n"
		"   trackName1    =   Main Track   ; Inline comment with semicolon\n"
		"   entranceMsg   =   Defect Detector #track   \n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Comments and whitespace handled cleanly");
	TEST_ASSERT(cfg.trackName[0] == "Main Track", "Whitespace trimmed and semicolon comment stripped");
	TEST_ASSERT(msgs.entranceMsg == "Defect Detector #track", "Hash inside message preserved as token");
}

void testMissingEqualsError()
{
	std::cout << "Running testMissingEqualsError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"trackName1 = Main 1\n"
		"invalid line without equals\n"
		"trackName2 = Siding\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on line without '='");
	TEST_ASSERT(res.errorLine == 2, "Error at line 2");
	TEST_ASSERT(res.errorTitle == "Syntax Error", "Title is Syntax Error");
}

void testUnknownGlobalKeyError()
{
	std::cout << "Running testUnknownGlobalKeyError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"trackName1 = Main 1\n"
		"unknownSetting = 12345\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on unknown key");
	TEST_ASSERT(res.errorLine == 2, "Error at line 2");
	TEST_ASSERT(res.errorTitle == "Unknown Key", "Title is Unknown Key");
}

void testIncompleteDefectError()
{
	std::cout << "Running testIncompleteDefectError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	// Omitted axleRate
	std::string config = 
		"[defect]\n"
		"alertMsg   = #tone #track defect\n"
		"displayMsg = #track\\Defect\\\n"
		"detailMsg  = #ord defect\n"
		"\n"
		"# Missing axleRate before EOF\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on incomplete defect");
	TEST_ASSERT(res.errorTitle == "Incomplete Defect", "Title is Incomplete Defect");
	TEST_ASSERT(res.errorDetail == "Missing axleRate", "Identifies missing axleRate");
}

void testDuplicateDefectKeyError()
{
	std::cout << "Running testDuplicateDefectKeyError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"[defect]\n"
		"alertMsg = first alert\n"
		"alertMsg = duplicate alert\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on duplicate key");
	TEST_ASSERT(res.errorLine == 3, "Error at line 3");
	TEST_ASSERT(res.errorTitle == "Duplicate Key", "Title is Duplicate Key");
}

void testDefectKeyOutsideDefectError()
{
	std::cout << "Running testDefectKeyOutsideDefectError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"alertMsg = spoken phrase\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail when defect key used outside [defect]");
	TEST_ASSERT(res.errorLine == 1, "Error at line 1");
	TEST_ASSERT(res.errorTitle == "Invalid Context", "Title is Invalid Context");
}

void testIntegerRangeErrors()
{
	std::cout << "Running testIntegerRangeErrors...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	// axleRate = 0 (below min 1)
	std::string config1 = 
		"[defect]\n"
		"alertMsg   = alert\n"
		"displayMsg = disp\n"
		"detailMsg  = detail\n"
		"axleRate   = 0\n";
	ConfigFileResult res1 = parseConfigurationString(config1, cfg, msgs);
	TEST_ASSERT(!res1.success, "Should fail on axleRate 0");
	TEST_ASSERT(res1.errorTitle == "Out of Range", "Title is Out of Range");

	// axleRate = 1000000 (above max 999999)
	std::string config2 = 
		"[defect]\n"
		"alertMsg   = alert\n"
		"displayMsg = disp\n"
		"detailMsg  = detail\n"
		"axleRate   = 1000000\n";
	ConfigFileResult res2 = parseConfigurationString(config2, cfg, msgs);
	TEST_ASSERT(!res2.success, "Should fail on axleRate 1000000");
	TEST_ASSERT(res2.errorTitle == "Out of Range", "Title is Out of Range");

	// integrityTrainRate = 1000000 (above max 999999)
	std::string config3 = "integrityTrainRate = 1000000\n";
	ConfigFileResult res3 = parseConfigurationString(config3, cfg, msgs);
	TEST_ASSERT(!res3.success, "Should fail on integrityTrainRate 1000000");
	TEST_ASSERT(res3.errorTitle == "Out of Range", "Title is Out of Range");

	// Non-numeric integer
	std::string config4 = "integrityTrainRate = not_a_number\n";
	ConfigFileResult res4 = parseConfigurationString(config4, cfg, msgs);
	TEST_ASSERT(!res4.success, "Should fail on non-numeric integer");
	TEST_ASSERT(res4.errorTitle == "Invalid Integer", "Title is Invalid Integer");
}

void testIllegalCharacterError()
{
	std::cout << "Running testIllegalCharacterError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = "entranceMsg = Defect Detector MP %invalid%\n";
	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on '%' illegal character");
	TEST_ASSERT(res.errorTitle == "Illegal Character", "Title is Illegal Character");
}

void testStringTooLongError()
{
	std::cout << "Running testStringTooLongError...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	// 17 characters is > 16 max
	std::string config = "trackName1 = 12345678901234567\n";
	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(!res.success, "Should fail on track name > 16 chars");
	TEST_ASSERT(res.errorTitle == "String Too Long", "Title is String Too Long");
}

void testAllOrNothingDefaults()
{
	std::cout << "Running testAllOrNothingDefaults...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	// Only set entranceMsg in config.txt
	std::string config = "entranceMsg = Defect Detector Inbound\n";
	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Should succeed with only entranceMsg");
	TEST_ASSERT(cfg.externalMessagesPresent, "externalMessagesPresent must be true");
	TEST_ASSERT(msgs.entranceMsg == "Defect Detector Inbound", "entranceMsg matches");
	TEST_ASSERT(msgs.exitCleanMsg.empty(), "exitCleanMsg remains empty/silent");
	TEST_ASSERT(msgs.exitDefectMsg.empty(), "exitDefectMsg remains empty/silent");
	TEST_ASSERT(msgs.defects.empty(), "No defects defined in config");
}

void testCaseInsensitivity()
{
	std::cout << "Running testCaseInsensitivity...\n";
	DetectorConfiguration cfg;
	MessageBundle msgs;

	std::string config = 
		"ENTRANCEMSG = Entrance\n"
		"TrackName1 = Main Track\n"
		"INTEGRITYTRAINRATE = 100\n"
		"[DEFECT]\n"
		"ALERTMSG = Alert\n"
		"DISPLAYMSG = Disp\n"
		"DETAILMSG = Detail\n"
		"AXLERATE = 50\n";

	ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
	TEST_ASSERT(res.success, "Case insensitive keys and section header parse successfully");
	TEST_ASSERT(msgs.entranceMsg == "Entrance", "Preserves value case");
	TEST_ASSERT(cfg.trackName[0] == "Main Track", "Preserves value case");
	TEST_ASSERT(msgs.integrityTrainRate == 100, "Integrity rate parses");
	TEST_ASSERT(msgs.defects.size() == 1, "Defect block parsed");
	TEST_ASSERT(msgs.defects[0].axleRate == 50, "Defect axleRate is 50");
}

void testDocConfigFile()
{
	std::cout << "Running testDocConfigFile...\n";
	const char* candidatePaths[] = {
		"doc/config.txt",
		"../doc/config.txt",
		"../../doc/config.txt",
		"../../../doc/config.txt"
	};

	std::ifstream f;
	for (const char* path : candidatePaths)
	{
		f.open(path);
		if (f.is_open())
		{
			break;
		}
	}

	TEST_ASSERT(f.is_open(), "Could not open doc/config.txt");
	if (!f.is_open()) return;

	DetectorConfiguration cfg;
	MessageBundle msgs;
	ConfigFileResult res = parseConfigurationStream(f, cfg, msgs);
	TEST_ASSERT(res.success, "doc/config.txt failed to parse: " + res.errorTitle + " (" + res.errorDetail + ")");
	TEST_ASSERT(cfg.trackName[0] == "Track 1", "trackName[0] match");
	TEST_ASSERT(cfg.trackName[1] == "Track 2", "trackName[1] match");
	TEST_ASSERT(cfg.direction1Name == "Eastbound", "direction1Name match");
	TEST_ASSERT(cfg.direction2Name == "Westbound", "direction2Name match");
	TEST_ASSERT(cfg.railName == "Left/Right", "railName match");
	TEST_ASSERT(msgs.integrityTrainRate == 100, "integrityTrainRate match");
	TEST_ASSERT(msgs.defects.size() == 4, "4 default defects loaded");
	if (msgs.defects.size() == 4)
	{
		TEST_ASSERT(msgs.defects[0].axleRate == 125, "Defect 0 axleRate");
		TEST_ASSERT(msgs.defects[1].axleRate == 125, "Defect 1 axleRate");
		TEST_ASSERT(msgs.defects[2].axleRate == 150, "Defect 2 axleRate");
		TEST_ASSERT(msgs.defects[3].axleRate == 250, "Defect 3 axleRate");
	}
}

void testUtf8BomHandling()
{
	std::cout << "Running testUtf8BomHandling...\n";

	// 1. BOM before comment line
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "\xef\xbb\xbf# UTF-8 BOM comment\ntrackName1 = Main 1\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "Should succeed with BOM before comment");
		TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track name 1 set with BOM comment");
		TEST_ASSERT(cfg.trackName[0] == "Main 1", "Track name 1 matches");
	}

	// 2. BOM directly on key-value pair
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "\xef\xbb\xbfTrackName1 = Main Track\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "Should succeed with BOM on key-value");
		TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track name 1 set with BOM key");
		TEST_ASSERT(cfg.trackName[0] == "Main Track", "Track name matches");
	}

	// 3. BOM directly on section header
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "\xef\xbb\xbf[Defect]\nalertMsg = Hot Box\ndisplayMsg = Hot Box\ndetailMsg = Hot Box\naxleRate = 100\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "Should succeed with BOM on section header");
		TEST_ASSERT(msgs.defects.size() == 1, "One defect parsed with BOM section");
		TEST_ASSERT(msgs.defects[0].alertMsg == "Hot Box", "Defect alertMsg matches");
	}

	// 4. BOM on its own blank line before content
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "\xef\xbb\xbf\ntrackName1 = Siding\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "Should succeed with BOM on empty line");
		TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track name 1 set");
		TEST_ASSERT(cfg.trackName[0] == "Siding", "Track name matches Siding");
	}

	// 5. BOM with leading whitespace
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "  \xef\xbb\xbftrackName1 = Branch\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "Should succeed with BOM preceded by spaces");
		TEST_ASSERT(cfg.externalTrackNamePresent[0], "Track name 1 set");
		TEST_ASSERT(cfg.trackName[0] == "Branch", "Track name matches Branch");
	}
}

void testIntegrityEnableConfig()
{
	std::cout << "Running testIntegrityEnableConfig...\n";

	// 1. Valid booleans true/on/yes/1
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "integrityEnable = on\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "integrityEnable = on should succeed");
		TEST_ASSERT(cfg.externalIntegrityEnablePresent, "externalIntegrityEnablePresent set");
		TEST_ASSERT(cfg.defectIntegrityEnable == true, "defectIntegrityEnable is true");

		std::string config2 = "defectIntegrityEnable = YES\n";
		ConfigFileResult res2 = parseConfigurationString(config2, cfg, msgs);
		TEST_ASSERT(res2.success, "defectIntegrityEnable = YES should succeed");
		TEST_ASSERT(cfg.defectIntegrityEnable == true, "defectIntegrityEnable is true");

		std::string config3 = "integrityEnable = 1\n";
		ConfigFileResult res3 = parseConfigurationString(config3, cfg, msgs);
		TEST_ASSERT(res3.success, "integrityEnable = 1 should succeed");
		TEST_ASSERT(cfg.defectIntegrityEnable == true, "defectIntegrityEnable is true");
	}

	// 2. Valid booleans false/off/no/0
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "integrityEnable = off\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "integrityEnable = off should succeed");
		TEST_ASSERT(cfg.externalIntegrityEnablePresent, "externalIntegrityEnablePresent set");
		TEST_ASSERT(cfg.defectIntegrityEnable == false, "defectIntegrityEnable is false");

		std::string config2 = "integrityEnable = FALSE\n";
		ConfigFileResult res2 = parseConfigurationString(config2, cfg, msgs);
		TEST_ASSERT(res2.success, "integrityEnable = FALSE should succeed");
		TEST_ASSERT(cfg.defectIntegrityEnable == false, "defectIntegrityEnable is false");

		std::string config3 = "integrityEnable = 0\n";
		ConfigFileResult res3 = parseConfigurationString(config3, cfg, msgs);
		TEST_ASSERT(res3.success, "integrityEnable = 0 should succeed");
		TEST_ASSERT(cfg.defectIntegrityEnable == false, "defectIntegrityEnable is false");
	}

	// 3. Invalid boolean
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "integrityEnable = maybe\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(!res.success, "integrityEnable = maybe should fail");
		TEST_ASSERT(res.errorTitle == "Invalid Boolean", "Title is Invalid Boolean");
	}

	// 4. integrityTrainRate = 0 disables integrity defect
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		cfg.defectIntegrityEnable = true;
		std::string config = "integrityTrainRate = 0\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "integrityTrainRate = 0 should succeed");
		TEST_ASSERT(cfg.externalIntegrityTrainRatePresent, "externalIntegrityTrainRatePresent set");
		TEST_ASSERT(cfg.externalIntegrityEnablePresent, "externalIntegrityEnablePresent set by rate 0");
		TEST_ASSERT(cfg.defectIntegrityEnable == false, "defectIntegrityEnable disabled by rate 0");
		TEST_ASSERT(msgs.integrityTrainRate == 0, "integrityTrainRate is 0");
	}

	// 5. defectIntegrityTrainRate alias
	{
		DetectorConfiguration cfg;
		MessageBundle msgs;
		std::string config = "defectIntegrityTrainRate = 250\n";
		ConfigFileResult res = parseConfigurationString(config, cfg, msgs);
		TEST_ASSERT(res.success, "defectIntegrityTrainRate = 250 should succeed");
		TEST_ASSERT(cfg.externalIntegrityTrainRatePresent, "externalIntegrityTrainRatePresent set by alias");
		TEST_ASSERT(msgs.integrityTrainRate == 250, "integrityTrainRate is 250");

		std::string config2 = "defectIntegrityTrainRate = 0\n";
		ConfigFileResult res2 = parseConfigurationString(config2, cfg, msgs);
		TEST_ASSERT(res2.success, "defectIntegrityTrainRate = 0 should succeed");
		TEST_ASSERT(cfg.defectIntegrityEnable == false, "defectIntegrityEnable disabled by alias rate 0");
		TEST_ASSERT(msgs.integrityTrainRate == 0, "integrityTrainRate is 0");
	}
}

int main()
{
	std::cout << "========================================\n";
	std::cout << "RUNNING CONFIG FILE PARSER UNIT TESTS\n";
	std::cout << "========================================\n";

	testValidGlobalConfig();
	testTrackNameAliases();
	testValidMessagesAndDefects();
	testCommentsAndWhitespace();
	testMissingEqualsError();
	testUnknownGlobalKeyError();
	testIncompleteDefectError();
	testDuplicateDefectKeyError();
	testDefectKeyOutsideDefectError();
	testIntegerRangeErrors();
	testIllegalCharacterError();
	testStringTooLongError();
	testAllOrNothingDefaults();
	testCaseInsensitivity();
	testUtf8BomHandling();
	testIntegrityEnableConfig();
	testDocConfigFile();

	std::cout << "========================================\n";
	std::cout << "Test Summary: " << passedTests << " passed, " << failedTests << " failed.\n";
	std::cout << "========================================\n";

	return (failedTests == 0) ? 0 : 1;
}
