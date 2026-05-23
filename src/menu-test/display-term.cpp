#include "display-term.h"
#include "display.h"
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

DisplayTerm::DisplayTerm(void) : _backlight(true) // Default to on
{
	// Save original settings
	tcgetattr(STDIN_FILENO, &original_terminal);

	// Configure raw mode
	struct termios raw = original_terminal;
	// ICANON: Disable line buffering (read character by character)
	// ECHO:   Disable printing the character back to the screen
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);

	// Set STDIN to non-blocking mode
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	// Hide the cursor
	std::cout << "\033[?25l" << std::flush;
}

DisplayTerm::~DisplayTerm()
{
	// Restore cursor
	std::cout << "\033[?25h" << std::flush;
	// Restore original terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
}

void DisplayTerm::clear(void)
{
	// Clear screen
	std::cout << "\033[2J";

	// Draw border
	const int width = 22;
	const int height = 6;

	// 1. Enable Inverse Video mode
	std::cout << "\033[7m";

	// 2. Draw Top and Bottom Horizontal lines
	for(int x = 1; x <= width; ++x)
	{
		std::cout << "\033[1;" << x << "H" << (x == 1 ? "┌" : (x == width ? "┐" : "─"));
		std::cout << "\033[" << height << ";" << x << "H" << (x == 1 ? "└" : (x == width ? "┘" : "─"));
	}

	// 3. Draw Left and Right Vertical lines
	for(int y = 2; y < height; ++y)
	{
		std::cout << "\033[" << y << ";1H│";
		std::cout << "\033[" << y << ";" << width << "H│";
	}

	// 4. Disable Inverse Video and flush
	std::cout << "\033[0m" << std::flush;

	// Draw the line 8 status update
	updateBacklightStatusDisplay();

	// Home cursor
	gotoxy(0, 0);
}

void DisplayTerm::gotoxy(int x, int y)
{
	// Add two since ANSI locations are 1-based, and to include the border
	std::cout << "\033[" << static_cast<int>(y + 2) << ";" << static_cast<int>(x + 2) << "H";
}

void DisplayTerm::print(char c) { std::cout << c << std::flush; }

void DisplayTerm::print(const char *str) { std::cout << str << std::flush; }

void DisplayTerm::print(const std::string &str) { std::cout << str << std::flush; }


void DisplayTerm::createCustomChar(uint8_t location, const uint8_t* charmap)
{
	// Do nothing
}

bool DisplayTerm::getEvent(DisplayEvent *event)
{
	if(eventQueue.isEmpty())
		return false;
	auto e = eventQueue.pop();
	if(!e.has_value())
		return false;
	*event = *e;
	return true;
}

void DisplayTerm::backlightOn(void)
{
	if (!_backlight) {
		_backlight = true;
		updateBacklightStatusDisplay();
	}
}

void DisplayTerm::backlightOff(void)
{
	if (_backlight) {
		_backlight = false;
		updateBacklightStatusDisplay();
	}
}

bool DisplayTerm::getBacklight(void) const
{
	return _backlight;
}

void DisplayTerm::updateBacklightStatusDisplay(void)
{
	// Save existing cursor position visually by using ANSI escape storage sequences
	std::cout << "\033[s"; 
	
	// Jump directly to Line 8, Column 1
	std::cout << "\033[8;1H";
	
	// Print status with extra spaces to cleanly overwrite old strings
	if (_backlight) {
		std::cout << "Backlight  On";
	} else {
		std::cout << "Backlight Off";
	}
	
	// Restore cursor position and flush
	std::cout << "\033[u" << std::flush;
}

void DisplayTerm::readKeys(void)
{
	char ch;
	int eventNum = 0;
	DisplayEvent newEvent;
	// read() returns the number of bytes read
	if(read(STDIN_FILENO, &ch, 1) > 0)
	{
		switch(ch)
		{
			case 'q':
				eventNum = 100;
				break;
			case 'a':
				eventNum = 1;
				break;
			case 's':
				eventNum = 2;
				break;
			case 'd':
				eventNum = 3;
				break;
			case 'f':
				eventNum = 4;
				break;
		}
		if(eventNum)
		{
			newEvent = {DisplayEventType::KEY_DOWN, eventNum};
			eventQueue.push(newEvent); // Blindly fails if queue full
			newEvent = {DisplayEventType::KEY_UP, eventNum};
			eventQueue.push(newEvent); // Blindly fails if queue full
		}
	}
}
