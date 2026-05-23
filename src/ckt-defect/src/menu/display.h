#pragma once
#include <memory>
#include <optional>
#include <stdint.h>
#include <string>
#include <vector>

enum class DisplayEventType
{
	KEY_DOWN,
	KEY_UP
};
struct DisplayEvent
{
		DisplayEventType type;
		int keyNum;
};

class Display
{
	public:
		virtual ~Display() {}
		virtual void clear(void) = 0;
		virtual void gotoxy(int x, int y) = 0;
		virtual void print(char c) = 0;
		virtual void print(const char *str) = 0;
		virtual void print(const std::string &str) = 0;
		virtual bool getEvent(DisplayEvent *event) = 0;

		// New Backlight Interface
		virtual void backlightOn(void) = 0;
		virtual void backlightOff(void) = 0;
		virtual bool getBacklight(void) const = 0;
};

template <typename T, size_t Capacity> class DisplayEventQueue
{
		T items[Capacity];
		int head = 0, tail = 0, count = 0;

	public:
		bool push(const T &event)
		{
			if(count == Capacity)
				return false;
			items[tail] = event;
			tail = (tail + 1) % Capacity;
			count++;
			return true;
		}
		std::optional<T> pop()
		{
			if(count == 0)
				return std::nullopt;
			T event = items[head];
			head = (head + 1) % Capacity;
			count--;
			return event;
		}
		bool isEmpty() const { return count == 0; }
};