
#pragma once
#include "display.h"
#include <termios.h>

class DisplayTerm : public Display
{
	public:
		DisplayTerm(void);
		~DisplayTerm();
		void clear(void);
		void gotoxy(int x, int y);
		void print(char c);
		void print(const char *str);
		void print(const std::string &str);
		bool getEvent(DisplayEvent *event);

		void readKeys(void);

	private:
		struct termios original_terminal;
		DisplayEventQueue<DisplayEvent, 10> eventQueue;
};
