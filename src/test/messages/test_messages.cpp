#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// Define macro to substitute the embedded Arduino header with our local mock header
#include "Arduino.h"

// Instantiate the mock serial object
MockSerial Serial;
TwoWire Wire; // Changed from MockWire to TwoWire

// Include production headers directly
#include "common.h"
#include "messages.h"

// --- ANSI Escape Codes for Color Output ---
#define ANSI_GREEN "\033[32m"
#define ANSI_RED   "\033[31m"
#define ANSI_RESET "\033[0m"

// --- Unit Test Cases Configuration ---
struct TestCase {
	std::string input;
	std::string expected;
	bool breakDigits;
	uint8_t trackNum;
	uint8_t direction;
};

const std::vector<TestCase> testCases = {
	{ "#milepost", "3 4 6 . 9", true, 0, 0 },
	{ "#milepost", "346.9", false, 0, 0 },
	{ "#milepost:3.1", "3 4 6 . 9", true, 0, 0 },
	{ "#milepost:4.1", " 346.9", false, 0, 0 },
	{ "#milepost:-4.1", "346.9 ", false, 0, 0 },
	{ "#milepost:6.1", "   346.9", false, 0, 0 },
	{ "#milepost:-6.1", "346.9   ", false, 0, 0 },
	{ "#axle", "4", false, 0, 0 },
	{ "#axle:3", "  4", false, 0, 0 },
	{ "#axles:-3", "1 2 4", true, 0, 0 },
	{ "#track", "Main 1", false, 0, 0 },
	{ "#track:4", "Main", false, 0, 0 },
	{ "#track:8", "  Main 1", false, 0, 0 },
	{ "#track:-8", "Main 1  ", false, 0, 0 },
	{ "#direction", "Northbound", false, 0, 1 },
	{ "#direction:4", "Sout", false, 0, 2 },
	{ "#direction:12", "  Northbound", false, 0, 1 },
	{ "#direction:-12", "Southbound  ", false, 0, 2 },
	{ "#direction", "", false, 0, 0 },
	{ "#direction", "", false, 0, 12 },
	{ "#speed", "4 5", true, 0, 0 },
	{ "#temp", "72", false, 0, 0 },
	{ "#defectlist", "HOT_BOX DRAG_EQ", false, 0, 0 },
	{ "milepost #milepost speed #speed:3 mph", "milepost 346.9 speed  45 mph", false, 0, 0 },
	{ "Axles:#axles", "Axles:124", false, 0},
	{ "Axles:#axles\\Speed:#speed", "Axles:124\nSpeed:45", false, 0, 0 },
	{ "Axles:\\\\#axles", "Axles:\\124", false, 0, 0 }
};

int main() {
	DetectorConfiguration cfg;
	cfg.milepost = 3469;
	cfg.trackName[0] = "Main 1";
	cfg.trackName[1] = "Track 2";
	cfg.direction1Name = "Northbound";
	cfg.direction2Name = "Southbound";

	DataBundle data;
	data.axleCount = 4;
	data.totalAxles = 124;
	data.speed = 45;
	data.defects = {"HOT_BOX", "DRAG_EQ"};

	int passed = 0;
	int failed = 0;

	std::cout << "========================================" << std::endl;
	std::cout << "RUNNING CHOSEN TRANSFORM_MESSAGE TESTS  " << std::endl;
	std::cout << "========================================" << std::endl;

	for (size_t i = 0; i < testCases.size(); ++i) {
		const auto& tc = testCases[i];
		
		// Directly invoking the production method from messages.h
		std::string result;
		data.direction = tc.direction;
		transformMessage(tc.input, result, cfg, data, tc.trackNum, tc.breakDigits);

		bool isPass = (result == tc.expected);
		if (isPass) passed++; else failed++;

		std::cout << "Test #" << i + 1 << "\n";
		std::cout << "  Input   : \"" << tc.input << "\"\n";
		std::cout << "  Expected: \"" << tc.expected << "\"\n";
		std::cout << "  Result  : \"" << result << "\"\n";
		std::cout << "  Status  : ";
		if (isPass) {
			std::cout << ANSI_GREEN << "PASS" << ANSI_RESET << "\n";
		} else {
			std::cout << ANSI_RED << "FAIL" << ANSI_RESET << "\n";
		}
		std::cout << "----------------------------------------" << std::endl;
	}

	std::cout << "\nTest Summary: ";
	if (failed == 0) {
		std::cout << ANSI_GREEN << passed << " passed, " << failed << " failed." << ANSI_RESET << std::endl;
	} else {
		std::cout << ANSI_RED << passed << " passed, " << failed << " failed." << ANSI_RESET << std::endl;
	}

	std::cout << "\n========================================" << std::endl;
	std::cout << "RUNNING GET_UNIQUE_WORDS CONFIG TESTS   " << std::endl;
	std::cout << "========================================" << std::endl;

	MessageBundle testMsgs;
	testMsgs.entranceMsg = "Equipment Defect Detector";

	DetectorConfiguration customCfg;
	customCfg.trackName[0] = "Front Range";
	customCfg.trackName[1] = "Siding 4";
	customCfg.direction1Name = "Outbound";
	customCfg.direction2Name = "Inbound";
	customCfg.railName = "Port/Starboard";

	std::vector<std::string> words = getUniqueWords(testMsgs, customCfg);

	auto hasWord = [&words](const std::string& target) {
		return std::find(words.begin(), words.end(), target) != words.end();
	};

	std::vector<std::string> expectedWords = {
		"front", "range", "siding", "4", "outbound", "inbound", "port", "starboard",
		"equipment", "defect", "detector"
	};

	int wordTestPassed = 0;
	int wordTestFailed = 0;

	for (const auto& w : expectedWords) {
		bool present = hasWord(w);
		std::cout << "Check word: \"" << w << "\" -> ";
		if (present) {
			std::cout << ANSI_GREEN << "PASS" << ANSI_RESET << "\n";
			wordTestPassed++;
		} else {
			std::cout << ANSI_RED << "FAIL" << ANSI_RESET << "\n";
			wordTestFailed++;
		}
	}

	// Also verify backwards compatibility overload without cfg
	std::vector<std::string> baseWords = getUniqueWords(testMsgs);
	bool baseHasBuiltin = (std::find(baseWords.begin(), baseWords.end(), "point") != baseWords.end());
	std::cout << "Check base getUniqueWords without cfg -> ";
	if (baseHasBuiltin) {
		std::cout << ANSI_GREEN << "PASS" << ANSI_RESET << "\n";
		wordTestPassed++;
	} else {
		std::cout << ANSI_RED << "FAIL" << ANSI_RESET << "\n";
		wordTestFailed++;
	}

	std::cout << "\nUnique Words Test Summary: "
	          << (wordTestFailed == 0 ? ANSI_GREEN : ANSI_RED)
	          << wordTestPassed << " passed, " << wordTestFailed << " failed."
	          << ANSI_RESET << std::endl;

	if (wordTestFailed > 0) failed++;

	return (failed == 0) ? 0 : 1;
}