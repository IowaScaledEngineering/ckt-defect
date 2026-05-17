#pragma once
#include "src/menu/display.h"
#include <Wire.h>
#include <string>

// Core OpenLCD Constants
#define LCD_DEFAULT_ADDRESS 0x72
#define SPECIAL_COMMAND     254
#define SETTING_COMMAND     0x7C
#define CLEAR_COMMAND       0x2D
#define LCD_SETDDRAMADDR    0x80
#define LCD_DISPLAYCONTROL  0x08
#define LCD_ENTRYMODESET    0x04

// Flags for display on/off control
#define LCD_DISPLAYON       0x04
#define LCD_CURSOROFF       0x00
#define LCD_BLINKOFF        0x00

// Flags for display entry mode
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTDECREMENT 0x00

class DisplayLcd : public Display
{
	public:
		DisplayLcd(TwoWire &wirePort = Wire, uint8_t i2c_addr = LCD_DEFAULT_ADDRESS);
		~DisplayLcd();

		// Display interface implementations
		void clear(void) override;
		void gotoxy(int x, int y) override;
		void print(char c) override;
		void print(const char *str) override;
		void print(const std::string &str) override;
		
		// Event Handling implementations
		bool getEvent(DisplayEvent *event) override;
		void readKeys(void);

	private:
		TwoWire *_i2cPort;
		uint8_t _i2cAddr;
		uint8_t _displayControl;
		uint8_t _displayMode;

		// Input polling states
		DisplayEventQueue<DisplayEvent, 10> eventQueue;
		uint8_t _lastButtonState;

		// Internal I2C utility methods
		void init(void);
		void beginTransmission(void);
		void transmit(uint8_t data);
		void endTransmission(void);
		void command(uint8_t cmd);
		void specialCommand(uint8_t cmd);
};