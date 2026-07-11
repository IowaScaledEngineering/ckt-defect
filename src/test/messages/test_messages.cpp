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

// Provide the local mock implementations of dependencies from common.cpp and temperature.h
void toLowercase(std::string& str) {
    for (char &c : str) {
        c = std::tolower(c);
    }
}

std::string intToString(int32_t intVal, uint32_t integerDigits, uint32_t fractionalDigits) {
    bool isNegative = (intVal < 0);
    uint32_t absVal = isNegative ? static_cast<uint32_t>(-intVal) : static_cast<uint32_t>(intVal);
    std::string numStr = std::to_string(absVal);
    std::string formatted;

    if (fractionalDigits >= numStr.length()) {
        size_t leadingZeros = fractionalDigits - numStr.length() + 1;
        formatted.append(leadingZeros, '0');
        formatted.insert(1, 1, '.');
        formatted.append(numStr);
    } else {
        formatted = numStr;
        if (fractionalDigits > 0) {
            formatted.insert(formatted.length() - fractionalDigits, 1, '.');
        }
    }

    if (isNegative) {
        formatted.insert(0, 1, '-');
    }

    size_t currentIntLength = formatted.find('.');
    if (currentIntLength == std::string::npos) {
        currentIntLength = formatted.length();
    }

    if (currentIntLength < integerDigits) {
        formatted.insert(0, integerDigits - currentIntLength, ' ');
    }

    return formatted;
}

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
    { "#track", "main 1", false, 0 },
    { "#track:4", "main", false, 0 },
    { "#track:8", "  main 1", false, 0 },
    { "#track:-8", "main 1  ", false, 0 },
    { "#speed", "4 5", true, 0 },
    { "#temp", "72", false, 0 },
    { "#defectlist", "hot_box drag_eq", false, 0 },
    { "milepost #milepost speed #speed:3 mph", "milepost 346.9 speed  45 mph", false, 0 }
};

int main() {
    DetectorConfiguration cfg;
    cfg.milepost = 3469;
    cfg.trackName[0] = "Main 1";
    cfg.trackName[1] = "Track 2";

    DataBundle data;
    data.defectAxle = 4;
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
        std::string* resultPtr = transformMessage(tc.input, cfg, data, tc.trackNum, tc.breakDigits);
        std::string result = *resultPtr;
        delete resultPtr;

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