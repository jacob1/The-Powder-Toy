#pragma once

#include "Component.h"
#include "graphics/VideoBuffer.h"

#include <memory>
#include <functional>

namespace ui
{
class LocalAvatarButton : public Component
{
	std::unique_ptr<VideoBuffer> avatar;

	struct AvatarButtonAction
	{
		std::function<void ()> action;
	};
	AvatarButtonAction actionCallback;

public:
	LocalAvatarButton(Point position, Point size, const unsigned char data[], unsigned int dataSize);

	void OnMouseClick(int x, int y, unsigned int button) override;
	void OnMouseDown(int x, int y, unsigned int button) override;

	void Draw(const Point& screenPos) override;

	void DoAction() const;

	void SetActionCallback(AvatarButtonAction const &action) { actionCallback = action; };
protected:
	bool isMouseInside = false, isButtonDown = false;
};

}
