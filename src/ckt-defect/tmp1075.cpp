#include "tmp1075.h"

TMP1075::TMP1075(uint8_t address)
{
	_address = address;
}

bool TMP1075::begin(TwoWire &wirePort)
{
	_i2cPort = &wirePort;
	
	// Verify device is responding on the I2C bus
	_i2cPort->beginTransmission(_address);
	return (_i2cPort->endTransmission() == 0);
}

float TMP1075::readTemperatureC()
{
	int16_t raw = readRegister16(TMP1075_REG_TEMP);
	
	// TMP1075 returns a 12-bit signed value left-justified in a 16-bit field.
	// The bottom 4 bits are 0. Shift right by 4 to get the actual 12-bit value.
	raw >>= 4;
	
	// Check sign bit for negative temperatures (12-bit two's complement)
	if (raw & 0x0800) {
		raw |= 0xF000; // Sign extend to 16-bit signed integer
	}
	
	// LSB resolution is 0.0625°C
	return (float)raw * 0.0625f;
}

void TMP1075::shutdown(bool enable)
{
	uint16_t config = readRegister16(TMP1075_REG_CONFIG);
	if (enable) {
		config |= 0x0100; // Set SD (Shutdown) bit
	} else {
		config &= ~0x0100; // Clear SD bit
	}
	writeRegister16(TMP1075_REG_CONFIG, config);
}

int16_t TMP1075::readRegister16(uint8_t reg)
{
	_i2cPort->beginTransmission(_address);
	_i2cPort->write(reg);
	_i2cPort->endTransmission(false); // Send repeated start
	
	_i2cPort->requestFrom(_address, (uint8_t)2);
	if (_i2cPort->available() == 2) {
		uint8_t msb = _i2cPort->read();
		uint8_t lsb = _i2cPort->read();
		return (int16_t)((msb << 8) | lsb);
	}
	return 0;
}

void TMP1075::writeRegister16(uint8_t reg, uint16_t value)
{
	_i2cPort->beginTransmission(_address);
	_i2cPort->write(reg);
	_i2cPort->write((uint8_t)(value >> 8));   // MSB
	_i2cPort->write((uint8_t)(value & 0xFF)); // LSB
	_i2cPort->endTransmission();
}
