#pragma once
#include "display.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class MenuEvent
{
	NOOP,
	BACK,
	FORWARD,
	NEXT,
	PREV
};

class Menu
{
	protected:
		static inline Display *disp = nullptr; // static global access to the display
		std::string menuName;
		Menu *parent = nullptr, *next = nullptr, *prev = nullptr;
		std::vector<std::shared_ptr<Menu>> children;
		bool hidden = false;

		// Global holding/repeating timers and button state properties
		static inline std::function<uint32_t()> getMillis = nullptr;
		static inline uint32_t initialHoldDelayMs = 1000; // Initial delay before auto-repeat starts
		static inline uint32_t holdDelayMs = 500; // Default programmable time (ms) to repeat
		static inline int lastButtonNum = 0;
		static inline uint32_t lastButtonPressTime = 0;
		static inline bool isHolding = false; // Tracks if initial timeout has expired

		// Shared helper to retrieve and process display/hold events
		bool getMenuInputEvent(DisplayEvent *ev);
	public:
		Menu(const std::string &name) : menuName(name) {}
		virtual ~Menu() = default;
		std::string getName() const { return menuName; }
		static void setDisplay(Display *d) { disp = d; }
		virtual MenuEvent update() = 0;

		// Setter interface for global button repeating assets
		static void setTimingCallback(std::function<uint32_t()> callback) { getMillis = callback; }
		static void setHoldDelay(uint32_t delayMs) { holdDelayMs = delayMs; }
		static void setInitialHoldDelay(uint32_t delayMs) { initialHoldDelayMs = delayMs; }

		virtual int getSelectedIndex() const { return 0; }

		void hide() { hidden = true; }
		void unhide() { hidden = false; }
		bool isVisible() const { return !hidden; }

		void addChild(std::shared_ptr<Menu> child)
		{
			if(!children.empty())
			{
				children.back()->next = child.get();
				child->prev = children.back().get();
			}
			child->parent = this;
			children.push_back(std::move(child));
		}
		Menu *getParent() { return parent; }
		const std::vector<std::shared_ptr<Menu>> &getChildren() { return children; }
};

class MenuListSelector : public Menu
{
		uint32_t selector = 0; // Index relative to visible items

	public:
		using Menu::Menu;
		MenuEvent update() override;
		int getSelectedIndex() const { return selector; }
};

class MenuDigitThumbwheel : public Menu
{
		uint32_t *valPtr;
		std::function<uint32_t()> getFunc;
		std::function<void(uint32_t)> setFunc;
		bool realTime;
		uint32_t originalVal;
		uint8_t iDigits, fDigits, state = 0, curDigit = 0;
		std::string modStr;
		bool suppressLeadingZeros;
		std::function<void()> saveCallback;

	public:
		// Constructor for direct pointers
		MenuDigitThumbwheel(const std::string &name, uint32_t *p, bool realTimeUpdate, uint32_t i, uint32_t f, bool suppressLeadingZeros, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(realTimeUpdate), iDigits(i),
		      fDigits(f), suppressLeadingZeros(suppressLeadingZeros), saveCallback(std::move(onSave))
		{
		}

		// Constructor using std::function callbacks
		MenuDigitThumbwheel(const std::string &name, std::function<uint32_t()> getter,
				    std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t i, uint32_t f,
				    bool suppress, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)),
		      realTime(realTimeUpdate), iDigits(i), fDigits(f), suppressLeadingZeros(suppress), saveCallback(std::move(onSave))
		{
		}

		MenuEvent update() override;
};

class MenuNumberDial : public Menu
{
		uint32_t *valPtr;
		std::function<uint32_t()> getFunc;
		std::function<void(uint32_t)> setFunc;
		bool realTime;
		uint32_t originalVal;
		uint32_t currentVal;
		uint32_t minVal;
		uint32_t maxVal;
		uint8_t state = 0;
		int fieldWidth = 0;
		int maxDigits = 0;
		std::string units = "";
		std::function<void()> saveCallback;

	public:
		// Constructor for direct pointers
		MenuNumberDial(const std::string &name, uint32_t *p, bool realTimeUpdate, uint32_t min, uint32_t max, std::string units, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(realTimeUpdate), minVal(min),
		      maxVal(max), units(units), saveCallback(std::move(onSave))
		{
			maxDigits = (int)std::to_string(maxVal).length();
		}

		// Constructor using std::function callbacks
		MenuNumberDial(const std::string &name, std::function<uint32_t()> getter,
			       std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t min, uint32_t max, std::string units, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)),
		      realTime(realTimeUpdate), minVal(min), maxVal(max), units(units), saveCallback(std::move(onSave))
		{
			maxDigits = (int)std::to_string(maxVal).length();
		}

		MenuEvent update() override;
};

class MenuBoolSelector : public Menu
{
		bool *valPtr;
		std::function<bool()> getFunc;
		std::function<void(bool)> setFunc;
		bool realTime;
		bool originalVal;
		bool currentVal;
		std::string opt1Name;
		std::string btn1Name;
		std::string opt2Name;
		std::string btn2Name;
		uint8_t state = 0;
		std::function<void()> saveCallback;

	public:
		// Constructor for direct pointers
		MenuBoolSelector(const std::string &name, bool *p, bool realTimeUpdate, const std::string &opt1, const std::string &btn1, const std::string &opt2, const std::string &btn2, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(realTimeUpdate),
		      opt1Name(opt1), btn1Name(btn1), opt2Name(opt2), btn2Name(btn2), saveCallback(std::move(onSave))
		{
		}

		// Constructor using std::function callbacks
		MenuBoolSelector(const std::string &name, std::function<bool()> getter, std::function<void(bool)> setter, bool realTimeUpdate, const std::string &opt1, const std::string &btn1, const std::string &opt2, const std::string &btn2, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)), realTime(realTimeUpdate),
		      opt1Name(opt1), btn1Name(btn1), opt2Name(opt2), btn2Name(btn2), saveCallback(std::move(onSave))
		{
		}

		MenuEvent update() override;
};

class MenuOptionSelector : public Menu
{
	private:
		uint32_t *valPtr;
		std::function<uint32_t()> getFunc;
		std::function<void(uint32_t)> setFunc;
		bool realTime;

		uint32_t originalVal;
		uint32_t currentVal;
		std::vector<std::string> options;
		uint32_t topIndex = 0;
		uint8_t state = 0;
		std::function<void()> saveCallback;

	public:
		// Constructor for direct pointers
		MenuOptionSelector(const std::string &name, uint32_t *p, bool realTimeUpdate, const std::vector<std::string> &opts, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(realTimeUpdate),
		      currentVal(0), options(opts), saveCallback(std::move(onSave))
		{
		}

		MenuOptionSelector(const std::string &name, std::function<uint32_t()> getter,
				   std::function<void(uint32_t)> setter, bool realTimeUpdate,
				   const std::vector<std::string> &opts, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)),
		      realTime(realTimeUpdate), currentVal(0), options(opts), saveCallback(std::move(onSave))
		{
		}

		MenuEvent update() override;
};

class MenuPercentageBar : public Menu
{
	private:
		uint32_t *valPtr;
		// Accept any callable object (lambdas, member functions, free functions)
		std::function<uint32_t()> getFunc;
		std::function<void(uint32_t)> setFunc;
		bool realTime;
		uint32_t maxVal;
		uint32_t stepVal;

		uint32_t originalVal;
		int32_t currentVal;
		uint8_t state = 0;
		std::function<void()> saveCallback;

	public:
		// Constructor for direct pointers
		MenuPercentageBar(const std::string &name, uint32_t *p, bool realTimeUpdate, uint32_t max, uint32_t pcntStep, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(realTimeUpdate),
		      maxVal(max == 0 ? 100 : max), stepVal(pcntStep), saveCallback(std::move(onSave))
		{
		}

		// Constructor using std::function callbacks
		MenuPercentageBar(const std::string &name, std::function<uint32_t()> getter,
				  std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t max, uint32_t pcntStep, std::function<void()> onSave = nullptr)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)),
		      realTime(realTimeUpdate), maxVal(max == 0 ? 100 : max), stepVal(pcntStep), saveCallback(std::move(onSave))
		{
		}

		MenuEvent update() override;
};
