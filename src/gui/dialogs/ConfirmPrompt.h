#ifndef COMFIRMPROMPT_H
#define COMFIRMPROMPT_H

#include <functional>
#include <string>
#include "interface/Window.h"

class ConfirmPrompt : public ui::Window
{
	std::function<void(bool)> confirmAction;
public:
	ConfirmPrompt(std::function<void(bool)>, std::string title, std::string message, std::string OK = "OK", std::string cancel = "Cancel");
	~ConfirmPrompt();
	bool wasConfirmed;
	void Action(bool isConfirmed);

	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
};

#endif
