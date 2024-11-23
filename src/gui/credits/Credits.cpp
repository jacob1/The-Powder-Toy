#include "Credits.h"

#include <common/platform/Platform.h>
#include <gui/interface/LocalAvatarButton.h>
#include <json/json.h>

#include "credits.json.h"
#include "gh_avatars.png.h"
#include "tpt_avatars.png.h"
#include "ResourceData.h"
#include "gui/Style.h"
#include "gui/interface/Button.h"
#include "gui/interface/Engine.h"
#include "gui/interface/Label.h"
#include "gui/interface/ScrollPanel.h"
#include "gui/interface/Separator.h"

Credits::Credits():
	ui::Window(ui::Point(-1, -1), ui::Point(WINDOWW, WINDOWH))
{
	Json::Value root;
	Json::Reader reader;
	if (bool parsed = reader.parse((const char*)credits_json, (const char*)credits_json + credits_json_size, root, false); !parsed) {
		// Failure. Shouldn't ever happen.
		return;
	}

	auto *scrollPanel = new ui::ScrollPanel(ui::Point(0, 0), ui::Point(Size.X, Size.Y - 12));
	AddComponent(scrollPanel);

	int xPos = 0, yPos = 0, row = 0;
	int nextY = 0;

	// Organize blocks of components of equal width into rows
	auto organizeComponents = [&xPos, &yPos, &nextY, &row](ui::ComponentSet components, const int panelWidth) {
		auto blockSize = components.Size();

		// New row, offset x position to ensure entire row is centered
		if (xPos == 0)
			xPos = (panelWidth % blockSize.X) / 2;
		components.AddOffset({ xPos, yPos });

		xPos += blockSize.X;
		nextY =  std::max(nextY, yPos + blockSize.Y);
		if (xPos + blockSize.X > panelWidth)
		{
			xPos = 0;
			yPos = nextY + 8;
			row++;
		}
	};

	auto addHeader = [&xPos, &yPos, &nextY, &row, &scrollPanel](const String &text, const bool addSeparator = true) {
		xPos = 0;
		yPos = nextY + 9;
		row = 0;

		if (addSeparator)
		{
			auto *separator = new ui::Separator(ui::Point(0, yPos), ui::Point(scrollPanel->Size.X, 1));
			scrollPanel->AddChild(separator);
			yPos += 6;
		}

		auto *label = new ui::Label(ui::Point(4, yPos), ui::Point(scrollPanel->Size.X, 24), text);
		label->SetTextColour(style::Colour::InformationTitle);
		label->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		scrollPanel->AddChild(label);
		yPos += label->Size.Y + 8;
	};


	addHeader("The following users have been credited in the intro text from the start.\n"
			"Their contributions to the early beginnings of TPT were invaluable in shaping TPT into what it is today.",
			false);

	auto OrigCredits = root["OrigCredits"];
	for (auto &item : OrigCredits)
	{
		ByteString username = item["username"].asString();
		ByteString realname = item["realname"].asString();
		ByteString message = item["message"].asString();

		unsigned int pos = item["pos"].asUInt();
		unsigned int size = item["size"].asUInt();
		ResourceData avatar = { &tpt_avatars[pos], size };

		auto components = AddCredit(avatar, realname.FromUtf8(), message.FromUtf8(), row == 0 ? Half : Large, GetProfileUri(username));
		organizeComponents(components, scrollPanel->Size.X);
		components.AddToPanel(scrollPanel);
	}


	addHeader("TPT is an open source project, developed by members of the community.\n"
			"We'd like to thank everyone who contributed to our GitHub repo:");

	auto GitHub = root["GitHub"];
	for (auto &item : GitHub)
	{
		ByteString gh = item["gh"].asString();
		ByteString tpt = item["tpt"].isNull() ? "" : item["tpt"].asString();
		ByteString tptLabelText = GetTptLabelText(tpt, gh);

		unsigned int pos = item["pos"].asUInt();
		unsigned int size = item["size"].asUInt();
		ResourceData avatar = { &gh_avatars[pos], size };

		auto components = AddCredit(avatar, gh.FromUtf8(), tptLabelText.FromUtf8(), row <= 2 ? Large : Small, GetGithubCommitsUri(gh));
		//auto components = AddCredit({}, gh.FromUtf8(), "", Small, "");
		organizeComponents(components, scrollPanel->Size.X);
		components.AddToPanel(scrollPanel);
	}


	addHeader("Staff");

	auto Moderators = root["Moderators"];
	for (auto &item : Moderators)
	{
		ByteString username = item["username"].asString();
		ByteString role = item["role"].asString();

		unsigned int pos = item["pos"].asUInt();
		unsigned int size = item["size"].asUInt();
		ResourceData avatar = { &tpt_avatars[pos], size };

		if (role == "Moderator" || role == "HalfMod")
		{
			auto components = AddCredit(avatar, username.FromUtf8(), "", Large, GetProfileUri(username));
			organizeComponents(components, scrollPanel->Size.X);
			components.AddToPanel(scrollPanel);
		}
	}


	addHeader("Former Staff", false);

	for (auto &item : Moderators)
	{
		ByteString username = item["username"].asString();
		ByteString role = item["role"].asString();

		unsigned int pos = item["pos"].asUInt();
		unsigned int size = item["size"].asUInt();
		ResourceData avatar = { &tpt_avatars[pos], size };

		if (role == "Former Staff")
		{
			auto components = AddCredit(avatar, username.FromUtf8(), "", Small, GetProfileUri(username));
			organizeComponents(components, scrollPanel->Size.X);
			components.AddToPanel(scrollPanel);
		}
	}


	scrollPanel->InnerSize = ui::Point(scrollPanel->Size.X, nextY);

	auto *closeButton = new ui::Button({ 0, Size.Y - 12 }, { Size.X, 12 }, "Close");
	closeButton->SetActionCallback({
	[this] {
		CloseActiveWindow();
	}  });
	AddComponent(closeButton);
}

ui::ComponentSet Credits::AddCredit(const ResourceData avatar, const String &message1, const String &message2,
	const CreditSize size, const ByteString &uri)
{
	std::vector<ui::Component *> components;
	int avatarWidth = size == Small ? 40 : 64;
	int fullWidth = size == Small ? 100 : (size == Large ? 155 : 310);
	int y = 0;

	if (avatar.size)
	{
		auto *avatarButton = new ui::LocalAvatarButton(ui::Point((fullWidth - avatarWidth) / 2, 0), ui::Point(avatarWidth, avatarWidth), avatar.data, avatar.size);
		if (!uri.empty())
		{
			avatarButton->SetActionCallback({[uri] {
				Platform::OpenURI(uri);
			} });
		}
		components.push_back(avatarButton);

		y += avatarButton->Size.Y + 2;
	}

	if (!message1.empty())
	{
		auto *message1Label = new ui::Label(ui::Point(0, y), ui::Point(fullWidth, 14), message1);
		message1Label->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		message1Label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		components.push_back(message1Label);

		y += message1Label->Size.Y;
	}

	if (!message2.empty())
	{
		auto *message2Label = new ui::Label(ui::Point(0, y), ui::Point(fullWidth, 14), message2);
		message2Label->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
		message2Label->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
		message2Label->SetTextColour(ui::Colour(170, 170, 170));
		components.push_back(message2Label);
	}

	return ui::ComponentSet(components);
}

ByteString Credits::GetProfileUri(const ByteString &username)
{
	return "https://powdertoy.co.uk/User.html?Name=" + username;
}

ByteString Credits::GetGithubCommitsUri(const ByteString &username)
{
	return "https://github.com/The-Powder-Toy/The-Powder-Toy/commits?author=" + username;
}

ByteString Credits::GetTptLabelText(const ByteString &tpt, const ByteString &github)
{
	if (tpt.empty() || tpt == github)
		return "";
	if (tpt.length() > 15)
		return tpt;
	return "(tpt: " + tpt + ")";
}

void Credits::OnTryExit(ExitMethod method)
{
	ui::Engine::Ref().CloseWindow();
}
