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
		static inline uint32_t longHoldDelayMs = 3000; // Delay before going to faster auto-increment
		static inline uint32_t holdDelayMs = 500; // Default programmable time (ms) to repeat
		static inline uint32_t fastDelayMs = 100; // Default programmable time (ms) to fast repeat
		static inline int lastButtonNum = 0;
		static inline uint32_t lastButtonPressTime = 0;
		static inline bool isHolding = false; // Tracks if initial timeout has expired

		// Centralized value binding fields for underlying model modifications
		bool *valPtrBool = nullptr;
		std::function<uint32_t()> getFunc32 = nullptr;
		std::function<bool()> getFuncBool = nullptr;
		std::function<void(uint32_t)> setFunc32 = nullptr;
		std::function<void(bool)> setFuncBool = nullptr;

		bool realTime = false;
		uint32_t originalVal = 0;
		std::function<void()> saveCallback = nullptr;

		// Shared state extraction and evaluation helpers
		bool getMenuInputEvent(DisplayEvent *ev);
		void handleButtonPress(int keyNum);
		void handleButtonRelease(int keyNum);
		uint32_t getValue();
		void setValue(uint32_t value);
		void applyChange(uint32_t value);
		void cancel();

	public:
		Menu(const std::string &name) : menuName(name) {}
		virtual ~Menu() = default;
		std::string getName() const { return menuName; }
		static void setDisplay(Display *d) { disp = d; }
		void setSaveCallback(std::function<void()> onSave);
		virtual void onEnter()
		{
			if(disp) disp->clear();
			lastButtonNum = 0;
			isHolding = false;
		}
		virtual MenuEvent update() = 0;

		// Setter interface for global button repeating assets
		static void setTimingCallback(std::function<uint32_t()> callback) { getMillis = callback; }
		static void setHoldDelay(uint32_t delayMs) { holdDelayMs = delayMs; }
		static void setFastDelay(uint32_t delayMs) { fastDelayMs = delayMs; }
		static void setInitialHoldDelay(uint32_t delayMs) { initialHoldDelayMs = delayMs; }
		static void setLongHoldDelay(uint32_t delayMs) { longHoldDelayMs = delayMs; }

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
		uint8_t iDigits, fDigits, curDigit = 0;
		std::string modStr;
		bool suppressLeadingZeros;

	public:
		// Constructor for direct pointers
		template <typename T>
		MenuDigitThumbwheel(const std::string &name, T *p, bool realTimeUpdate, uint32_t i, uint32_t f, bool suppressLeadingZeros, std::function<void()> onSave = nullptr)
		    : Menu(name), iDigits(i), fDigits(f), suppressLeadingZeros(suppressLeadingZeros)
		{
			static_assert(std::is_integral<T>::value, "MenuOptionSelector requires an integer type pointer.");
			getFunc32 = [p]() -> uint32_t { return static_cast<uint32_t>(*p); };
			setFunc32 = [p](uint32_t val) { *p = static_cast<T>(val); };
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		// Constructor using std::function callbacks
		MenuDigitThumbwheel(const std::string &name, std::function<uint32_t()> getter,
				    std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t i, uint32_t f,
				    bool suppress, std::function<void()> onSave = nullptr)
		    : Menu(name), iDigits(i), fDigits(f), suppressLeadingZeros(suppress)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		void onEnter() override;
		MenuEvent update() override;
};

class MenuNumberDial : public Menu
{
		uint32_t currentVal;
		uint32_t minVal;
		uint32_t maxVal;
		int fieldWidth = 0;
		int maxDigits = 0;
		std::string units = "";

	public:
		// Constructor for direct pointers
		template <typename T>
		MenuNumberDial(const std::string &name, T *p, bool realTimeUpdate, uint32_t min, uint32_t max, std::string units, std::function<void()> onSave = nullptr)
		    : Menu(name), minVal(min), maxVal(max), units(units)
		{
			static_assert(std::is_integral<T>::value, "MenuOptionSelector requires an integer type pointer.");
			getFunc32 = [p]() -> uint32_t { return static_cast<uint32_t>(*p); };
			setFunc32 = [p](uint32_t val) { *p = static_cast<T>(val); };
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
			maxDigits = (int)std::to_string(maxVal).length();
		}

		// Constructor using std::function callbacks
		MenuNumberDial(const std::string &name, std::function<uint32_t()> getter,
			       std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t min, uint32_t max, std::string units, std::function<void()> onSave = nullptr)
		    : Menu(name), minVal(min), maxVal(max), units(units)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
			maxDigits = (int)std::to_string(maxVal).length();
		}

		void onEnter() override;
		MenuEvent update() override;
};

class MenuBoolSelector : public Menu
{
		bool currentVal;
		std::string optTrueName;
		std::string btnTrueName;
		std::string optFalseName;
		std::string btnFalseName;

	public:
		// Constructor for direct pointers
		MenuBoolSelector(const std::string &name, bool *p, bool realTimeUpdate, const std::string &optTrue, const std::string &btnTrue, const std::string &optFalse, const std::string &btnFalse, std::function<void()> onSave = nullptr)
		    : Menu(name), optTrueName(optTrue), btnTrueName(btnTrue), optFalseName(optFalse), btnFalseName(btnFalse)
		{
			valPtrBool = p;
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		// Constructor using std::function callbacks
		MenuBoolSelector(const std::string &name, std::function<bool()> getter, std::function<void(bool)> setter, bool realTimeUpdate, const std::string &optTrue, const std::string &btnTrue, const std::string &optFalse, const std::string &btnFalse, std::function<void()> onSave = nullptr)
		    : Menu(name), optTrueName(optTrue), btnTrueName(btnTrue), optFalseName(optFalse), btnFalseName(btnFalse)
		{
			getFuncBool = std::move(getter);
			setFuncBool = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		void onEnter() override;
		MenuEvent update() override;
};

class MenuOptionSelector : public Menu
{
	private:
		uint32_t currentVal;
		std::vector<std::string> options;
		uint32_t topIndex = 0;

	public:
		// Constructor for direct pointers
		template <typename T>
		MenuOptionSelector(const std::string &name, T *p, bool realTimeUpdate, const std::vector<std::string> &opts, std::function<void()> onSave = nullptr)
		    : Menu(name), currentVal(0), options(opts)
		{
			static_assert(std::is_integral<T>::value, "MenuOptionSelector requires an integer type pointer.");
			getFunc32 = [p]() -> uint32_t { return static_cast<uint32_t>(*p); };
			setFunc32 = [p](uint32_t val) { *p = static_cast<T>(val); };
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		// Constructor using std::function callbacks
		MenuOptionSelector(const std::string &name, std::function<uint32_t()> getter,
				   std::function<void(uint32_t)> setter, bool realTimeUpdate,
				   const std::vector<std::string> &opts, std::function<void()> onSave = nullptr)
		    : Menu(name), currentVal(0), options(opts)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		void onEnter() override;
		MenuEvent update() override;
};

class MenuPercentageBar : public Menu
{
	private:
		uint32_t maxVal;
		uint32_t stepVal;
		int32_t currentVal;

	public:
		// Constructor for direct pointers
		template <typename T>
		MenuPercentageBar(const std::string &name, T *p, bool realTimeUpdate, uint32_t max, uint32_t pcntStep, std::function<void()> onSave = nullptr)
		    : Menu(name), maxVal(max == 0 ? 100 : max), stepVal(pcntStep)
		{
			static_assert(std::is_integral<T>::value, "MenuOptionSelector requires an integer type pointer.");
			getFunc32 = [p]() -> uint32_t { return static_cast<uint32_t>(*p); };
			setFunc32 = [p](uint32_t val) { *p = static_cast<T>(val); };
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		// Constructor using std::function callbacks
		MenuPercentageBar(const std::string &name, std::function<uint32_t()> getter,
				  std::function<void(uint32_t)> setter, bool realTimeUpdate, uint32_t max, uint32_t pcntStep, std::function<void()> onSave = nullptr)
		    : Menu(name), maxVal(max == 0 ? 100 : max), stepVal(pcntStep)
		{
			getFunc32 = std::move(getter);
			setFunc32 = std::move(setter);
			realTime = realTimeUpdate;
			saveCallback = std::move(onSave);
		}

		void onEnter() override;
		MenuEvent update() override;
};
