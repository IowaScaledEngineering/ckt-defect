#pragma once
#include "display.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

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

	public:
		Menu(const std::string &name) : menuName(name) {}
		virtual ~Menu() = default;
		std::string getName() const { return menuName; }
		static void setDisplay(Display *d) { disp = d; }
		virtual MenuEvent update() = 0;

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
		uint8_t iDigits, fDigits, state = 0, curDigit = 0;
		std::string modStr;
		bool suppressLeadingZeros;

	public:
		MenuDigitThumbwheel(const std::string &name, uint32_t *p, uint32_t i, uint32_t f, bool suppress)
		    : Menu(name), valPtr(p), iDigits(i), fDigits(f), suppressLeadingZeros(suppress)
		{
		}
		MenuEvent update() override;
};

class MenuNumberDial : public Menu
{
		uint32_t *valPtr;
		uint32_t currentVal;
		uint32_t minVal;
		uint32_t maxVal;
		uint8_t state = 0;
		int fieldWidth = 0;
		int maxDigits = 0;
		std::string units = "";

	public:
		MenuNumberDial(const std::string &name, uint32_t *p, uint32_t min, uint32_t max, std::string units)
		    : Menu(name), valPtr(p), minVal(min), maxVal(max), units(units)
		{
			maxDigits = (int)std::to_string(maxVal).length();
		}

		MenuEvent update() override;
};

class MenuBoolSelector : public Menu
{
		bool *valPtr;
		bool currentVal;
		std::string opt1Name;
		std::string opt2Name;
		std::string btn1Name;
		std::string btn2Name;
		uint8_t state = 0;

	public:
		MenuBoolSelector(const std::string &name, bool *p, const std::string &opt1, const std::string &btn1,
				 const std::string &opt2, const std::string &btn2)
		    : Menu(name), valPtr(p), currentVal(*p), opt1Name(opt1), opt2Name(opt2), btn1Name(btn1),
		      btn2Name(btn2)
		{
		}

		MenuEvent update() override;
};

class MenuOptionSelector : public Menu
{
		uint32_t *valPtr;
		uint32_t currentVal;
		std::vector<std::string> options;
		uint32_t topIndex = 0;
		uint8_t state = 0;

	public:
		MenuOptionSelector(const std::string &name, uint32_t *p, const std::vector<std::string> &opts)
		    : Menu(name), valPtr(p), currentVal(*p), options(opts)
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

	public:
		// Constructor for direct pointers (remains unchanged)
		MenuPercentageBar(const std::string &name, uint32_t *p, uint32_t max, uint32_t pcntStep)
		    : Menu(name), valPtr(p), getFunc(nullptr), setFunc(nullptr), realTime(false),
		      maxVal(max == 0 ? 100 : max), stepVal(pcntStep) {}

		// Modernized constructor using std::function
		MenuPercentageBar(const std::string &name, 
		                  std::function<uint32_t()> getter, 
		                  std::function<void(uint32_t)> setter, 
		                  uint32_t max, 
		                  uint32_t pcntStep, 
		                  bool realTimeUpdate)
		    : Menu(name), valPtr(nullptr), getFunc(std::move(getter)), setFunc(std::move(setter)), 
		      realTime(realTimeUpdate), maxVal(max == 0 ? 100 : max), stepVal(pcntStep) {}

		MenuEvent update() override;
};
