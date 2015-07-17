#include <cstdlib>
#include "VideoBuffer.h"
#define INCLUDE_FONTDATA
#include "font.h"

VideoBuffer::VideoBuffer(int width, int height):
	width(width),
	height(height)
{
	vid = new pixel[width*height];
	Clear();
}

VideoBuffer::~VideoBuffer()
{
	delete[] vid;
}

void VideoBuffer::Clear()
{
	std::fill(vid, vid+(width*height), 0);
}

void VideoBuffer::ClearRect(int x, int y, int w, int h)
{
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (x+w >= width)
		w = width-x;
	if (y+h >= height)
		h = height-y;
	if (w<0 || h<0)
		return;

	for (int i = 0; i < h; i++)
		std::fill(&vid[x+width*(y+i)], &vid[x+w+width*(y+i)], 0);
}

void VideoBuffer::CopyVideoBuffer(pixel* vidPaste, int vidWidth, int x, int y)
{
	for (int i = 0; i < height; i++)
	{
		std::copy(&vid[width*i], &vid[width*(i+1)], &vidPaste[x+((y+i)*vidWidth)]);
	}
}

//This function does NO bounds checking
void VideoBuffer::DrawPixel(int x, int y, int r, int g, int b, int a)
{
	//if (x < 0 || y < 0 || x >= width || y >= height || a == 0)
	//	return;
	if (a == 0)
		return;
	if (a != 255)
	{
		pixel t = vid[y*width+x];
		r = (a*r + (255-a)*PIXR(t)) >> 8;
		g = (a*g + (255-a)*PIXG(t)) >> 8;
		b = (a*b + (255-a)*PIXB(t)) >> 8;
	}
	vid[y*width+x] = PIXRGB(r,g,b);
}

void VideoBuffer::DrawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int a)
{
	int dx, dy, startX, startY, e, x, y;
	bool reverseXY = false;

	dx = std::abs(x1-x2);
	dy = std::abs(y1-y2);
	startX = (x1<x2) ? 1 : -1;
	startY = (y1<y2) ? 1 : -1;
	x = x1;
	y = y1;

	if (dy > dx)
	{
		dx = dx + dy;
		dy = dx - dy;
		dx = dx - dy;
		reverseXY = true;
	}

	e = (dy<<2) - dx;
	for (int i = 0; i <= dx; i++)
	{
		if (x >= 0 && y >= 0 && x < width && y < height)
			DrawPixel(x, y, r, g, b, a);
		if (e >= 0)
		{
			if (reverseXY)
				x = x + startX;
			else
				y = y + startY;
			e = e - (dx<<2);
		}
		if (reverseXY)
			y = y + startY;
		else
			x = x + startX;
		e = e + (dy<<2);
	}
}

void VideoBuffer::DrawRect(int x, int y, int w, int h, int r, int g, int b, int a)
{
	if (x < 0)
	{
		if (x+w-1 >= 0)
		{
			w += x;
			x = 0;
		}
		else
			return;
	}
	if (y < 0)
	{
		if (y+h-1 >= 0)
		{
			h += y;
			y = 0;
		}
		else
			return;
	}
	for (int i = 0; i < w && x+i < width; i++)
	{
		if (y < height)
			DrawPixel(x+i, y, r, g, b, a);
		if (y+h-1 < height)
			DrawPixel(x+i, y+h-1, r, g, b, a);
	}
	for (int i = 1; i < h-1 && y+i < height; i++)
	{
		if (x < width)
			DrawPixel(x, y+i, r, g, b, a);
		if (x+w < width)
			DrawPixel(x+w-1, y+i, r, g, b, a);
	}
}

//draws a rectangle and fills it in as well.
void VideoBuffer::FillRect(int x, int y, int w, int h, int r, int g, int b, int a)
{
	if (x < 0)
	{
		if (x+w >= 0)
		{
			w += x;
			x = 0;
		}
		else
			return;
	}
	if (y < 0)
	{
		if (y+h >= 0)
		{
			h += y;
			y = 0;
		}
		else
			return;
	}
	if (x+w >= width)
		w = width-x;
	if (y+h >= height)
		h = height-y;

	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++)
			DrawPixel(x+i, y+j, r, g, b, a);
}

int VideoBuffer::DrawChar(int x, int y, unsigned char c, int r, int g, int b, int a)
{
	int w, bn = 0, ba = 0;
	char *rp = (char*)font_data + font_ptrs[c];
	w = *(rp++);
	for (int j = 0; j < FONT_H; j++)
		for (int i = 0; i < w; i++)
		{
			if (!bn)
			{
				ba = *(rp++);
				bn = 8;
			}
			if (x+i >= 0 && y+j >= 0 && x+i < width && y+j < height)
				DrawPixel(x+i, y+j, r, g, b, ((ba&3)*a)/3);
			ba >>= 2;
			bn -= 2;
		}
	return x + w;
}

int VideoBuffer::DrawText(int x, int y, std::string s, int r, int g, int b, int a)
{
	if (a == 0)
		return x;
	int startX = x;
	bool highlight = false;
	int oldR = r, oldG = g, oldB = b;
	for (int i = 0; i < s.length(); i++)
	{
		switch (s[i])
		{
		case '\n':
		case '\r':
			x = startX;
			y += FONT_H+2;
			if (highlight && s.length() > i+1 && (s[i+1] == '\n' || s[i+1] == '\r' || (s.length() > i+2 && s[i+1] == '\x01' && (s[i+2] == '\n' || s[i+2] == '\r'))))
			{
				FillRect(x, y-2, font_data[font_ptrs[' ']], FONT_H+2, 0, 0, 255, 127);
			}
			break;
		case '\x0F':
			if (s.length() < i+3)
				break;
			oldR = r;
			oldG = g;
			oldB = b;
			r = (unsigned char)s[i+1];
			g = (unsigned char)s[i+2];
			b = (unsigned char)s[i+3];
			i += 3;
			break;
		case '\x0E':
			r = oldR;
			g = oldG;
			b = oldB;
			break;
		case '\x01':
			highlight = !highlight;
			break;
		case '\x02':
			DrawLine(x, y-1, x, y+FONT_H-2, 255, 255, 255, 255);
			break;
		// old colour codes, expect a single character after them
		case '\b':
			if (s.length() < i+1)
				break;
			switch (s[i+1])
			{
			case 'w':
				r = g = b = 255;
				break;
			case 'g':
				r = g = b = 192;
				break;
			case 'o':
				r = 255;
				g = 216;
				b = 32;
				break;
			case 'r':
				r = 255;
				g = b = 0;
				break;
			case 'l':
				r = 255;
				g = b = 75;
				break;
			case 'b':
				r = g = 0;
				b = 255;
				break;
			case 't':
				b = 255;
				g = 170;
				r = 32;
				break;
			case 'p':
				b = 100;
				g = 10;
				r = 100;
				break;
			}
			i++;
			break;
		default:
			int oldX = x;
			x = DrawChar(x, y, s[i], r, g, b, a);
			if (highlight)
			{
				FillRect(oldX, y-2, font_data[font_ptrs[s[i]]], FONT_H+2, 0, 0, 255, 127);
			}
		}
	}
	return x;
}

// static method that returns the width of a character
int VideoBuffer::CharSize(unsigned char c)
{
	return font_data[font_ptrs[static_cast<int>(c)]];
}

// static method that returns the width and height of a string
Point VideoBuffer::TextSize(std::string s)
{
	int x = 0;
	int width = 0;
	int height = FONT_H;
	for (int i = 0; i < s.length(); i++)
	{
		switch (s[i])
		{
		case '\n':
		case '\r':
			if (x > width)
				width = x;
			x = 0;
			height += FONT_H+2;
			break;
		case '\x0F':
			if (s.length() < i+3)
				break;
			i += 3;
			break;
		case '\x0E':
		case '\x01':
		case '\x02':
			break;
		case '\b':
			if (s.length() < i+1)
				break;
			i++;
			break;
		default:
			x += CharSize(static_cast<unsigned char>(s[i]));
		}
	}
	if (x > width)
		width = x;
	return Point(width, height);
}

void VideoBuffer::DrawImage(pixel *img, int x, int y, int w, int h)
{
	// image out of bounds (scrollable windows)
	if (y < 0)
	{
		// only draw the height we need
		if (y+h >= 0)
		{
			img -= y*h;
			h += y;
			y = 0;
		}
		else
			return;
	}
	for (int j = 0; j < h; j++)
		for (int i = 0; i < w; i++)
		{
			int r = PIXR(*img);
			int g = PIXG(*img);
			int b = PIXB(*img);
			if (x >= 0 && x < width && y < height)
				DrawPixel(x+i, y+j, r, g, b, 255);
			img++;
		}
}
