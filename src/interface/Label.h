#ifndef LABEL_H
#define LABEL_H

#include <string>
#include "common/Point.h"
#include "Component.h"

class VideoBuffer;
class Label : public Component
{
private:
	void FindWordPosition(unsigned int position, unsigned int *cursorStart, unsigned int *cursorEnd, const char* spaces);
	unsigned int UpdateCursor(unsigned int position);
	bool CheckPlaceCursor(bool updateCursor, unsigned int position, int posX, int posY);
	uint32_t currentTick;

protected:
	std::string text;
	int textWidth, textHeight;
	bool multiline;
	unsigned int cursor, cursorStart;
	int cursorX, cursorY; // these two and cursorPosReset are used by Textboxes to make the up / down arrow keys work
	bool cursorPosReset;
	uint32_t lastClick;
	unsigned int numClicks, clickPosition;
	bool autosizeX, autosizeY;

	std::string CleanText(std::string dirty, bool ascii, bool colour, bool newlines);
	void UpdateDisplayText(bool updateCursor = false, bool firstClick = false);
	void MoveCursor(unsigned int *cursor, int amount);
	virtual bool ShowCursor() { return false; }

public:
	Label(Point position, Point size, std::string text, bool multiline = false);

	void SetText(std::string text_);
	std::string GetText();
	bool IsMultiline() { return multiline; }

	virtual void OnMouseDown(int x, int y, unsigned char button);
	virtual void OnMouseUp(int x, int y, unsigned char button);
	virtual void OnDefocus();
	virtual void OnMouseMoved(int x, int y, Point difference);
	virtual void OnKeyPress(int key, unsigned short character, unsigned short modifiers);
	virtual void OnDraw(VideoBuffer* vid);
	virtual void OnTick(uint32_t ticks);

	static const int AUTOSIZE = -1;
};

#endif
