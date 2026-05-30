#pragma once
#include "menu.h"
class MenuManager
{
		Display *disp;
		std::shared_ptr<Menu> cur;

	public:
		MenuManager(Display *d, std::shared_ptr<Menu> r) : disp(d), cur(r) { }
		void begin();
		void process();
};
