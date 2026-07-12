#include <iostream>
#include <vector>
#include <string>
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
};

const std::vector<TestCase> testCases = {
	{ "#milepost", "3 4 6 . 9", true, 0 },
	{ "#milepost", "346.9", false, 0 },
	{ "#milepost:3.1", "3 4 6 . 9", true, 0 },
	{ "#milepost:4.1", " 346.9", false, 0 },
	{ "#milepost:-4.1", "346.9 ", false, 0 },
	{ "#milepost:6.1", "   346.9", false, 0 },
	{ "#milepost:-6.1", "346.9   ", false, 0 },
	{ "#axle", "4", false, 0 },
	{ "#axle:3", "  4", false, 0 },
	{ "#axles:-3", "1 2 4", true, 0 },
	{ "#track", "Main 1", false, 0 },
	{ "#track:4", "Main", false, 0 },
	{ "#track:8", "  Main 1", false, 0 },
	{ "#track:-8", "Main 1  ", false, 0 },
	{ "#speed", "4 5", true, 0 },
	{ "#temp", "72", false, 0 },
	{ "#defectlist", "HOT_BOX DRAG_EQ", false, 0 },
	{ "milepost #milepost speed #speed:3 mph", "milepost 346.9 speed  45 mph", false, 0 }
};

int main() {
	DetectorConfiguration cfg;
	cfg.milepost = 3469;
	cfg.trackName[0] = "Main 1";
	cfg.trackName[1] = "Track 2";

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

	return (failed == 0) ? 0 : 1;
}