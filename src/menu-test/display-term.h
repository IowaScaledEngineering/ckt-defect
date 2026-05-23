#pragma once
#include "display.h"
#include <termios.h>

class DisplayTerm : public Display
{
	public:
		DisplayTerm(void);
		~DisplayTerm();
		void clear(void) override;
		void gotoxy(int x, int y) override;
		void print(char c) override;
		void print(const char *str) override;
		void print(const std::string &str) override;
		bool getEvent(DisplayEvent *event) override;

		void readKeys(void);

		// Backlight Implementations
		void backlightOn(void) override;
		void backlightOff(void) override;
		bool getBacklight(void) const override;

	private:
		struct termios original_terminal;
		DisplayEventQueue<DisplayEvent, 10> eventQueue;
		
		bool _backlight;
		void updateBacklightStatusDisplay(void);
};