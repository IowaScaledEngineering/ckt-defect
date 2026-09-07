#pragma once
#include <iostream>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <cstdint>

// FreeRTOS Mock Stubs
inline uint32_t uxTaskGetStackHighWaterMark(void*) { return 4096; }
inline uint32_t xPortGetFreeHeapSize(void) { return 128000; }

// Simulating Arduino Serial Object
class MockSerial {
public:
    void print(const char* s) { std::cout << s; }
    void print(int n) { std::cout << n; }
    void println(const char* s) { std::cout << s << "\n"; }
    void println(int n) { std::cout << n << "\n"; }

    // Implement printf for MockSerial
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};
extern MockSerial Serial;

// Mock the real Arduino TwoWire class name
class TwoWire {
public:
    void begin() {}
    void beginTransmission(int) {}
    int endTransmission() { return 0; }
    void write(int) {}
    int read() { return 0; }
    void requestFrom(int, int) {}
};
extern TwoWire Wire;

// Mock TemperatureManager completely inline here so messages.cpp can see it
class TemperatureManager {
private:
    inline static TemperatureManager* _instance = nullptr;
public:
    static TemperatureManager* getInstance() {
        if (!_instance) {
            static TemperatureManager instance;
            _instance = &instance;
        }
        return _instance;
    }
    // Added const to exactly match what messages.cpp expects
    double getTemperature() const { return 72.0; } 
};