#include "ComponentSet.h"
#include "gui/interface/ScrollPanel.h"

namespace ui
{

ComponentSet::ComponentSet(const std::vector<ui::Component*> &components):
	components(components)
{ }

void ComponentSet::AddOffset(ui::Point offset)
{
	for (const auto &component : components)
		component->Position += offset;
}

void ComponentSet::AddToPanel(ui::ScrollPanel *panel)
{
	for (const auto &component : components)
		panel->AddChild(component);
}

ui::Point ComponentSet::Size() const
{
	ui::Point size = { 0, 0 };
	for (const auto &component : components)
	{
		size.X = std::max(size.X, component->Position.X + component->Size.X);
		size.Y = std::max(size.Y, component->Position.Y + component->Size.Y);
	}
	return size;
}

}