#pragma once

#include <vector>
#include "gui/interface/Point.h"

namespace ui
{

class Component;
class ScrollPanel;

class ComponentSet
{
	std::vector<ui::Component*> components;

public:
	explicit ComponentSet(const std::vector<ui::Component*> &components);

	void AddOffset(ui::Point offset);
	void AddToPanel(ui::ScrollPanel *panel);
	ui::Point Size() const;
};

}
