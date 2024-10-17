#pragma once
#include "ComponentSet.h"
#include "ResourceData.h"
#include "gui/interface/Window.h"

class Credits : public ui::Window
{
	enum CreditSize
	{
		Small,
		Large,
		Half,
	};

	static ui::ComponentSet AddCredit(ResourceData avatar, const String &message1, const String &message2,
		CreditSize size, const ByteString &uri);
	static ByteString GetProfileUri(const ByteString &username);
	static ByteString GetGithubCommitsUri(const ByteString &username);
	static ByteString GetTptLabelText(const ByteString &tpt, const ByteString &github);
public:
	Credits();

	void OnTryExit(ExitMethod method) override;
};
