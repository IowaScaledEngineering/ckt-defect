#pragma once
#include "menu.h"
class MenuManager
{
		std::shared_ptr<Menu> cur;

	public:
		MenuManager(std::shared_ptr<Menu> r) : cur(r) { }
		void begin();
		void process();
};
