#include "menu-mgr.h"
#include <algorithm> // Added for std::find_if

void MenuManager::begin()
{
	if (cur)
	{
		cur->onEnter();
	}
}

void MenuManager::process()
{
	if(!cur)
		return;
	MenuEvent ev = cur->update();
	std::shared_ptr<Menu> next = nullptr;

	if(ev == MenuEvent::FORWARD)
	{
		if(!cur->getChildren().empty())
		{
			// Get visible children only
			std::vector<std::shared_ptr<Menu>> visible;
			for(auto &c : cur->getChildren())
				if(c->isVisible())
					visible.push_back(c);

			if(!visible.empty())
			{
				// Safely get index polymorphically.
				// MenuListSelector returns its tracked selection; other menus default to 0.
				int index = cur->getSelectedIndex();

				if(index >= 0 && index < static_cast<int>(visible.size()))
				{
					next = visible[index];
				}
			}
		}
	}
	else if(ev == MenuEvent::BACK && cur->getParent())
	{
		next = std::shared_ptr<Menu>(cur->getParent(), [](Menu *) {});
	}
	// Added: Handle NEXT and PREV sibling navigation
	else if((ev == MenuEvent::NEXT || ev == MenuEvent::PREV) && cur->getParent())
	{
		const auto &siblings = cur->getParent()->getChildren();

		// Find current menu's position among its siblings via raw pointer comparison
		auto it = std::find_if(siblings.begin(), siblings.end(),
				       [&](const std::shared_ptr<Menu> &s) { return s.get() == cur.get(); });

		if(it != siblings.end())
		{
			if(ev == MenuEvent::NEXT)
			{
				// Seek forward to find the next visible sibling
				auto next_it = it + 1;
				while(next_it != siblings.end() && !(*next_it)->isVisible())
				{
					next_it++;
				}
				if(next_it != siblings.end())
				{
					next = *next_it;
				}
			}
			else if(ev == MenuEvent::PREV)
			{
				// Seek backward to find the previous visible sibling
				if(it != siblings.begin())
				{
					auto prev_it = it - 1;
					while(prev_it != siblings.begin() && !(*prev_it)->isVisible())
					{
						prev_it--;
					}
					if((*prev_it)->isVisible())
					{
						next = *prev_it;
					}
				}
			}
		}
	}

	if(next && next != cur)
	{
		cur = next;
		cur->onEnter();
	}
}