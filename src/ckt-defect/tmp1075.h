#pragma once

#include <Arduino.h>
#include <Wire.h>

// Default I2C Address for TMP1075 (A0, A1, A2 connected to GND)
#define TMP1075_DEFAULT_ADDRESS 0x48

// Register Pointers
#define TMP1075_REG_TEMP        0x00
#define TMP1075_REG_CONFIG      0x01

class TMP1075 {
public:
    TMP1075(uint8_t address = TMP1075_DEFAULT_ADDRESS);

    // Initialization
    bool begin(TwoWire &wirePort = Wire);

    // Core functions
    float readTemperatureC();
    float readTemperatureF();
    
    // Power management
    void shutdown(bool enable);

private:
    uint8_t _address;
    TwoWire *_i2cPort;
    
    int16_t readRegister16(uint8_t reg);
    void writeRegister16(uint8_t reg, uint16_t value);
};

