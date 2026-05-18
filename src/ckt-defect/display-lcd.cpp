#include "display-lcd.h"

DisplayLcd::DisplayLcd(TwoWire &wirePort, uint8_t i2c_addr)
	: _i2cPort(&wirePort), 
	  _i2cAddr(i2c_addr),
	  _displayControl(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF),
	  _displayMode(LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT),
	  _lastButtonState(0), // Initialize all buttons as unpressed
	  _cursorX(0),
	  _cursorY(0)
{
	// Initialize cache with spaces
	for (int r = 0; r < LCD_ROWS; ++r) {
		for (int c = 0; c < LCD_COLS; ++c) {
			_cache[r][c] = ' ';
		}
	}
	init();
}

DisplayLcd::~DisplayLcd()
{
	// No terminal states or settings to restore
}

void DisplayLcd::init(void)
{
	beginTransmission();
	transmit(SPECIAL_COMMAND);                      // Send special command character
	transmit(LCD_DISPLAYCONTROL | _displayControl); // Send the display command
	transmit(SPECIAL_COMMAND);                      // Send special command character
	transmit(LCD_ENTRYMODESET | _displayMode);      // Send the entry mode command
	transmit(SETTING_COMMAND);                      // Put LCD into setting mode
	transmit(CLEAR_COMMAND);                        // Send clear display command
	endTransmission();                              // Stop transmission
	delay(5);                                      // Let things settle
}

void DisplayLcd::beginTransmission(void)
{
	_i2cPort->beginTransmission(_i2cAddr);
}

void DisplayLcd::transmit(uint8_t data)
{
	_i2cPort->write(data);
}

void DisplayLcd::endTransmission(void)
{
	_i2cPort->endTransmission();
}

void DisplayLcd::command(uint8_t cmd)
{
	beginTransmission();
	transmit(SETTING_COMMAND); // Put LCD into setting mode
	transmit(cmd);             // Send the command code
	endTransmission();
}

void DisplayLcd::specialCommand(uint8_t cmd)
{
	beginTransmission();
	transmit(SPECIAL_COMMAND); // Send special command character
	transmit(cmd);             // Send the command code
	endTransmission();
}

void DisplayLcd::clear(void)
{
	// Force immediate physical clear
	command(CLEAR_COMMAND);
	delay(5); // Extra delay after a screen clear

	// Clear local cache and reset position tracking
	for (int r = 0; r < LCD_ROWS; ++r) {
		for (int c = 0; c < LCD_COLS; ++c) {
			_cache[r][c] = ' ';
		}
	}
	_cursorX = 0;
	_cursorY = 0;
}

void DisplayLcd::gotoxy(int x, int y)
{
	// Keep variables within LCD matrix bounds
	if (x < 0) x = 0;
	if (x >= LCD_COLS) x = LCD_COLS - 1;
	if (y < 0) y = 0;
	if (y >= LCD_ROWS) y = LCD_ROWS - 1;

	_cursorX = x;
	_cursorY = y;

	int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	specialCommand(LCD_SETDDRAMADDR | (_cursorX + row_offsets[_cursorY]));
}

void DisplayLcd::advanceCursor(void)
{
	_cursorX++;
	if (_cursorX >= LCD_COLS) {
		_cursorX = 0;
		_cursorY++;
		if (_cursorY >= LCD_ROWS) {
			_cursorY = 0; // Wrap around to the top left
		}
	}
}

void DisplayLcd::print(char c)
{
	// Check if the hardware character matches what we want to print
	if (_cache[_cursorY][_cursorX] != c) {
		_cache[_cursorY][_cursorX] = c;
		
		beginTransmission();
		transmit(static_cast<uint8_t>(c));
		endTransmission();
	}
	
	// Advance position state regardless of whether we physical wrote it
	advanceCursor();
}

void DisplayLcd::print(const char *str)
{
	if (str == nullptr) return;

	bool inTransaction = false;

	while (*str) {
		char c = *str;

		if (_cache[_cursorY][_cursorX] != c) {
			// If cache mismatch occurs and we aren't transmitting, sync hardware cursor and start
			if (!inTransaction) {
				// Relocate hardware to match current internal cache coordinates
				int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
				specialCommand(LCD_SETDDRAMADDR | (_cursorX + row_offsets[_cursorY]));
				
				beginTransmission();
				inTransaction = true;
			}

			_cache[_cursorY][_cursorX] = c;
			transmit(static_cast<uint8_t>(c));
			advanceCursor();
		} 
		else {
			// Cache matches. If we are currently transmitting a mismatch block, close it out.
			if (inTransaction) {
				endTransmission();
				inTransaction = false;
			}
			advanceCursor();
		}
		str++;
	}

	// Close final open transaction if it finished on a mismatch string chunk
	if (inTransaction) {
		endTransmission();
	}
}

void DisplayLcd::print(const std::string &str)
{
	print(str.c_str());
}

bool DisplayLcd::getEvent(DisplayEvent *event)
{
	if (eventQueue.isEmpty())
		return false;
		
	auto e = eventQueue.pop();
	if (!e.has_value())
		return false;
		
	*event = *e;
	return true;
}

void DisplayLcd::readKeys(void)
{
	// Request 1 byte of data containing button states from the LCD module
	if (_i2cPort->requestFrom(_i2cAddr, static_cast<uint8_t>(1)) == 1)
	{
		// Isolate the lowest 4 bits representing Buttons 1 through 4
		uint8_t currentButtonState = _i2cPort->read() & 0x0F;
		
		// XOR reveals which bits transitioned state since our last check
		uint8_t changed = currentButtonState ^ _lastButtonState;

		if (changed)
		{
			for (int i = 0; i < 4; ++i)
			{
				// Check if this specific button's bit changed
				if (changed & (1 << i))
				{
					int eventNum = i + 1; // Map bit index (0-3) to button ID (1-4)
					
					// Determine whether it was a press (bit high) or release (bit low)
					DisplayEventType type = (currentButtonState & (1 << i)) 
						? DisplayEventType::KEY_DOWN 
						: DisplayEventType::KEY_UP;

					DisplayEvent newEvent = {type, eventNum};
					eventQueue.push(newEvent); // Attempts to push event onto the circular buffer
				}
			}
			// Update our saved register for the next cycle
			_lastButtonState = currentButtonState;
		}
	}
}