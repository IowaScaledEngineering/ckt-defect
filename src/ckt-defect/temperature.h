#pragma once

#include <Arduino.h>
#include <memory>
#include "tmp1075.h"
#include "configuration.h"

class TemperatureManager {
private:
    TMP1075 _sensor;                         // Internal hardware sensor instance
    DetectorConfiguration* _config;          // Pointer to system configuration struct
    float _currentTemperatureC = 0.0f;       // Cached temperature in Celsius
    bool _isInitialized = false;

    // Simulation IIR Filter State Variables
    float _targetTemperatureC = 0.0f;        // Destination temperature for simulation
    uint32_t _updateCounter = 0;             // Tracks number of calls to update()
    
    // Configurable filter parameters
    float _filterCoefficient = 0.1f;        // Fraction of delta to apply each step
    uint32_t _targetInterval = 30;          // How many updates before picking a new target

    // Helper to generate a new random value matching the configured units
    float generateRandomTargetCelsius();

    // Static pointer for global service discovery
    inline static TemperatureManager* _instance = nullptr;

public:
    // Pass the pointer to the configuration during instantiation
    TemperatureManager(DetectorConfiguration* cfg);
    ~TemperatureManager();

    // Global access point for other files/modules
    static TemperatureManager* getInstance();

    // Standard Arduino / ESP32 initialization routines
    bool begin();

    // Periodic tick function called by the main routine
    void update();

    // Getters to adjust parameters dynamically if desired
    void setFilterParameters(float n, uint32_t m);

    // Temperature getters
    float getTemperatureC() const;
    float getTemperatureF() const;
    float getTemperature() const; // Returns temp based on configured units (F or C)
};
