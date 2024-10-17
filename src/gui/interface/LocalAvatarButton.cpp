#include "LocalAvatarButton.h"
#include "Button.h"
#include "Format.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "ContextMenu.h"
#include "Config.h"
#include <iostream>
#include <SDL.h>

namespace ui
{

LocalAvatarButton::LocalAvatarButton(const Point position, const Point size, const unsigned char data[], const unsigned int dataSize):
	Component(position, size)
{
	avatar = VideoBuffer::FromPNG(std::vector<char>(&data[0], &data[0] + dataSize));
	if (avatar)
		avatar->Resize(size);
}

void LocalAvatarButton::Draw(const Point& screenPos)
{
	Graphics * g = GetGraphics();

	if (avatar)
	{
		auto *tex = avatar.get();
		g->BlendImage(tex->Data(), 255, RectSized(screenPos, tex->Size()));
	}
}

void LocalAvatarButton::OnMouseClick(int x, int y, unsigned int button)
{
	if (button != 1)
	{
		return;
	}

	if (isButtonDown)
	{
		isButtonDown = false;
		DoAction();
	}
}

void LocalAvatarButton::OnMouseDown(int x, int y, unsigned int button)
{
	if (MouseDownInside)
	{
		if (button == SDL_BUTTON_RIGHT)
		{
			if (menu)
				menu->Show(GetContainerPos() + ui::Point(x, y));
		}
		else
		{
			isButtonDown = true;
		}
	}
}

void LocalAvatarButton::DoAction() const
{
	if (actionCallback.action)
		actionCallback.action();
}

}
