/**
 * Powder Toy - user interface
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <bzlib.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#ifdef WIN
#include <direct.h>
#define getcwd _getcwd
#ifdef _MSC_VER
#undef rmdir
#define rmdir _rmdir //deprecated in visual studio
#endif
#endif
#if defined(LIN) || defined(MACOSX)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "http.h"
#include "md5.h"
#include "font.h"
#include "defines.h"
#include "powder.h"
#include "interface.h"
#include "misc.h"
#include "console.h"
#include "luaconsole.h"
#include "gravity.h"
#include "images.h"

#include "powdergraphics.h"
#include "save.h"
#include "hud.h"
#include "cJSON.h"
#include "json/json.h"
#include "update.h"

#include "game/Menus.h"
#include "game/ToolTip.h"
#include "game/Download.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"
#include "simulation/ToolNumbers.h"
#include "simulation/GolNumbers.h"

#include "gui/profile/ProfileViewer.h"

unsigned short sdl_mod;
int sdl_key, sdl_rkey, sdl_wheel, sdl_ascii, sdl_zoom_trig=0;

char *shift_0="`1234567890-=[]\\;',./";
char *shift_1="~!@#$%^&*()_+{}|:\"<>?";

int svf_messages = 0;
int svf_login = 0;
int svf_admin = 0;
int svf_mod = 0;
char svf_user[64] = "";
char svf_user_id[64] = "";
char svf_pass[64] = "";
char svf_session_id[64] = "";
char svf_session_key[64] = "";

int svf_open = 0;
int svf_own = 0;
int svf_myvote = 0;
int svf_publish = 0;
char svf_filename[255] = "";
int svf_fileopen = 0;
char svf_id[16] = "";
char svf_name[64] = "";
char svf_description[255] = "";
char svf_author[64] = "";
char svf_tags[256] = "";
void *svf_last = NULL;
int svf_lsize;

char *search_ids[GRID_X*GRID_Y];
char *search_dates[GRID_X*GRID_Y];
int   search_votes[GRID_X*GRID_Y];
int   search_publish[GRID_X*GRID_Y];
int	  search_scoredown[GRID_X*GRID_Y];
int	  search_scoreup[GRID_X*GRID_Y];
char *search_names[GRID_X*GRID_Y];
char *search_owners[GRID_X*GRID_Y];
void *search_thumbs[GRID_X*GRID_Y];
int   search_thsizes[GRID_X*GRID_Y];

int search_own = 0;
int search_fav = 0;
int search_date = 0;
int search_page = 0;
int p1_extra = 0;
char search_expr[256] = "";

char server_motd[512] = "";

char *tag_names[TAG_MAX];
int tag_votes[TAG_MAX];

int zoom_en = 0;
int zoom_x=(XRES-ZSIZE_D)/2, zoom_y=(YRES-ZSIZE_D)/2;
int zoom_wx=0, zoom_wy=0;
unsigned char ZFACTOR = 8;
unsigned char ZSIZE = 32;

int hud_menunum = 0;
int has_quit = 0;
int dateformat = 7;
int show_ids = 0;

int drawgrav_enable = 0;

void get_sign_pos(int i, int *x0, int *y0, int *w, int *h)
{
	//Changing width if sign have special content
	if (strcmp(signs[i].text, "{p}")==0)
		*w = textwidth("Pressure: -000.00");

	if (strcmp(signs[i].text, "{t}")==0)
		*w = textwidth("Temp: 0000.00");

	if (!sregexp(signs[i].text, "^{[ct]:[0-9]*|.*}$") || !sregexp(signs[i].text, "^{s:.*|.*}$") || !sregexp(signs[i].text, "^{b|.*}$"))
	{
		int sldr, startm;
		char buff[256];
		memset(buff, 0, sizeof(buff));
		for (sldr=2; signs[i].text[sldr-1] != '|'; sldr++)
			startm = sldr + 1;

		sldr = startm;
		while (signs[i].text[sldr] != '}')
		{
			buff[sldr - startm] = signs[i].text[sldr];
			sldr++;
		}
		*w = textwidth(buff) + 5;
	}

	//Usual width
	if (strcmp(signs[i].text, "{p}") && strcmp(signs[i].text, "{t}") && sregexp(signs[i].text, "^{[ct]:[0-9]*|.*}$") && sregexp(signs[i].text, "^{s:.*|.*}$") && sregexp(signs[i].text, "^{b|.*}$"))
		*w = textwidth(signs[i].text) + 5;
	*h = 14;
	*x0 = (signs[i].ju == 2) ? signs[i].x - *w :
	      (signs[i].ju == 1) ? signs[i].x - *w/2 : signs[i].x;
	*y0 = (signs[i].y > 18) ? signs[i].y - 18 : signs[i].y + 4;
}

void add_sign_ui(pixel *vid_buf, int mx, int my)
{
	int i, w, h, x, y, nm=0, ju;
	int x0=(XRES-204)/2,y0=(YRES-80)/2,b=1,bq;
	ui_edit ed;

	// if currently moving a sign, stop doing so
	if (MSIGN != -1)
	{
		MSIGN = -1;
		return;
	}

	// check if it is an existing sign
	for (i=0; i<MAXSIGNS; i++)
		if (signs[i].text[0])
		{
			get_sign_pos(i, &x, &y, &w, &h);
			if (mx>=x && mx<=x+w && my>=y && my<=y+h)
				break;
		}
	// else look for empty spot
	if (i >= MAXSIGNS)
	{
		nm = 1;
		for (i=0; i<MAXSIGNS; i++)
			if (!signs[i].text[0])
				break;
	}
	if (i >= MAXSIGNS)
		return;
	if (nm)
	{
		signs[i].x = mx;
		signs[i].y = my;
		signs[i].ju = 1;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	ui_edit_init(&ed, x0+25, y0+25, 184, 14);
	ed.def = "[message]";
	ed.cursor = ed.cursorstart = strlen(signs[i].text);
	strcpy(ed.str, signs[i].text);
	ju = signs[i].ju;

	fillrect(vid_buf, -1, -1, XRES, YRES+MENUSIZE, 0, 0, 0, 192);
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		drawrect(vid_buf, x0, y0, 217, 80, 192, 192, 192, 255);
		clearrect(vid_buf, x0+1, y0+1, 216, 79);
		drawtext(vid_buf, x0+8, y0+8, nm ? "New sign:" : "Edit sign:", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+23, "\xA1", 32, 64, 128, 255);
		drawtext(vid_buf, x0+12, y0+23, "\xA0", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+20, 201, 16, 192, 192, 192, 255);
		ui_edit_draw(vid_buf, &ed);
		drawtext(vid_buf, x0+8, y0+46, "Justify:", 255, 255, 255, 255);
		draw_icon(vid_buf, x0+50, y0+42, (char)0x9D, ju == 0);
		draw_icon(vid_buf, x0+68, y0+42, (char)0x9E, ju == 1);
		draw_icon(vid_buf, x0+86, y0+42, (char)0x9F, ju == 2);





		if (!nm)
		{
			drawtext(vid_buf, x0+138, y0+45, "\x86", 160, 48, 32, 255);
			drawtext(vid_buf, x0+138, y0+45, "\x85", 255, 255, 255, 255);
			drawtext(vid_buf, x0+152, y0+46, "Delete", 255, 255, 255, 255);
			drawrect(vid_buf, x0+134, y0+42, 50, 15, 255, 255, 255, 255);
			drawrect(vid_buf,x0+104,y0+42,26,15,255,255,255,255);
			drawtext(vid_buf, x0+110, y0+48, "Mv.", 255, 255, 255, 255);
		}

		drawtext(vid_buf, x0+5, y0+69, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+64, 217, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		ui_edit_process(mx, my, b, bq, &ed);

		if (!b && bq)
		{
			if (mx>=x0+50 && mx<=x0+67 && my>=y0+42 && my<=y0+59)
				ju = 0;
			if (mx>=x0+68 && mx<=x0+85 && my>=y0+42 && my<=y0+59)
				ju = 1;
			if (mx>=x0+86 && mx<=x0+103 && my>=y0+42 && my<=y0+59)
				ju = 2;

			if (!nm && mx>=x0+104 && mx<=x0+130 && my>=y0+42 && my<=y0+59)
			{
				MSIGN = i;
				break;
			}
			if (mx>=x0+9 && mx<x0+23 && my>=y0+22 && my<y0+36)
				break;
			if (mx>=x0 && mx<x0+217 && my>=y0+64 && my<=y0+80)
				break;

			if (!nm && mx>=x0+134 && my>=y0+42 && mx<=x0+184 && my<=y0+59)
			{
				signs[i].text[0] = 0;
				return;
			}

			if (mx < x0 || my < y0 || mx > x0+217 || my > y0+80)
				return;
		}

		if (sdl_key == SDLK_RETURN)
			break;
		if (sdl_key == SDLK_ESCAPE)
		{
			return;
		}
	}

	strcpy(signs[i].text, ed.str);
	signs[i].ju = ju;
}

void ui_edit_init(ui_edit *ed, int x, int y, int w, int h)
{
	ed->x = x;
	ed->y = y;
	ed->w = w;
	ed->h = h;
	ed->nx = 1;
	ed->def = "";
	strcpy(ed->str, "");
	ed->focus = 1;
	ed->alwaysFocus = 0;
	ed->hide = 0;
	ed->multiline = 0;
	ed->limit = 255;
	ed->cursor = ed->cursorstart = 0;
	ed->highlightstart = ed->highlightlength = 0;
	ed->resizable = ed->resizespeed = 0;
	ed->lastClick = ed->numClicks = 0;
	ed->overDelete = 0;
}

int ui_edit_draw(pixel *vid_buf, ui_edit *ed)
{
	int cx, i, cy, ret = 12;
	char echo[1024], *str, highlightstr[1024];

	if (ed->cursor>ed->cursorstart)
	{
		ed->highlightstart = ed->cursorstart;
		ed->highlightlength = ed->cursor-ed->cursorstart;
	}
	else
	{
		ed->highlightstart = ed->cursor;
		ed->highlightlength = ed->cursorstart-ed->cursor;
	}
	if (ed->hide)
	{
		for (i=0; ed->str[i]; i++)
			echo[i] = (char)0x8D;
		echo[i] = 0;
		str = echo;
	}
	else
		str = ed->str;

	if (ed->str[0])
	{
		int deletecolour = 127+ed->overDelete*64;
		if (ed->multiline) {
			ret = drawtextwrap(vid_buf, ed->x, ed->y, ed->w-14, 0, str, 255, 255, 255, 255);
			if (ed->highlightlength)
			{
				strncpy(highlightstr, &str[ed->highlightstart], ed->highlightlength);
				highlightstr[ed->highlightlength] = 0;
				drawhighlightwrap(vid_buf, ed->x, ed->y, ed->w-14, 0, ed->str, ed->highlightstart, ed->highlightlength);
			}
			drawtext(vid_buf, ed->x+ed->w-11, ed->y-1, "\xAA", deletecolour, deletecolour, deletecolour, 255);
		} else {
			ret = drawtext(vid_buf, ed->x, ed->y, str, 255, 255, 255, 255);
			if (ed->highlightlength)
			{
				strncpy(highlightstr, &str[ed->highlightstart], ed->highlightlength);
				highlightstr[ed->highlightlength] = 0;
				drawhighlight(vid_buf, ed->x+textwidth(str)-textwidth(&str[ed->highlightstart]), ed->y, highlightstr);
			}
			drawtext(vid_buf, ed->x+ed->w-11, ed->y-1, "\xAA", deletecolour, deletecolour, deletecolour, 255);
		}
	}
	else if (!ed->focus)
		drawtext(vid_buf, ed->x, ed->y, ed->def, 128, 128, 128, 255);
	if (ed->focus && ed->numClicks < 2)
	{
		if (ed->multiline) {
			textnpos(str, ed->cursor, ed->w-14, &cx, &cy);
		} else {
			cx = textnwidth(str, ed->cursor);
			cy = 0;
		}

		for (i=-3; i<9; i++)
			drawpixel(vid_buf, ed->x+cx, ed->y+i+cy, 255, 255, 255, 255);
	}
	if (ed->resizable && ed->multiline)
	{
		int diff = ((ret+2)-ed->h)/5;
		if (diff == 0)
			ed->h = ret+2;
		else
			ed->h += diff;
		ret = ed->h-2;
	}
	return ret;
}

void findWordPosition(const char *s, int position, int *cursorStart, int *cursorEnd, const char *spaces)
{
	int wordLength = 0, totalLength = 0, strLength = strlen(s);
	while (totalLength < strLength)
	{
		wordLength = strcspn(s, spaces);
		if (totalLength + wordLength >= position)
		{
			*cursorStart = totalLength;
			*cursorEnd = totalLength+wordLength;
			return;
		}
		s += wordLength+1;
		totalLength += wordLength+1;
	}
	*cursorStart = totalLength;
	*cursorEnd = totalLength+wordLength;
}

void ui_edit_process(int mx, int my, int mb, int mbq, ui_edit *ed)
{
	char ch, ts[2], echo[1024], *str = ed->str;
	int l, i;

	if (ed->alwaysFocus)
		ed->focus = 1;
	if (ed->cursor>ed->cursorstart)
	{
		ed->highlightstart = ed->cursorstart;
		ed->highlightlength = ed->cursor-ed->cursorstart;
	}
	else
	{
		ed->highlightstart = ed->cursor;
		ed->highlightlength = ed->cursorstart-ed->cursor;
	}
	if (ed->hide)
	{
		for (i=0; ed->str[i]; i++)
			echo[i] = (char)0x8D;
		echo[i] = 0;
		str = echo;
	}
	if (mx>=ed->x+ed->w-11 && mx<ed->x+ed->w && my>=ed->y-5 && my<ed->y+11) //on top of the delete button
	{
		if (!ed->overDelete && !mb)
			ed->overDelete = 1;
		else if (mb && !mbq)
			ed->overDelete = 2;
		if (!mb && mbq && ed->overDelete == 2)
		{
			ed->focus = 1;
			ed->cursor = 0;
			ed->str[0] = 0;
			if (!ed->numClicks)
				ed->numClicks = 1;
		}
	}
	else if (mb && (ed->focus || !mbq) && mx>=ed->x-ed->nx && mx<ed->x+ed->w && my>=ed->y-5 && my<ed->y+(ed->multiline?ed->h:11)) //clicking / dragging over textbox
	{
		ed->focus = 1;
		ed->overDelete = 0;
		ed->cursor = textposxy(str, ed->w-14, mx-ed->x, my-ed->y);
		if (!ed->numClicks)
			ed->numClicks = 1;
	}
	else if (mb && !mbq) //a first click anywhere outside
	{
		if (!ed->alwaysFocus)
		{
			ed->focus = 0;
			ed->cursor = ed->cursorstart = 0;
		}
	}
	else //when a click has moved outside the textbox
	{
		if (ed->numClicks && mb && (ed->focus || !mbq))
		{
			if (my <= ed->y-5)
				ed->cursor = 0;
			else if (my >= ed->y+ed->h)
				ed->cursor = strlen(ed->str);
		}
		ed->overDelete = 0;
	}
	if (ed->focus && sdl_key)
	{
		l = strlen(ed->str);
		switch (sdl_key)
		{
		case SDLK_HOME:
			ed->cursorstart = 0;
			if (!(sdl_mod & KMOD_SHIFT))
				ed->cursor = 0;
			break;
		case SDLK_END:
			ed->cursorstart = l;
			if (!(sdl_mod & KMOD_SHIFT))
				ed->cursor = l;
			break;
		case SDLK_LEFT:
			if (sdl_mod & (KMOD_LSHIFT|KMOD_RSHIFT))
			{
				if (ed->cursor)
					ed->cursor--;
				else if (ed->cursorstart < ed->cursor && ed->cursorstart)
					ed->cursorstart--;
			}
			else
			{
				if (ed->cursor > 0)
					ed->cursor--;
				if (ed->cursor > 0 && ed->str[ed->cursor-1] == '\b')
				{
					ed->cursor--;
				}
				ed->cursorstart = ed->cursor;
			}
			break;
		case SDLK_RIGHT:
			if (sdl_mod & (KMOD_LSHIFT|KMOD_RSHIFT))
			{
				if (ed->cursor < l)
					ed->cursor++;
				else if (ed->cursorstart > ed->cursor && ed->cursorstart < l)
					ed->cursorstart++;
			}
			else
			{
				if (ed->cursor < l && ed->str[ed->cursor] == '\b')
				{
					ed->cursor++;
				}
				if (ed->cursor < l)
					ed->cursor++;
				ed->cursorstart = ed->cursor;
			}
			break;
		case SDLK_DELETE:
			if (sdl_mod & (KMOD_CTRL|KMOD_META))
			{
				int start, end;
				const char *spaces = " .,!?\n";
				findWordPosition(ed->str, ed->cursor+1, &start, &end, spaces);
				if (end > ed->cursor)
				{
					memmove(ed->str+ed->cursor, ed->str+end, l-ed->cursor);
					break;
				}
			}
			if (ed->highlightlength)
			{
				memmove(ed->str+ed->highlightstart, ed->str+ed->highlightstart+ed->highlightlength, l-ed->highlightstart);
				ed->cursor = ed->highlightstart;
			}
			else if (ed->cursor < l)
			{
				if (ed->str[ed->cursor] == '\b')
				{
					memmove(ed->str+ed->cursor, ed->str+ed->cursor+2, l-ed->cursor);
				}
				else
					memmove(ed->str+ed->cursor, ed->str+ed->cursor+1, l-ed->cursor);
			}
			ed->cursorstart = ed->cursor;
			break;
		case SDLK_BACKSPACE:
			if (sdl_mod & (KMOD_CTRL|KMOD_META))
			{
				int start, end;
				const char *spaces = " .,!?\n";
				findWordPosition(ed->str, ed->cursor-1, &start, &end, spaces);
				if (start < ed->cursor)
				{
					memmove(ed->str+start, ed->str+ed->cursor, l-ed->cursor+1);
					ed->cursor = start;
					break;
				}
			}
			if (ed->highlightlength)
			{
				memmove(ed->str+ed->highlightstart, ed->str+ed->highlightstart+ed->highlightlength, l-ed->highlightstart);
				ed->cursor = ed->highlightstart;
			}
			else if (ed->cursor > 0)
			{
				ed->cursor--;
				memmove(ed->str+ed->cursor, ed->str+ed->cursor+1, l-ed->cursor);
				if (ed->cursor > 0 && ed->str[ed->cursor-1] == '\b')
				{
					ed->cursor--;
					memmove(ed->str+ed->cursor, ed->str+ed->cursor+1, l-ed->cursor);
				}
			}
			ed->cursorstart = ed->cursor;
			break;
		default:
			if(sdl_mod & (KMOD_CTRL|KMOD_META) && sdl_key=='c')//copy
			{
				if (ed->highlightlength)
				{
					char highlightstr[1024];
					strncpy(highlightstr, &str[ed->highlightstart], ed->highlightlength);
					highlightstr[ed->highlightlength] = 0;
					clipboard_push_text(highlightstr);
				}
				else
					clipboard_push_text(ed->str);
				break;
			}
			else if(sdl_mod & (KMOD_CTRL|KMOD_META) && sdl_key=='v')//paste
			{
				char* paste = clipboard_pull_text();
				if (!paste)
					return;
				int pl = strlen(paste);
				if ((textwidth(str)+textwidth(paste) > ed->w-14 && !ed->multiline) || (pl+(int)strlen(ed->str)>ed->limit) || (float)(((textwidth(str)+textwidth(paste))/(ed->w-14)*12) > ed->h && ed->multiline && ed->limit != 1023))
					break;
				if (ed->highlightlength)
				{
					memmove(ed->str+ed->highlightstart, ed->str+ed->highlightstart+ed->highlightlength, l-ed->highlightstart);
					ed->cursor = ed->highlightstart;
				}
				memmove(ed->str+ed->cursor+pl, ed->str+ed->cursor, l-ed->cursor+1);
				memcpy(ed->str+ed->cursor,paste,pl);
				ed->cursor += pl;
				ed->cursorstart = ed->cursor;
				break;
			}
			else if(sdl_mod & (KMOD_CTRL|KMOD_META) && sdl_key=='a')//highlight all
			{
				ed->cursorstart = 0;
				ed->cursor = l;
			}
			if ((sdl_mod & (KMOD_CTRL|KMOD_META)) && (svf_admin || svf_mod))
			{
				if (ed->cursor > 1 && ed->str[ed->cursor-2] == '\b')
					break;
				if (sdl_key=='w' || sdl_key=='g' || sdl_key=='o' || sdl_key=='r' || sdl_key=='l' || sdl_key=='b' || sdl_key=='t')// || sdl_key=='p')
					ch = sdl_key;
				else
					break;
				if (ed->highlightlength)
				{
					memmove(ed->str+ed->highlightstart, ed->str+ed->highlightstart+ed->highlightlength, l-ed->highlightstart);
					ed->cursor = ed->highlightstart;
				}
				if ((textwidth(str) > ed->w-14 && !ed->multiline) || (float)(((textwidth(str))/(ed->w-14)*12) > ed->h && ed->multiline && ed->limit != 1023))
					break;
				memmove(ed->str+ed->cursor+2, ed->str+ed->cursor, l+2-ed->cursor);
				ed->str[ed->cursor] = '\b';
				ed->str[ed->cursor+1] = ch;
				ed->cursor+=2;
				ed->cursorstart = ed->cursor;
			}
			if (sdl_ascii>=' ' && sdl_ascii<127 && l<ed->limit)
			{
				if (ed->highlightlength)
				{
					memmove(ed->str+ed->highlightstart, ed->str+ed->highlightstart+ed->highlightlength, l-ed->highlightstart);
					ed->cursor = ed->highlightstart;
				}
				ch = sdl_ascii;
				ts[0]=ed->hide?0x8D:ch;
				ts[1]=0;
				if ((textwidth(str)+textwidth(ts) > ed->w-14 && !ed->multiline) || (float)(((textwidth(str)+textwidth(ts))/(ed->w-14)*12) > ed->h && ed->multiline && ed->limit != 1023))
					break;
				memmove(ed->str+ed->cursor+1, ed->str+ed->cursor, l+1-ed->cursor);
				ed->str[ed->cursor] = ch;
				ed->cursor++;
				ed->cursorstart = ed->cursor;
			}
			break;
		}
	}
	if (mb && !mbq && ed->focus && mx>=ed->x-ed->nx && mx<ed->x+ed->w && my>=ed->y-5 && my<ed->y+(ed->multiline?ed->h:11))
	{
		int clickTime = SDL_GetTicks()-ed->lastClick;
		ed->lastClick = SDL_GetTicks();
		if (clickTime < 300)
		{
			ed->clickPosition = ed->cursor;
			if (ed->numClicks == 2)
				ed->numClicks = 3;
			else
				ed->numClicks = 2;
		}
		ed->cursorstart = ed->cursor;
	}
	else if (!mb && SDL_GetTicks()-ed->lastClick > 300)
		ed->numClicks = 0;
	if (ed->numClicks >= 2)
	{
		int start = 0, end = strlen(ed->str);
		const char *spaces = " .,!?_():;~\n";
		if (ed->numClicks == 3)
			spaces = "\n";
		findWordPosition(ed->str, ed->cursor, &start, &end, spaces);
		if (start < ed->clickPosition)
		{
			ed->cursorstart = start;
			findWordPosition(ed->str, ed->clickPosition, &start, &end, spaces);
			ed->cursor = end;
		}
		else
		{
			ed->cursorstart = end;
			findWordPosition(ed->str, ed->clickPosition, &start, &end, spaces);
			ed->cursor = start;
		}
	}
}

void ui_label_init(ui_label *label, int x, int y, int w, int h)
{
	label->x = x;
	label->y = y;
	label->w = w;
	label->h = h;
	label->maxHeight = 0;
	strcpy(label->str, "");
	label->focus = 0;
	label->multiline = 1;
	label->cursor = label->cursorstart = 0;
	label->highlightstart = label->highlightlength = 0;
	label->lastClick = label->numClicks = 0;
}

int ui_label_draw(pixel *vid_buf, ui_label *ed)
{
	char highlightstr[1024];
	int ret = 0, heightlimit = ed->maxHeight;

	if (ed->cursor>ed->cursorstart)
	{
		ed->highlightstart = ed->cursorstart;
		ed->highlightlength = ed->cursor-ed->cursorstart;
	}
	else
	{
		ed->highlightstart = ed->cursor;
		ed->highlightlength = ed->cursorstart-ed->cursor;
	}

	if (ed->str[0])
	{
		if (ed->multiline) {
			ret = drawtextwrap(vid_buf, ed->x, ed->y, ed->w-14, heightlimit, ed->str, 255, 255, 255, 185);
			if (ed->maxHeight)
				ed->h = ret;
			if (ed->highlightlength)
			{
				drawhighlightwrap(vid_buf, ed->x, ed->y, ed->w-14, heightlimit, ed->str, ed->highlightstart, ed->highlightlength);
			}
		} else {
			ret = drawtext(vid_buf, ed->x, ed->y, ed->str, 255, 255, 255, 255);
			if (ed->highlightlength)
			{
				strncpy(highlightstr, &ed->str[ed->highlightstart], ed->highlightlength);
				highlightstr[ed->highlightlength] = 0;
				drawhighlight(vid_buf, ed->x+textwidth(ed->str)-textwidth(&ed->str[ed->highlightstart]), ed->y, highlightstr);
			}
		}
	}
	return ret;
}

void ui_label_process(int mx, int my, int mb, int mbq, ui_label *ed)
{
	if (ed->cursor>ed->cursorstart)
	{
		ed->highlightstart = ed->cursorstart;
		ed->highlightlength = ed->cursor-ed->cursorstart;
	}
	else
	{
		ed->highlightstart = ed->cursor;
		ed->highlightlength = ed->cursorstart-ed->cursor;
	}
	
	if (mb && (ed->focus || !mbq) && mx>=ed->x && mx<ed->x+ed->w && my>=ed->y-2 && my<ed->y+(ed->multiline?ed->h:11))
	{
		ed->focus = 1;
		ed->cursor = textposxy(ed->str, ed->w-14, mx-ed->x, my-ed->y);
		if (!ed->numClicks)
			ed->numClicks = 1;
	}
	else if (mb && !mbq)
	{
		ed->focus = 0;
		ed->cursor = ed->cursorstart = 0;
	}
	else if (mb && (ed->focus || !mbq))
	{
		if (my <= ed->y-2)
			ed->cursor = 0;
		else if (my >= ed->y+ed->h)
			ed->cursor = strlen(ed->str);
	}

	if (ed->focus && sdl_key)
	{
		int l = strlen(ed->str);
		if (sdl_key == SDLK_HOME && (sdl_mod & KMOD_SHIFT))
		{
			ed->cursorstart = 0;
		}
		else if (sdl_key == SDLK_END && (sdl_mod & KMOD_SHIFT))
		{
			ed->cursorstart = l;
		}
		else if (sdl_key == SDLK_LEFT)
		{
			if (sdl_mod & (KMOD_LSHIFT|KMOD_RSHIFT))
			{
				if (ed->cursor)
					ed->cursor--;
				else if (ed->cursorstart < ed->cursor && ed->cursorstart)
					ed->cursorstart--;
			}
		}
		else if (sdl_key == SDLK_RIGHT)
		{
			if (sdl_mod & (KMOD_LSHIFT|KMOD_RSHIFT))
			{
				if (ed->cursor < l)
					ed->cursor++;
				else if (ed->cursorstart > ed->cursor && ed->cursorstart < l)
					ed->cursorstart++;
			}
		}
		else if(sdl_mod & (KMOD_CTRL|KMOD_META) && sdl_key=='c')//copy
		{
			if (ed->highlightlength)
			{
				char highlightstr[1024];
				strncpy(highlightstr, &ed->str[ed->highlightstart], ed->highlightlength);
				highlightstr[ed->highlightlength] = 0;
				clipboard_push_text(highlightstr);
			}
			else
				clipboard_push_text(ed->str);
		}
		else if(sdl_mod & (KMOD_CTRL|KMOD_META) && sdl_key=='a')//highlight all
		{
			ed->cursorstart = 0;
			ed->cursor = l;
		}
	}
	if (mb && !mbq && ed->focus)
	{
		int clickTime = SDL_GetTicks()-ed->lastClick;
		ed->lastClick = SDL_GetTicks();
		if (clickTime < 300)
		{
			ed->clickPosition = ed->cursor;
			if (ed->numClicks == 2)
				ed->numClicks = 3;
			else
				ed->numClicks = 2;
		}
		ed->cursorstart = ed->cursor;
	}
	else if (!mb && SDL_GetTicks()-ed->lastClick > 300)
		ed->numClicks = 0;
	if (ed->numClicks >= 2)
	{
		int start = 0, end = strlen(ed->str);
		const char *spaces = " .,!?_():;~\n";
		if (ed->numClicks == 3)
			spaces = "\n";
		findWordPosition(ed->str, ed->cursor, &start, &end, spaces);
		if (start < ed->clickPosition)
		{
			ed->cursorstart = start;
			findWordPosition(ed->str, ed->clickPosition, &start, &end, spaces);
			ed->cursor = end;
		}
		else
		{
			ed->cursorstart = end;
			findWordPosition(ed->str, ed->clickPosition, &start, &end, spaces);
			ed->cursor = start;
		}
	}
}

void ui_list_process(pixel * vid_buf, int mx, int my, int mb, ui_list *ed)
{
	int i, ystart, selected = 0;
	if(ed->selected > ed->count || ed->selected < -1)
	{
		ed->selected = -1;
	}
	if(mx > ed->x && mx < ed->x+ed->w && my > ed->y && my < ed->y+ed->h)
	{
		ed->focus = 1;
		if(mb)
		{
			ystart = ed->y-(ed->count*8);
			if(ystart < 5)
				ystart = 5;
			while (!sdl_poll())
			{
				mb = mouse_get_state(&mx, &my);
				if (!mb)
					break;
			}

			while (!sdl_poll() && !selected)
			{
				mb = mouse_get_state(&mx, &my);
				for(i = 0; i < ed->count; i++)
				{
					if(mx > ed->x && mx < ed->x+ed->w && my > (ystart + i*16) && my < (ystart + i * 16) + 16)
					{
						if(mb){
							ed->selected = i;
							selected = 1;
						}
						fillrect(vid_buf, ed->x, ystart + i * 16, ed->w, 16, 255, 255, 255, 25);
						drawtext(vid_buf, ed->x + 4, ystart + i * 16 + 5, ed->items[i], 255, 255, 255, 255);
					}
					else
					{
						drawtext(vid_buf, ed->x + 4, ystart + i * 16 + 5, ed->items[i], 192, 192, 192, 255);
					}
					draw_line(vid_buf, ed->x, ystart + i * 16, ed->x+ed->w, ystart + i * 16, 128, 128, 128, XRES+BARSIZE);
				}
				drawrect(vid_buf, ed->x, ystart, ed->w, ed->count*16, 255, 255, 255, 255);
#ifdef OGLR
				clearScreen(1.0f);
#endif
				sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
				clearrect(vid_buf, ed->x-1, ystart-1, ed->w+3, (ed->count*16)+3);

				if(!selected && mb)
					break;
			}
			while (!sdl_poll())
			{
				mb = mouse_get_state(&mx, &my);
				if (!mb)
					break;
			}
			
			if(ed->selected!=-1)
				strcpy(ed->str, ed->items[ed->selected]);
		}
	}
	else
	{
		ed->focus = 0;
	}
}

void ui_list_draw(pixel *vid_buf, ui_list *ed)
{
	if(ed->selected > ed->count || ed->selected < -1)
	{
		ed->selected = -1;
	}
	if (ed->focus)
	{
		drawrect(vid_buf, ed->x, ed->y, ed->w, ed->h, 255, 255, 255, 255);
	}
	else
	{
		drawrect(vid_buf, ed->x, ed->y, ed->w, ed->h, 192, 192, 192, 255);
	}
	if(ed->selected!=-1)
	{
		drawtext(vid_buf, ed->x+4, ed->y+5, ed->items[ed->selected], 255, 255, 255, 255);
	}
	else
	{
		drawtext(vid_buf, ed->x+4, ed->y+5, ed->def, 192, 192, 192, 255);
	}
}

void ui_checkbox_draw(pixel *vid_buf, ui_checkbox *ed)
{
	int w = 12;
	if (ed->checked)
	{
		drawtext(vid_buf, ed->x+2, ed->y+2, "\xCF", 128, 128, 128, 255);
	}
	if (ed->focus)
	{
		drawrect(vid_buf, ed->x, ed->y, w, w, 255, 255, 255, 255);
	}
	else
	{
		drawrect(vid_buf, ed->x, ed->y, w, w, 128, 128, 128, 255);
	}
}

void ui_checkbox_process(int mx, int my, int mb, int mbq, ui_checkbox *ed)
{
	int w = 12;

	if (mb && !mbq)
	{
		if (mx>=ed->x && mx<=ed->x+w && my>=ed->y && my<=ed->y+w)
		{
			ed->checked = (ed->checked)?0:1;
		}
	}
	else
	{
		if (mx>=ed->x && mx<=ed->x+w && my>=ed->y && my<=ed->y+w)
		{
			ed->focus = 1;
		}
		else
		{
			ed->focus = 0;
		}
	}
}

void ui_radio_draw(pixel *vid_buf, ui_checkbox *ed)
{
	if (ed->checked)
	{
		int nx, ny;
		for(nx=-6; nx<=6; nx++)
			for(ny=-6; ny<=6; ny++)
				if((nx*nx+ny*ny)<10)
					blendpixel(vid_buf, ed->x+6+nx, ed->y+6+ny, 128, 128, 128, 255);
	}
	if (ed->focus)
	{
		int nx, ny;
		for(nx=-6; nx<=6; nx++)
			for(ny=-6; ny<=6; ny++)
				if((nx*nx+ny*ny)<40 && (nx*nx+ny*ny)>28)
					blendpixel(vid_buf, ed->x+6+nx, ed->y+6+ny, 255, 255, 255, 255);
	}
	else
	{
		int nx, ny;
		for(nx=-6; nx<=6; nx++)
			for(ny=-6; ny<=6; ny++)
				if((nx*nx+ny*ny)<40 && (nx*nx+ny*ny)>28)
					blendpixel(vid_buf, ed->x+6+nx, ed->y+6+ny, 128, 128, 128, 255);
	}
}

void ui_radio_process(int mx, int my, int mb, int mbq, ui_checkbox *ed)
{
	int w = 12;

	if (mb && !mbq)
	{
		if (mx>=ed->x && mx<=ed->x+w && my>=ed->y && my<=ed->y+w)
		{
			ed->checked = (ed->checked)?0:1;
		}
	}
	else
	{
		if (mx>=ed->x && mx<=ed->x+w && my>=ed->y && my<=ed->y+w)
		{
			ed->focus = 1;
		}
		else
		{
			ed->focus = 0;
		}
	}
}

void ui_copytext_draw(pixel *vid_buf, ui_copytext *ed)
{
	int g = 180, i = 0;
	if (!ed->state)
	{
		if (ed->hover)
			i = 0;
		else
			i = 100;
		g = 255;
		drawtext(vid_buf, (ed->x+(ed->width/2))-(textwidth("Click the box below to copy the save ID")/2), ed->y-12, "Click the box below to copy the save ID", 255, 255, 255, 255-i);
	}
	else
	{
		i = 0;
		if (ed->state == 2)
			g = 230;
		else
			g = 190;
		drawtext(vid_buf, (ed->x+(ed->width/2))-(textwidth("Copied!")/2), ed->y-12, "Copied!", 255, 255, 255, 255-i);
	}

	drawrect(vid_buf, ed->x, ed->y, ed->width, ed->height, g, 255, g, 255-i);
	drawrect(vid_buf, ed->x+1, ed->y+1, ed->width-2, ed->height-2, g, 255, g, 100-i);
	drawtext(vid_buf, ed->x+6, ed->y+5, ed->text, g, 255, g, 230-i);
}

void ui_copytext_process(int mx, int my, int mb, int mbq, ui_copytext *ed)
{
	if (my>=ed->y && my<=ed->y+ed->height && mx>=ed->x && mx<=ed->x+ed->width)
	{
		if (mb && !mbq)
		{
			clipboard_push_text(ed->text);
			ed->state = 2;
		}
		ed->hover = 1;
	}
	else
		ed->hover = 0;

	if (ed->state == 2 && !(mb && ed->hover))
		ed->state = 1;
}

void ui_richtext_draw(pixel *vid_buf, ui_richtext *ed)
{
	ed->str[511] = 0;
	ed->printstr[511] = 0;
	drawtext(vid_buf, ed->x, ed->y, ed->printstr, 255, 255, 255, 255);
}

int markup_getregion(char *text, char *action, char *data, char *atext){
	int datamarker = 0;
	int terminator = 0;
	int minit;
	if (sregexp(text, "^{a:.*|.*}")==0)
	{
		*action = text[1];
		for (minit=3; text[minit-1] != '|'; minit++)
			datamarker = minit + 1;
		for (minit=datamarker; text[minit-1] != '}'; minit++)
			terminator = minit + 1;
		strncpy(data, text+3, datamarker-4);
		strncpy(atext, text+datamarker, terminator-datamarker-1);
		return terminator;
	}
	else
	{
		return 0;
	}	
}

void ui_richtext_settext(char *text, ui_richtext *ed)
{
	int pos = 0, action = 0, ppos = 0, ipos = 0;
	memset(ed->printstr, 0, 512);
	memset(ed->str, 0, 512);
	strcpy(ed->str, text);
	//strcpy(ed->printstr, text);
	for(action = 0; action < 6; action++){
		ed->action[action] = 0;	
		memset(ed->actiondata[action], 0, 256);
		memset(ed->actiontext[action], 0, 256);
	}
	action = 0;
	for(pos = 0; pos<512; ){
		if(!ed->str[pos])
			break;
		if(ed->str[pos] == '{'){
			int mulen = 0;
			mulen = markup_getregion(ed->str+pos, &ed->action[action], ed->actiondata[action], ed->actiontext[action]);
			if(mulen){
				ed->regionss[action] = ipos;
				ed->regionsf[action] = ipos + strlen(ed->actiontext[action]);
				//printf("%c, %s, %s [%d, %d]\n", ed->action[action], ed->actiondata[action], ed->actiontext[action], ed->regionss[action], ed->regionsf[action]);
				strcpy(ed->printstr+ppos, ed->actiontext[action]);
				ppos+=strlen(ed->actiontext[action]);
				ipos+=strlen(ed->actiontext[action]);
				pos+=mulen;
				action++;			
			} 
			else
			{
				pos++;			
			}
		} else {
			ed->printstr[ppos] = ed->str[pos];
			ppos++;
			pos++;
			ipos++;
			if(ed->str[pos] == '\b'){
				ipos-=2;			
			}
		}
	}
	ed->printstr[ppos] = 0;
	//printf("%s\n", ed->printstr);
}

void ui_richtext_process(int mx, int my, int mb, int mbq, ui_richtext *ed)
{
	int action = 0;
	int currentpos = 0;
	if(mx>ed->x && mx < ed->x+textwidth(ed->printstr) && my > ed->y && my < ed->y + 10 && mb && !mbq){
		currentpos = textwidthx(ed->printstr, mx-ed->x);
		for(action = 0; action < 6; action++){
			if(currentpos >= ed->regionss[action] && currentpos <= ed->regionsf[action])
			{	
				//Do action
				if(ed->action[action]=='a'){
					//Open link
					open_link(ed->actiondata[action]);	
				}
				break;
			}
		}
	}
}

void draw_svf_ui(pixel *vid_buf, int alternate)// all the buttons at the bottom
{
	//the open browser button
	if(alternate)
	{
		fillrect(vid_buf, 0, YRES+(MENUSIZE-16)-1, 18, 16, 255, 255, 255, 255);
		drawtext(vid_buf, 4, YRES+(MENUSIZE-14), "\x81", 0, 0, 0, 255);
	} else {
		drawtext(vid_buf, 4, YRES+(MENUSIZE-14), "\x81", 255, 255, 255, 255);
		drawrect(vid_buf, 1, YRES+(MENUSIZE-16), 16, 14, 255, 255, 255, 255);
	}

	// the reload button
	int c = (svf_last) ? 255 : 128;
	drawtext(vid_buf, 23, YRES+(MENUSIZE-14), "\x91", c, c, c, 255);
	drawrect(vid_buf, 19, YRES+(MENUSIZE-16), 16, 14, c, c, c, 255);

	// the save sim button
	if (!svf_login || alternate)
	{
		c = !svf_login ? 255 : 0;
		if (!svf_login)
			drawrect(vid_buf, 37, YRES+(MENUSIZE-16), 150, 14, 255, 255, 255, 255);
		else
			fillrect(vid_buf, 36, YRES+(MENUSIZE-16)-1, 152, 16, 255, 255, 255, 255);
		drawtext(vid_buf, 40, YRES+(MENUSIZE-14), "\x82", c, c, c, 255);
		if (svf_fileopen)
			drawtext(vid_buf, 58, YRES+(MENUSIZE-12), svf_filename, c, c, c, 255);
		else
			drawtext(vid_buf, 58, YRES+(MENUSIZE-12), "[save to disk]", c, c, c, 255);
		if (svf_fileopen)
			drawdots(vid_buf, 55, YRES+(MENUSIZE-15), 12, c, c, c, 255);
	}
	else
	{
		c = svf_login ? 255 : 128;
		drawtext(vid_buf, 40, YRES+(MENUSIZE-14), "\x82", c, c, c, 255);
		if (svf_open)
			drawtextmax(vid_buf, 58, YRES+(MENUSIZE-12), 125, svf_name, c, c, c, 255);
		else
			drawtext(vid_buf, 58, YRES+(MENUSIZE-12), "[untitled simulation]", c, c, c, 255);
		drawrect(vid_buf, 37, YRES+(MENUSIZE-16), 150, 14, c, c, c, 255);
		if (svf_open && svf_own)
			drawdots(vid_buf, 55, YRES+(MENUSIZE-15), 12, c, c, c, 255);
	}

	c = (svf_login && svf_open) ? 255 : 128;

	//the vote buttons
	drawrect(vid_buf, 189, YRES+(MENUSIZE-16), 38, 14, c, c, c, 255);
	drawrect(vid_buf, 227, YRES+(MENUSIZE-16), 14, 14, c, c, c, 255);

	if (svf_myvote==1 && (svf_login && svf_open))
	{
		fillrect(vid_buf, 189, YRES+(MENUSIZE-16), 38, 14, 0, 108, 10, 255);
	}
	else if (svf_myvote==-1 && (svf_login && svf_open))
	{
		fillrect(vid_buf, 227, YRES+(MENUSIZE-16), 14, 14, 108, 10, 0, 255);
	}
	drawtext(vid_buf, 192, YRES+(MENUSIZE-12), "\xCB", 0, 187, 18, c);
	drawtext(vid_buf, 205, YRES+(MENUSIZE-12), "Vote", 0, 187, 18, c);
	drawtext(vid_buf, 229, YRES+(MENUSIZE-14), "\xCA", 187, 40, 0, c);

	c = svf_open ? 255 : 128;

	//the tags button
	drawtext(vid_buf, 246, YRES+(MENUSIZE-15), "\x83", c, c, c, 255);
	if (svf_tags[0])
		drawtextmax(vid_buf, 264, YRES+(MENUSIZE-12), XRES+BARSIZE-445, svf_tags, c, c, c, 255);
	else
		drawtext(vid_buf, 264, YRES+(MENUSIZE-12), "[no tags set]", c, c, c, 255);

	drawrect(vid_buf, 243, YRES+(MENUSIZE-16), XRES+BARSIZE-420, 14, c, c, c, 255);

	//Report bugs
	drawtext(vid_buf, XRES+BARSIZE-173, YRES+(MENUSIZE-14), "\xE7", 255, 255, 255, 255);
	drawrect(vid_buf, XRES+BARSIZE-175, YRES+(MENUSIZE-16), 14, 14, 255, 255, 255, 255);

	//the clear sim button------------some of the commented values are wrong
	drawtext(vid_buf, XRES-139+BARSIZE/*371*/, YRES+(MENUSIZE-14), "\x92", 255, 255, 255, 255);
	drawrect(vid_buf, XRES-143+BARSIZE/*367*/, YRES+(MENUSIZE-16), 16, 14, 255, 255, 255, 255);

	//the login button
	drawtext(vid_buf, XRES-122+BARSIZE/*388*/, YRES+(MENUSIZE-13), "\x84", 255, 255, 255, 255);
	if (svf_login)
	{
		drawtextmax(vid_buf, XRES-104+BARSIZE/*406*/, YRES+(MENUSIZE-12), 66, svf_user, 255, 255, 255, 255);
		drawdots(vid_buf, XRES+BARSIZE-108, YRES+(MENUSIZE-15), 12, c, c, c, 255);
	}
	else
		drawtext(vid_buf, XRES-104+BARSIZE/*406*/, YRES+(MENUSIZE-12), "[sign in]", 255, 255, 255, 255);
	drawrect(vid_buf, XRES-125+BARSIZE/*385*/, YRES+(MENUSIZE-16), 91, 14, 255, 255, 255, 255);

	//the pause button
	if (sys_pause)
	{
		fillrect(vid_buf, XRES-17+BARSIZE/*493*/, YRES+(MENUSIZE-17), 16, 16, 255, 255, 255, 255);
		drawtext(vid_buf, XRES-14+BARSIZE/*496*/, YRES+(MENUSIZE-14), "\x90", 0, 0, 0, 255);
	}
	else
	{
		drawtext(vid_buf, XRES-14+BARSIZE/*496*/, YRES+(MENUSIZE-14), "\x90", 255, 255, 255, 255);
		drawrect(vid_buf, XRES-16+BARSIZE/*494*/, YRES+(MENUSIZE-16), 14, 14, 255, 255, 255, 255);
	}

	//The simulation options button, used to be the heat sim button
	{
		drawtext(vid_buf, XRES-156+BARSIZE/*481*/, YRES+(MENUSIZE-13), "\xCF", 255, 255, 255, 255);
		drawrect(vid_buf, XRES-159+BARSIZE/*494*/, YRES+(MENUSIZE-16), 14, 14, 255, 255, 255, 255);
	}

	//the view mode button
	addchar(vid_buf, XRES-29+BARSIZE, YRES+(MENUSIZE-13), 0xD8, 255, 0, 0, 255);
	addchar(vid_buf, XRES-29+BARSIZE, YRES+(MENUSIZE-13), 0xD9, 0, 255, 0, 255);
	addchar(vid_buf, XRES-29+BARSIZE, YRES+(MENUSIZE-13), 0xDA, 0, 0, 255, 255);
	drawrect(vid_buf, XRES-32+BARSIZE/*478*/, YRES+(MENUSIZE-16), 14, 14, 255, 255, 255, 255);

	// special icons for admin/mods
	if (svf_admin)
	{
		drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\xC9", 232, 127, 35, 255);
		drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\xC7", 255, 255, 255, 255);
		drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\xC8", 255, 255, 255, 255);
	}
	else if (svf_mod)
	{
		drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\xC9", 35, 127, 232, 255);
		drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\xC7", 255, 255, 255, 255);
	}
	//else if (amd)
	//	drawtext(vid_buf, XRES-45+BARSIZE/*463*/, YRES+(MENUSIZE-14), "\x97", 0, 230, 153, 255); //Why is this here?
}

void error_ui(pixel *vid_buf, int err, const char *txt)
{
	int x0=(XRES-240)/2,y0=YRES/2,b=1,bq,mx,my,textheight;
	char *msg;

	msg = (char*)malloc(strlen(txt)+16);
	if (err)
		sprintf(msg, "%03d %s", err, txt);
	else
		sprintf(msg, "%s", txt);
	textheight = textwrapheight(msg, 240);
	y0 -= (52+textheight)/2;
	if (y0<2)
		y0 = 2;
	if (y0+50+textheight>YRES)
		textheight = YRES-50-y0;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, 243, 51+textheight);
		drawrect(vid_buf, x0, y0, 240, 48+textheight, 192, 192, 192, 255);
		if (err)
			drawtext(vid_buf, x0+8, y0+8, "HTTP error:", 255, 64, 32, 255);
		else
			drawtext(vid_buf, x0+8, y0+8, "Error:", 255, 64, 32, 255);
		drawtextwrap(vid_buf, x0+8, y0+26, 224, 0, msg, 255, 255, 255, 255);
		drawtext(vid_buf, x0+5, y0+textheight+37, "Dismiss", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+textheight+32, 240, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+240 && my>=y0+textheight+32 && my<=y0+textheight+48)
				break;
			//else if (mx < x0 || my < y0 || mx > x0+240 || my > y0+textheight+48)
			//	break;
		}

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	free(msg);

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}

typedef struct int_pair
{
	int first, second;
} int_pair;

int int_pair_cmp (const void * a, const void * b)
{
	int_pair *ap = (int_pair*)a;
	int_pair *bp = (int_pair*)b;
	return ( ap->first - bp->first );
}

#include "simulation/Simulation.h"
void element_search_ui(pixel *vid_buf, Tool ** selectedLeft, Tool ** selectedRight)
{
	int windowHeight = 300, windowWidth = 240;
	int x0 = (XRES-windowWidth)/2, y0 = (YRES-windowHeight)/2, b = 1, bq, mx, my;
	int toolx = 0, tooly = 0, i, xoff, yoff, c, found;
	char tempCompare[512];
	char tempString[512];
	int_pair tempInts[PT_NUM];
	int selectedl = -1;
	int selectedr = -1;
	int firstResult = -1, hover = -1;

	ui_edit ed;
	ui_edit_init(&ed, x0+12, y0+30, windowWidth - 20, 14);
	ed.def = "[element name]";


	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, windowWidth+3, windowHeight+3);
		drawrect(vid_buf, x0, y0, windowWidth, windowHeight, 192, 192, 192, 255);
		
		drawtext(vid_buf, x0+8, y0+8, "\xE6 Element Search", 255, 255, 255, 255);

		drawrect(vid_buf, ed.x-4, ed.y-5, ed.w+4, 16, 192, 192, 192, 255);
		ui_edit_draw(vid_buf, &ed);
		ui_edit_process(mx, my, b, bq, &ed);
		
		drawrect(vid_buf, ed.x-4, (ed.y-5)+20, ed.w+4, windowHeight-((ed.y-5)), 192, 192, 192, 255);
		xoff = (ed.x-4)+6;
		yoff = ((ed.y-5)+20)+4;
		toolx = 0;
		tooly = 0;
		
		drawtext(vid_buf, xoff+toolx+4, yoff+tooly+3, "Matches:", 255, 255, 255, 180);
		draw_line(vid_buf, xoff+toolx+2, yoff+tooly+14, xoff+toolx+5+(ed.w-16), yoff+tooly+14, 180, 180, 180, XRES+BARSIZE);
		tooly += 17;
		
		//Covert input to lower case
		c = 0;
		while (ed.str[c]) { tempString[c] = tolower(ed.str[c]); c++; } tempString[c] = 0;
		
		firstResult = -1;
		hover = -1;
		for(i = 0; i < PT_NUM; i++)
		{
			c = 0;
			while (ptypes[i].name[c]) { tempCompare[c] = tolower(ptypes[i].name[c]); c++; } tempCompare[c] = 0;
			if(strstr(tempCompare, tempString)!=0 && ptypes[i].enabled)
			{
				Tool* foundTool = GetToolFromIdentifier(globalSim->elements[i].Identifier);
				if (!foundTool)
					continue;
				if(firstResult==-1)
					firstResult = i;
				toolx += draw_tool_xy(vid_buf, toolx+xoff, tooly+yoff, foundTool)+5;
				if (!bq && mx>=xoff+toolx-32 && mx<xoff+toolx && my>=yoff+tooly && my<yoff+tooly+15)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 255, 55, 55, 255);
					hover = i;
				}
				else if (i == selectedl || foundTool == *selectedLeft)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 255, 55, 55, 255);
				}
				else if (i==selectedr || foundTool == *selectedRight)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 55, 55, 255, 255);
				}
				if(toolx > ed.w-4)
				{
					tooly += 18;
					toolx = 0;
				}
				if(tooly>windowHeight-((ed.y-5)+20))
					break;;
			}
		}
		
		if(toolx>0)
		{
			toolx = 0;
			tooly += 18;
		}
		
		if(tooly<windowHeight-((ed.y-5)+20))
		{
			drawtext(vid_buf, xoff+toolx+4, yoff+tooly+3, "Related:", 255, 255, 255, 180);
			draw_line(vid_buf, xoff+toolx+2, yoff+tooly+14, xoff+toolx+5+(ed.w-16), yoff+tooly+14, 180, 180, 180, XRES+BARSIZE);
			tooly += 17;
			
			found = 0;
			for(i = 0; i < PT_NUM; i++)
			{
				c = 0;
				while (ptypes[i].descs[c]) { tempCompare[c] = tolower(ptypes[i].descs[c]); c++; } tempCompare[c] = 0;
				if(strstr(tempCompare, tempString)!=0 && ptypes[i].enabled)
				{
					tempInts[found].first = (strstr(tempCompare, tempString)==NULL)?0:1;
					tempInts[found++].second = i;
				}
			}
			
			qsort(tempInts, found, sizeof(int_pair), int_pair_cmp);
			
			for(i = 0; i < found; i++)
			{
				Tool* foundTool = GetToolFromIdentifier(globalSim->elements[tempInts[i].second].Identifier);
				if (!foundTool)
					continue;
				if(firstResult==-1)
					firstResult = tempInts[i].second;
				toolx += draw_tool_xy(vid_buf, toolx+xoff, tooly+yoff, foundTool)+5;
				if (!bq && mx>=xoff+toolx-32 && mx<xoff+toolx && my>=yoff+tooly && my<yoff+tooly+15)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 255, 55, 55, 255);
					hover = tempInts[i].second;
				}
				else if (tempInts[i].second == selectedl || foundTool == *selectedLeft)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 255, 55, 55, 255);
				}
				else if (tempInts[i].second == selectedr || foundTool == *selectedRight)
				{
					drawrect(vid_buf, xoff+toolx-32, yoff+tooly-1, 29, 17, 55, 55, 255, 255);
				}
				if(toolx > ed.w-4)
				{
					tooly += 18;
					toolx = 0;
				}
				if(tooly>windowHeight-((ed.y-5)+18))
					break;
			}
		}
		
		if(b==1 && hover!=-1)
		{
			selectedl = hover;
			break;
		}
		if(b==4 && hover!=-1)
		{
			selectedr = hover;
			break;
		}
		
		drawtext(vid_buf, x0+5, y0+windowHeight-12, "Dismiss", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+windowHeight-16, windowWidth, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+windowWidth && my>=y0+windowHeight-16 && my<=y0+windowHeight)
				break;
			else if (mx < x0 || mx > x0+windowWidth || my < y0 || my > y0+windowHeight)
				break;
		}

		if (sdl_key==SDLK_RETURN)
		{
			if(selectedl==-1)
				selectedl = firstResult;
			break;
		}
		if (sdl_key==SDLK_ESCAPE)
		{
			selectedl = -1;
			selectedr = -1;
			break;
		}
	}

	if(selectedl!=-1)
		*selectedLeft = GetToolFromIdentifier(globalSim->elements[selectedl].Identifier);
	if(selectedr!=-1)
		*selectedRight = GetToolFromIdentifier(globalSim->elements[selectedr].Identifier);
	
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}

char *input_ui(pixel *vid_buf, char *title, char *prompt, char *text, char *shadow)
{
	int xsize = 244;
	int ysize = 90;
	int x0=(XRES-xsize)/2,y0=(YRES-MENUSIZE-ysize)/2,b=1,bq,mx,my;

	ui_edit ed;
	ui_edit_init(&ed, x0+12, y0+50, xsize-20, 14);
	ed.def = shadow;
	ed.focus = 0;
	strncpy(ed.str, text, 254);

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, xsize+3, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, title, 160, 160, 255, 255);
		drawtext(vid_buf, x0+8, y0+26, prompt, 255, 255, 255, 255);
		
		drawrect(vid_buf, ed.x-4, ed.y-5, ed.w+4, 16, 192, 192, 192, 255);

		ui_edit_draw(vid_buf, &ed);
		ui_edit_process(mx, my, b, bq, &ed);

		drawtext(vid_buf, x0+5, y0+ysize-11, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+ysize-16, xsize, 16, 192, 192, 192, 255);

#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+xsize && my>=y0+ysize-16 && my<=y0+ysize)
				break;
			//else if (mx < x0 || my < y0 || mx > x0+xsize || my > y0+ysize)
			//	break;
		}

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	return mystrdup(ed.str);
}

int propSelected = 0;
char propValue[255] = "";
void prop_edit_ui(pixel *vid_buf)
{
	pixel * o_vid_buf;
	int format, propoffset = -1;
	char *listitems[] = {"type", "life", "ctype", "temp", "tmp", "tmp2", "vy", "vx", "x", "y", "dcolour", "flags", "pavg0", "pavg1"};
	int listitemscount = 14;
	int xsize = 244;
	int ysize = 87;
	int x0=(XRES-xsize)/2,y0=(YRES-MENUSIZE-ysize)/2,b=1,bq,mx,my;
	ui_list ed;
	ui_edit ed2;
	PropTool* propTool = (PropTool*)GetToolFromIdentifier("DEFAULT_UI_PROPERTY");

	ed.x = x0+8;
	ed.y = y0+25;
	ed.w = xsize - 16;
	ed.h = 16;
	ed.def = "[property]";
	ed.selected = propSelected;
	ed.items = listitems;
	ed.count = listitemscount;
	strncpy(ed.str, listitems[propSelected], 254);
	
	ui_edit_init(&ed2, x0+12, y0+50, xsize-20, 14);
	ed2.def = "[value]";
	ed2.focus = 1;
	strncpy(ed2.str, propValue, 254);
	ed2.cursorstart = 0;
	ed2.cursor = strlen(propValue);

	o_vid_buf = (pixel*)calloc((YRES+MENUSIZE) * (XRES+BARSIZE), PIXELSIZE);
	if (o_vid_buf)
		memcpy(o_vid_buf, vid_buf, ((YRES+MENUSIZE) * (XRES+BARSIZE)) * PIXELSIZE);
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		if (o_vid_buf)
			memcpy(vid_buf, o_vid_buf, ((YRES+MENUSIZE) * (XRES+BARSIZE)) * PIXELSIZE);
		clearrect(vid_buf, x0-1, y0-1, xsize+4, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, "Change particle property", 160, 160, 255, 255);
		//drawtext(vid_buf, x0+8, y0+26, prompt, 255, 255, 255, 255);
		
		//drawrect(vid_buf, ed.x-4, ed.y-5, ed.w+4, 16, 192, 192, 192, 255);
		drawrect(vid_buf, ed2.x-4, ed2.y-5, ed2.w+4, 16, 192, 192, 192, 255);

		ui_list_draw(vid_buf, &ed);
		ui_list_process(vid_buf, mx, my, b, &ed);
		ui_edit_draw(vid_buf, &ed2);
		ui_edit_process(mx, my, b, bq, &ed2);

		drawtext(vid_buf, x0+5, y0+ysize-11, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+ysize-16, xsize, 16, 192, 192, 192, 255);

#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+xsize && my>=y0+ysize-16 && my<=y0+ysize)
				break;
			else if (mx < x0 || my < y0 || mx > x0+xsize || my > y0+ysize)
				goto exit;
		}

		if (sdl_key == SDLK_RETURN)
			break;
		else if (sdl_key == SDLK_ESCAPE)
			goto exit;
		else if (sdl_key == SDLK_UP && ed.selected > 0)
		{
			ed.selected--;
			strcpy(ed.str, ed.items[ed.selected]);
		}
		else if (sdl_key == SDLK_DOWN && ed.selected < ed.count-1)
		{
			ed.selected++;
			strcpy(ed.str, ed.items[ed.selected]);
		}
	}

	if (ed.selected != -1)
	{
		propoffset = Particle_GetOffset(ed.str, &format);
	}
	if (propoffset == -1)
	{
		error_ui(vid_buf, 0, "Invalid property");
		goto exit;
	}
	
	propTool->propOffset = (size_t)propoffset;
	if (format == 2 || !strcmp(ed.str, "ctype")) //ctype is special here
	{
		unsigned int value;
		int isint = 1;
		//Check if it's an element name
		for (unsigned int i = 0; i < strlen(ed2.str); i++)
		{
			if (!((ed2.str[i] >= '0' && ed2.str[i] <= '9') || (i == 0 && ed2.str[0] == '-')))
			{
				isint = 0;
				break;
			}
		}
		if (isint)
		{
			sscanf(ed2.str, "%u", &value);
		}
		else
		{
			if (!console_parse_type(ed2.str, (int*)(&value), NULL))
			{
				error_ui(vid_buf, 0, "Invalid element name");
				goto exit;
			}
		}
		if (format == 2 && (value < 0 || value > PT_NUM || !ptypes[value].enabled))
		{
			error_ui(vid_buf, 0, "Invalid element number");
			goto exit;
		}
		propTool->propValue.UInteger = value;
		propTool->propType = UInteger;
	}
	else if (format == 0)
	{
		int value;
		sscanf(ed2.str, "%d", &value);
		propTool->propValue.Integer = value;
		propTool->propType = Integer;
	}
	else if (format == 1)
	{
		float value;
		sscanf(ed2.str, "%f", &value);
		propTool->propValue.Float = value;
		propTool->propType = Float;
	}
	else if (format == 3)
	{
		unsigned int value;
		if (ed2.str[0] == '#') // #FFFFFFFF
		{
			//Convert to lower case
			for (unsigned int j = 0; j < strlen(ed2.str); j++)
				ed2.str[j] = tolower(ed2.str[j]);
			sscanf(ed2.str, "#%x", &value);
		}
		else if (ed2.str[0] == '0' && ed2.str[1] == 'x') // 0xFFFFFFFF
		{
			//Convert to lower case
			for (unsigned int j = 0; j < strlen(ed2.str); j++)
				ed2.str[j] = tolower(ed2.str[j]);
			sscanf(ed2.str, "0x%x", &value);
		}
		else
		{
			sscanf(ed2.str, "%u", &value);
		}
		propTool->propValue.UInteger = value;
		propTool->propType = UInteger;
	}
	propSelected = ed.selected;
	strncpy(propValue, ed2.str, 254);

exit:
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}

void info_ui(pixel *vid_buf, char *top, char *txt)
{
	int x0=(XRES-240)/2,y0=(YRES-MENUSIZE)/2,b=1,bq,mx,my;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, 243, 63);
		drawrect(vid_buf, x0, y0, 240, 60, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, top, 160, 160, 255, 255);
		drawtext(vid_buf, x0+8, y0+26, txt, 255, 255, 255, 255);
		drawtext(vid_buf, x0+5, y0+49, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+44, 240, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq && mx>=x0 && mx<x0+240 && my>=y0+44 && my<=y0+60)
			break;

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}

void info_box(pixel *vid_buf, char *msg)
{
	int w = textwidth(msg)+16;
	int x0=(XRES-w)/2,y0=(YRES-24)/2;
	
	clearrect(vid_buf, x0-1, y0-1, w+3, 27);
	drawrect(vid_buf, x0, y0, w, 24, 192, 192, 192, 255);
	drawtext(vid_buf, x0+8, y0+8, msg, 192, 192, 240, 255);
#ifndef RENDERER
#ifdef OGLR
	clearScreen(1.0f);
#endif
	sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
#endif
}

void info_box_overlay(pixel *vid_buf, char *msg)
{
	int w = textwidth(msg)+16;
	int x0=(XRES-w)/2,y0=(YRES-24)/2;
	
	clearrect(vid_buf, x0-1, y0-1, w+3, 27);
	drawrect(vid_buf, x0, y0, w, 24, 192, 192, 192, 255);
	drawtext(vid_buf, x0+8, y0+8, msg, 192, 192, 240, 255);
}

void copytext_ui(pixel *vid_buf, char *top, char *txt, char *copytxt)
{
	int xsize = 244;
	int ysize = 90;
	int x0=(XRES-xsize)/2,y0=(YRES-MENUSIZE-ysize)/2,b=1,bq,mx,my;
	int buttonx = 0;
	int buttony = 0;
	int buttonwidth = 0;
	int buttonheight = 0;
	ui_copytext ed;

	buttonwidth = textwidth(copytxt)+12;
	buttonheight = 10+8;
	buttony = y0+50;
	buttonx = x0+(xsize/2)-(buttonwidth/2);

	ed.x = buttonx;
	ed.y = buttony;
	ed.width = buttonwidth;
	ed.height = buttonheight;
	ed.hover = 0;
	ed.state = 0;
	strcpy(ed.text, copytxt);

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, xsize+3, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, top, 160, 160, 255, 255);
		drawtext(vid_buf, x0+8, y0+26, txt, 255, 255, 255, 255);

		ui_copytext_draw(vid_buf, &ed);
		ui_copytext_process(mx, my, b, bq, &ed);

		drawtext(vid_buf, x0+5, y0+ysize-11, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+ysize-16, xsize, 16, 192, 192, 192, 255);

#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq && mx>=x0 && mx<x0+xsize && my>=y0+ysize-16 && my<=y0+ysize)
			break;

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}
bool confirm_ui(pixel *vid_buf, char *top, char *msg, char *btn)
{
	int x0=(XRES-240)/2,y0=YRES/2,b=1,bq,mx,my,textheight;
	bool ret = false;

	textheight = textwrapheight(msg, 224);
	y0 -= (52+textheight)/2;
	if (y0<2)
		y0 = 2;
	if (y0+50+textheight>YRES)
		textheight = YRES-50-y0;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, 243, 51+textheight);
		drawrect(vid_buf, x0, y0, 240, 48+textheight, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, top, 255, 216, 32, 255);
		drawtextwrap(vid_buf, x0+8, y0+26, 224, 0, msg, 255, 255, 255, 255);
		drawtext(vid_buf, x0+5, y0+textheight+37, "Cancel", 255, 255, 255, 255);
		drawtext(vid_buf, x0+165, y0+textheight+37, btn, 255, 216, 32, 255);
		drawrect(vid_buf, x0, y0+textheight+32, 160, 16, 192, 192, 192, 255);
		drawrect(vid_buf, x0+160, y0+textheight+32, 80, 16, 192, 192, 192, 255);
		
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b && !bq && mx>=x0+160 && mx<x0+240 && my>=y0+textheight+32 && my<=y0+textheight+48)
		{
			ret = true;
			break;
		}
		
		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+160 && my>=y0+textheight+32 && my<=y0+textheight+48)
				break;
			//else if (mx < x0 || mx > x0+160 || my < y0 || my > y0+textheight+48)
			//	break;
		}

		if (sdl_key==SDLK_RETURN)
		{
			ret = true;
			break;
		}
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	
	return ret;
}

void login_ui(pixel *vid_buf)
{
	int x0=(XRES+BARSIZE-192)/2,y0=(YRES+MENUSIZE-80)/2,b=1,bq,mx,my;
	ui_edit ed1,ed2;
	char passwordHash[33];
	char totalHash[33];
	char hashStream[99]; //not really a stream ...

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	ui_edit_init(&ed1, x0+25, y0+25, 158, 14);
	ed1.def = "[user name]";
	ed1.cursor = ed1.cursorstart = strlen(svf_user);
	strcpy(ed1.str, svf_user);
	if (ed1.cursor)
		ed1.focus = 0;

	ui_edit_init(&ed2, x0+25, y0+45, 158, 14);
	ed2.def = "[password]";
	ed2.hide = 1;
	if (!ed1.cursor)
		ed2.focus = 0;

	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		drawrect(vid_buf, x0, y0, 192, 80, 192, 192, 192, 255);
		clearrect(vid_buf, x0+1, y0+1, 191, 79);
		drawtext(vid_buf, x0+8, y0+8, "Server login:", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+23, "\x8B", 32, 64, 128, 255);
		drawtext(vid_buf, x0+12, y0+23, "\x8A", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+20, 176, 16, 192, 192, 192, 255);
		drawtext(vid_buf, x0+11, y0+44, "\x8C", 160, 144, 32, 255);
		drawtext(vid_buf, x0+11, y0+44, "\x84", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+40, 176, 16, 192, 192, 192, 255);
		ui_edit_draw(vid_buf, &ed1);
		ui_edit_draw(vid_buf, &ed2);
		drawtext(vid_buf, x0+5, y0+69, "Sign out", 255, 255, 255, 255);
		drawtext(vid_buf, x0+187-textwidth("Sign in"), y0+69, "Sign in", 255, 255, 55, 255);
		drawrect(vid_buf, x0, y0+64, 192, 16, 192, 192, 192, 255);
		drawrect(vid_buf, x0, y0+64, 96, 16, 192, 192, 192, 255);
		
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		ui_edit_process(mx, my, b, bq, &ed1);
		ui_edit_process(mx, my, b, bq, &ed2);

		if (b && !bq)
		{
			if (mx>=x0+9 && mx<x0+23 && my>=y0+22 && my<y0+36)
				break;
			if (mx>=x0+9 && mx<x0+23 && my>=y0+42 && my<y0+46)
				break;
			if (mx>=x0 && mx<x0+96 && my>=y0+64 && my<=y0+80)
				goto fail;
			if (mx>=x0+97 && mx<x0+192 && my>=y0+64 && my<=y0+80)
				break;
			if (mx < x0 || my < y0 || mx > x0+192 || my > y0+80)
				return;
		}

		if (sdl_key==SDLK_RETURN || sdl_key==SDLK_TAB)
		{
			if (!ed1.focus)
				break;
			ed1.focus = 0;
			ed2.focus = 1;
		}
		if (sdl_key==SDLK_ESCAPE)
		{
			if (!ed1.focus && !ed2.focus)
				return;
			ed1.focus = 0;
			ed2.focus = 0;
		}
	}

	strcpy(svf_user, ed1.str);
	strcpy(svf_pass, ed2.str);
	//md5_ascii(svf_pass, (unsigned char *)ed2.str, 0);

	md5_ascii(passwordHash, (unsigned char *)svf_pass, strlen(svf_pass));
	passwordHash[32] = 0;
	sprintf(hashStream, "%s-%s", svf_user, passwordHash);
	//hashStream << username << "-" << passwordHash;
	md5_ascii(totalHash, (const unsigned char *)hashStream, strlen(hashStream));
	totalHash[32] = 0;
	if (totalHash)
	{
		int dataStatus, dataLength;
		const char *const postNames[] = { "Username", "Hash", NULL };
		const char *const postDatas[] = { svf_user, totalHash };
		size_t postLengths[] = { strlen(svf_user), 32 };
		char * data;
		data = http_multipart_post(
				  "http://" SERVER "/Login.json",
				  postNames, postDatas, postLengths,
				  NULL, NULL, NULL,
				  &dataStatus, &dataLength);
		if(dataStatus == 200 && data)
		{
			cJSON *root, *tmpobj;//, *notificationarray, *notificationobj;
			if (root = cJSON_Parse((const char*)data))
			{
				tmpobj = cJSON_GetObjectItem(root, "Status");
				if (tmpobj && tmpobj->type == cJSON_Number && tmpobj->valueint == 1)
				{
					if((tmpobj = cJSON_GetObjectItem(root, "UserID")) && tmpobj->type == cJSON_Number)
						sprintf(svf_user_id, "%i", tmpobj->valueint);
					if((tmpobj = cJSON_GetObjectItem(root, "SessionID")) && tmpobj->type == cJSON_String)
						strcpy(svf_session_id, tmpobj->valuestring);
					if((tmpobj = cJSON_GetObjectItem(root, "SessionKey")) && tmpobj->type == cJSON_String)
						strcpy(svf_session_key, tmpobj->valuestring);
					if((tmpobj = cJSON_GetObjectItem(root, "Elevation")) && tmpobj->type == cJSON_String)
					{
						char * elevation = tmpobj->valuestring;
						if (!strcmp(elevation, "Mod"))
						{
							svf_admin = 0;
							svf_mod = 1;
						}
						else if (!strcmp(elevation, "Admin"))
						{
							svf_admin = 1;
							svf_mod = 0;
						}
						else
						{
							svf_admin = 0;
							svf_mod = 0;
						}
					}
					/*notificationarray = cJSON_GetObjectItem(root, "Notifications");
					notificationobj = cJSON_GetArrayItem(notificationarray, 0);
					while (notificationobj)
					{
						i++;
						if((tmpobj = cJSON_GetObjectItem(notificationarray, "Text")) && tmpobj->type == cJSON_String)
							if (strstr(tmpobj->valuestring, "message"))
								svf_messages++;
						notificationobj = cJSON_GetArrayItem(notificationarray, i);
					}*/

					svf_login = 1;
					save_presets(0);
				}
				else
				{
					tmpobj = cJSON_GetObjectItem(root, "Error");
					if (tmpobj && tmpobj->type == cJSON_String)
					{
						char * banstring = strstr(tmpobj->valuestring, ". Ban expire in");
						if (banstring) //TODO: temporary, remove this when the ban message is fixed
						{
							char banreason[256] = "Account banned. Login at http://powdertoy.co.uk in order to see the full ban reason. Ban expires";
							strappend(banreason, strstr(tmpobj->valuestring, " in"));
							error_ui(vid_buf, 0, banreason);
						}
						else
							error_ui(vid_buf, 0, tmpobj->valuestring);
					}
					else
						error_ui(vid_buf, 0, "Could not read Error response");
					if (data)
						free(data);
					goto fail;
				}
			}
			else
				error_ui(vid_buf, 0, "Could not read response");
		}
		else
			error_ui(vid_buf, dataStatus, http_ret_text(dataStatus));
		if (data)
			free(data);
		return;
	}

fail:
	strcpy(svf_user, "");
	strcpy(svf_pass, "");
	strcpy(svf_user_id, "");
	strcpy(svf_session_id, "");
	svf_login = 0;
	svf_own = 0;
	svf_admin = 0;
	svf_mod = 0;
	svf_messages = 0;
	save_presets(0);
}

int stamp_ui(pixel *vid_buf, int *reorder)
{
	int b=1,bq,mx,my,d=-1,i,j,k,x,gx,gy,y,w,h,r=-1,stamp_page=0,per_page=GRID_X*GRID_Y,page_count,numdelete=0,lastdelete;
	char page_info[64];
	// stamp_count-1 to avoid an extra page when there are per_page stamps on each page
	page_count = (stamp_count-1)/per_page+1;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, 0, 0, XRES+BARSIZE, YRES+MENUSIZE);
		k = stamp_page*per_page;//0;
		r = -1;
		d = -1;
		if (!stamps[k].name[0])
			drawtext(vid_buf, (XRES-textwidth("Use 's' to save stamps"))/2, YRES/2-6, "Use 's' to save stamps", 255, 255, 255, 255);
		for (j=0; j<GRID_Y; j++)
			for (i=0; i<GRID_X; i++)
			{
				if (stamps[k].name[0])
				{
					gx = ((XRES/GRID_X)*i) + (XRES/GRID_X-XRES/GRID_S)/2;
					gy = ((((YRES-MENUSIZE+20)+15)/GRID_Y)*j) + ((YRES-MENUSIZE+20)/GRID_Y-(YRES-MENUSIZE+20)/GRID_S+10)/2 + 18;
					x = (XRES*i)/GRID_X + XRES/(GRID_X*2);
					y = (YRES*j)/GRID_Y + YRES/(GRID_Y*2);
					gy -= 20;
					w = stamps[k].thumb_w;
					h = stamps[k].thumb_h;
					x -= w/2;
					y -= h/2;
					if (stamps[k].thumb)
					{
						draw_image(vid_buf, stamps[k].thumb, gx+(((XRES/GRID_S)/2)-(w/2)), gy+(((YRES/GRID_S)/2)-(h/2)), w, h, 255);
						xor_rect(vid_buf, gx+(((XRES/GRID_S)/2)-(w/2)), gy+(((YRES/GRID_S)/2)-(h/2)), w, h);
					}
					else
					{
						drawtext(vid_buf, gx+8, gy+((YRES/GRID_S)/2)-4, "Error loading stamp", 255, 255, 255, 255);
					}
					if ((mx>=gx+XRES/GRID_S-4 && mx<(gx+XRES/GRID_S)+6 && my>=gy-6 && my<gy+4) || stamps[k].dodelete)
					{
						if (mx>=gx+XRES/GRID_S-4 && mx<(gx+XRES/GRID_S)+6 && my>=gy-6 && my<gy+4)
							d = k;
						drawrect(vid_buf, gx-2, gy-2, XRES/GRID_S+3, YRES/GRID_S+3, 128, 128, 128, 255);
						drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x86", 255, 48, 32, 255);
					}
					else
					{
						if (mx>=gx && mx<gx+(XRES/GRID_S) && my>=gy && my<gy+(YRES/GRID_S) && stamps[k].thumb)
						{
							r = k;
							drawrect(vid_buf, gx-2, gy-2, XRES/GRID_S+3, YRES/GRID_S+3, 128, 128, 210, 255);
						}
						else
						{
							drawrect(vid_buf, gx-2, gy-2, XRES/GRID_S+3, YRES/GRID_S+3, 128, 128, 128, 255);
						}
						drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x86", 150, 48, 32, 255);
					}
					drawtext(vid_buf, gx+XRES/(GRID_S*2)-textwidth(stamps[k].name)/2, gy+YRES/GRID_S+7, stamps[k].name, 192, 192, 192, 255);
					drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x85", 255, 255, 255, 255);
				}
				k++;
			}

		if (numdelete)
		{
			drawrect(vid_buf,(XRES/2)-19,YRES+MENUSIZE-18,37,16,255,255,255,255);
			drawtext(vid_buf, (XRES/2)-14, YRES+MENUSIZE-14, "Delete", 255, 255, 255, 255);
			if (b == 1 && bq == 0 && mx > (XRES/2)-20 && mx < (XRES/2)+19 && my > YRES+MENUSIZE-19 && my < YRES+MENUSIZE-1)
			{
				sprintf(page_info, "%d stamp%s", numdelete, (numdelete == 1)?"":"s");
				if (confirm_ui(vid_buf, "Do you want to delete?", page_info, "Delete"))
					del_stamp(lastdelete);
				for (i=0; i<STAMP_MAX; i++)
					stamps[i].dodelete = 0;
				numdelete = 0;
				d = lastdelete = -1;
			}
		}
		else
		{
			sprintf(page_info, "Page %d of %d", stamp_page+1, page_count);

			drawtext(vid_buf, (XRES/2)-(textwidth(page_info)/2), YRES+MENUSIZE-14, page_info, 255, 255, 255, 255);
		}

		if (stamp_page)
		{
			drawtext(vid_buf, 4, YRES+MENUSIZE-14, "\x96", 255, 255, 255, 255);
			drawrect(vid_buf, 1, YRES+MENUSIZE-18, 16, 16, 255, 255, 255, 255);
		}
		if (stamp_page<page_count-1)
		{
			drawtext(vid_buf, XRES-15, YRES+MENUSIZE-14, "\x95", 255, 255, 255, 255);
			drawrect(vid_buf, XRES-18, YRES+MENUSIZE-18, 16, 16, 255, 255, 255, 255);
		}
		drawtext(vid_buf, XRES-60, YRES+MENUSIZE-14, "Rescan", 255, 255, 255, 255);
		drawrect(vid_buf, XRES-65, YRES+MENUSIZE-18, 40, 16, 255, 255, 255, 255);

		if (b==1&&bq==0&&d!=-1)
		{
			if (sdl_mod & (KMOD_CTRL|KMOD_META))
			{
				if (!stamps[d].dodelete)
				{
					stamps[d].dodelete = 1;
					numdelete++;
					lastdelete=d;
				}
				else
				{
					stamps[d].dodelete = 0;
					numdelete--;
				}
			}
			else if (!numdelete && confirm_ui(vid_buf, "Do you want to delete?", stamps[d].name, "Delete"))
			{
				del_stamp(d);
			}
		}

#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		if (b==1&&r!=-1)
			break;
		if (b==4&&r!=-1)
		{
			r = -1;
			break;
		}

		if ((b && !bq && mx>=1 && mx<=17 && my>=YRES+MENUSIZE-18 && my<YRES+MENUSIZE-2) || sdl_wheel>0)
		{
			if (stamp_page)
			{
				stamp_page --;
			}
			sdl_wheel = 0;
		}
		if ((b && !bq && mx>=XRES-18 && mx<=XRES-1 && my>=YRES+MENUSIZE-18 && my<YRES+MENUSIZE-2) || sdl_wheel<0)
		{
			if (stamp_page<page_count-1)
			{
				stamp_page ++;
			}
			sdl_wheel = 0;
		}
		if (b && !bq && mx >= XRES-65 && mx <= XRES-25 && my >= YRES+MENUSIZE-18 && my < YRES+MENUSIZE-2 && confirm_ui(vid_buf, "Rescan stamps?", "Rescanning stamps will find all stamps in your stamps/ directory and overwrite stamps.def", "OK"))
		{
			DIR *directory;
			struct dirent * entry;
			directory = opendir("stamps");
			if (directory != NULL)
			{
				std::string stampList = "";
				while (entry = readdir(directory))
				{
					if (strstr(entry->d_name, ".stm") && strlen(entry->d_name) == 14)
						stampList.insert(0, entry->d_name, 10);
				}
				closedir(directory);

				FILE *f = fopen("stamps" PATH_SEP "stamps.def", "wb");
				if (!f)
				{
					error_ui(vid_buf, 0, "Could not open stamps.def");
				}
				else
				{
					fwrite(stampList.c_str(), stampList.length(), 1, f);
					fclose(f);

					for (int i = 0; i < STAMP_MAX; i++)
						if (stamps[i].thumb)
							free(stamps[i].thumb);
					stamp_count = 0;
					stamp_init();
					page_count = (stamp_count-1)/per_page+1;
				}
			}
		}

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
		{
			r = -1;
			break;
		}
	}
	if (sdl_mod & (KMOD_CTRL|KMOD_META))
		*reorder = 0;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	for (i=0; i<STAMP_MAX; i++)
		stamps[i].dodelete = 0;
	return r;
}

void tag_list_ui(pixel *vid_buf)
{
	int y,d,x0=(XRES-192)/2,y0=(YRES-256)/2,b=1,bq,mx,my,vp,vn;
	char *p,*q,s;
	char *tag=NULL, *op=NULL;
	struct strlist *vote=NULL,*down=NULL;

	ui_edit ed;
	ui_edit_init(&ed, x0+25, y0+221, 158, 14);
	ed.def = "[new tag]";
	ed.focus = 0;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	fillrect(vid_buf, -1, -1, XRES, YRES+MENUSIZE, 0, 0, 0, 192);
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		op = tag = NULL;

		drawrect(vid_buf, x0, y0, 192, 256, 192, 192, 192, 255);
		clearrect(vid_buf, x0+1, y0+1, 191, 255);
		drawtext(vid_buf, x0+8, y0+8, "Manage tags:    \bgTags are only to \nbe used to improve search results", 255, 255, 255, 255);
		p = svf_tags;
		s = svf_tags[0] ? ' ' : 0;
		y = 36 + y0;
		while (s)
		{
			q = strchr(p, ' ');
			if (!q)
				q = p+strlen(p);
			s = *q;
			*q = 0;
			if (svf_own || svf_admin || svf_mod)
			{
				drawtext(vid_buf, x0+20, y-1, "\x86", 160, 48, 32, 255);
				drawtext(vid_buf, x0+20, y-1, "\x85", 255, 255, 255, 255);
				d = 14;
				if (b && !bq && mx>=x0+18 && mx<x0+32 && my>=y-2 && my<y+12)
				{
					op = "delete";
					tag = mystrdup(p);
				}
			}
			else
				d = 0;
			if (svf_login)
			{
				vp = strlist_find(&vote, p);
				vn = strlist_find(&down, p);
				if ((!vp && !vn && !svf_own) || svf_admin || svf_mod)
				{
					drawtext(vid_buf, x0+d+20, y-1, "\x88", 32, 144, 32, 255);
					drawtext(vid_buf, x0+d+20, y-1, "\x87", 255, 255, 255, 255);
					if (b && !bq && mx>=x0+d+18 && mx<x0+d+32 && my>=y-2 && my<y+12)
					{
						op = "vote";
						tag = mystrdup(p);
						strlist_add(&vote, p);
					}
					drawtext(vid_buf, x0+d+34, y-1, "\x88", 144, 48, 32, 255);
					drawtext(vid_buf, x0+d+34, y-1, "\xA2", 255, 255, 255, 255);
					if (b && !bq && mx>=x0+d+32 && mx<x0+d+46 && my>=y-2 && my<y+12)
					{
						op = "down";
						tag = mystrdup(p);
						strlist_add(&down, p);
					}
				}
				if (vp)
					drawtext(vid_buf, x0+d+48+textwidth(p), y, " - voted!", 48, 192, 48, 255);
				if (vn)
					drawtext(vid_buf, x0+d+48+textwidth(p), y, " - voted.", 192, 64, 32, 255);
			}
			drawtext(vid_buf, x0+d+48, y, p, 192, 192, 192, 255);
			*q = s;
			p = q+1;
			y += 16;
		}
		
		drawtext(vid_buf, x0+11, y0+219, "\x86", 32, 144, 32, 255);
		drawtext(vid_buf, x0+11, y0+219, "\x89", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+216, 176, 16, 192, 192, 192, 255);
		ui_edit_draw(vid_buf, &ed);
		drawtext(vid_buf, x0+5, y0+245, "Close", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+240, 192, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		ui_edit_process(mx, my, b, bq, &ed);

		if (b && mx>=x0 && mx<=x0+192 && my>=y0+240 && my<y0+256)
			break;

		if (op)
		{
			d = execute_tagop(vid_buf, op, tag);
			free(tag);
			op = tag = NULL;
			if (d)
				break;
		}

		if (b && !bq && mx>=x0+9 && mx<x0+23 && my>=y0+218 && my<y0+232)
		{
			d = execute_tagop(vid_buf, "add", ed.str);
			strcpy(ed.str, "");
			ed.cursor = ed.cursorstart = 0;
			if (d)
				break;
		}

		if (!b && bq && (mx < x0 || my < y0 || mx > x0+192 || my > y0+256))
			break;

		if (sdl_key == SDLK_RETURN)
		{
			if (svf_login)
			{
				if (!ed.focus)
					break;
				d = execute_tagop(vid_buf, "add", ed.str);
				strcpy(ed.str, "");
				ed.cursor = ed.cursorstart = 0;
				if (d)
					break;
			}
			else
			{
				error_ui(vid_buf, 0, "Not Authenticated");
				break;
			}
		}
		if (sdl_key == SDLK_ESCAPE)
		{
			if (!ed.focus)
				break;
			strcpy(ed.str, "");
			ed.cursor = ed.cursorstart = 0;
			ed.focus = 0;
		}
	}
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	sdl_key = 0;

	strlist_free(&vote);
}

int save_name_ui(pixel *vid_buf)
{
	int x0=(XRES-420)/2,y0=(YRES-78-YRES/4)/2,b=1,bq,mx,my,ths,idtxtwidth,nd=0;
	bool can_publish = true;
	void *th;
	pixel *old_vid=(pixel *)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	ui_edit ed;
	ui_edit ed2;
	ui_checkbox cbPublish;
	ui_checkbox cbPaused;
	ui_copytext ctb;

	th = build_thumb(&ths, 0);
	if (check_save(2,0,0,XRES,YRES,0))
		can_publish = false;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	ui_edit_init(&ed, x0+25, y0+25, 158, 14);
	ed.def = "[simulation name]";
	ed.cursor = ed.cursorstart = strlen(svf_name);
	strcpy(ed.str, svf_name);

	ui_edit_init(&ed2, x0+13, y0+45, 170, 115);
	ed2.def = "[simulation description]";
	ed2.focus = 0;
	ed2.cursor = ed2.cursorstart = strlen(svf_description);
	ed2.multiline = 1;
	strcpy(ed2.str, svf_description);

	ctb.x = 0;
	ctb.y = YRES+MENUSIZE-20;
	ctb.width = textwidth(svf_id)+12;
	ctb.height = 10+7;
	ctb.hover = 0;
	ctb.state = 0;
	strcpy(ctb.text, svf_id);

	cbPublish.x = x0+10;
	cbPublish.y = y0+74+YRES/4;
	cbPublish.focus = 0;
	cbPublish.checked = svf_publish;

	cbPaused.x = x0+110;
	cbPaused.y = y0+74+YRES/4;
	cbPaused.focus = 0;
	cbPaused.checked = sys_pause || framerender;
	
	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	draw_rgba_image(vid_buf, (unsigned char*)save_to_server_image, 0, 0, 0.7f);
	
	memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		drawrect(vid_buf, x0, y0, 420, 110+YRES/4, 192, 192, 192, 255); // rectangle around entire thing
		clearrect(vid_buf, x0+1, y0+1, 419, 109+YRES/4);
		if (strcmp(svf_name, ed.str) || !svf_own)
			drawtext(vid_buf, x0+8, y0+8, "Upload new simulation:", 255, 255, 255, 255);
		else
			drawtext(vid_buf, x0+8, y0+8, "Modify simulation properties:", 255, 255, 255, 255);
		drawtext(vid_buf, x0+10, y0+23, "\x82", 192, 192, 192, 255);
		drawrect(vid_buf, x0+8, y0+20, 176, 16, 192, 192, 192, 255); //rectangle around title box

		drawrect(vid_buf, x0+8, y0+40, 176, 124, 192, 192, 192, 255); //rectangle around description box

		ui_edit_draw(vid_buf, &ed);
		ui_edit_draw(vid_buf, &ed2);

		drawrect(vid_buf, x0+(205-XRES/3)/2-2+205, y0+40, XRES/3+3, YRES/3+3, 128, 128, 128, 255); //rectangle around thumbnail
		render_thumb(th, ths, 0, vid_buf, x0+(205-XRES/3)/2+205, y0+42, 3);

		if (!can_publish)
		{
			drawtext(vid_buf, x0+235, y0+180, "\xE4", 255, 255, 0, 255);
			drawtext(vid_buf, x0+251, y0+182, "Warning: uses mod elements", 192, 192, 192, 255);
		}

		ui_checkbox_draw(vid_buf, &cbPublish);
		drawtext(vid_buf, cbPublish.x+24, cbPublish.y+3, "Publish?", 192, 192, 192, 255);

		ui_checkbox_draw(vid_buf, &cbPaused);
		drawtext(vid_buf, cbPaused.x+24, cbPaused.y+3, "Paused?", 192, 192, 192, 255);

		drawtext(vid_buf, x0+5, y0+99+YRES/4, "Save simulation", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+94+YRES/4, 192, 16, 192, 192, 192, 255);

		draw_line(vid_buf, x0+192, y0, x0+192, y0+110+YRES/4, 150, 150, 150, XRES+BARSIZE);

		if (svf_id[0])
		{
			//Save ID text and copybox
			idtxtwidth = textwidth("Current save ID: ");
			idtxtwidth += ctb.width;
			ctb.x = textwidth("Current save ID: ")+(XRES+BARSIZE-idtxtwidth)/2;
			drawtext(vid_buf, (XRES+BARSIZE-idtxtwidth)/2, YRES+MENUSIZE-15, "Current save ID: ", 255, 255, 255, 255);

			ui_copytext_draw(vid_buf, &ctb);
			ui_copytext_process(mx, my, b, bq, &ctb);
		}
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		memcpy(vid_buf, old_vid, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);

		ui_edit_process(mx, my, b, bq, &ed);
		ui_edit_process(mx, my, b, bq, &ed2);
		ui_checkbox_process(mx, my, b, bq, &cbPublish);
		ui_checkbox_process(mx, my, b, bq, &cbPaused);

		if ((b && !bq && mx>=x0 && mx<x0+192 && my>=y0+94+YRES/4 && my<y0+110+YRES/4) || sdl_key==SDLK_RETURN)
		{
			if (th) free(th);
			if (!ed.str[0])
				return 0;
			nd = strcmp(svf_name, ed.str) || !svf_own;
			strncpy(svf_name, ed.str, 63);
			svf_name[63] = 0;
			strncpy(svf_description, ed2.str, 254);
			strncpy(svf_author, svf_user, 63);
			svf_description[254] = 0;
			if (nd)
			{
				strcpy(svf_id, "");
				strcpy(svf_tags, "");
			}
			svf_open = 1;
			svf_own = 1;
			svf_publish = cbPublish.checked;
			sys_pause = cbPaused.checked;
			svf_filename[0] = 0;
			svf_fileopen = 0;
			free(old_vid);
			return nd+1;
		}
		if (b && !bq && (mx < x0 || my < y0 || mx > x0+420 || my > y0+110+YRES/4))
			break;
		if (sdl_key == SDLK_ESCAPE)
		{
			break;
		}
	}
	if (th) free(th);
	free(old_vid);
	return 0;
}

//calls menu_ui_v2 when needed, draws fav. menu on bottom
void old_menu_v2(int active_menu, int x, int y, int b, int bq)
{
	if (active_menu > SC_FAV)
	{
		Tool *over = menu_draw(x, y, b, bq, active_menu);
		if (over)
		{
			int height = (int)(ceil((float)menuSections[active_menu]->tools.size()/16.0f)*18);
			int textY = (((YRES/GetNumMenus())*active_menu)+((YRES/GetNumMenus())/2))-(height/2)+(FONT_H/2)+25+height;
			menu_draw_text(over, textY);
			menu_select_element(b, over);
		}
	}
	else
	{
		Tool *over = menu_draw(x, y, b, bq, SC_FAV);
		if (over)
		{
			menu_draw_text(over, YRES-9);
			menu_select_element(b, over);
		}
	}
	int numMenus = GetNumMenus();
	for (int i = 0; i < numMenus; i++)
	{
		if (i < SC_FAV)
			drawchar(vid_buf, XRES+1, /*(12*i)+2*/((YRES/numMenus)*i)+((YRES/numMenus)/2)+5, menuSections[i]->icon, 255, 255, 255, 255);
	}
	//check mouse position to see if it is on a menu section
	for (int i = 0; i < numMenus; i++)
	{
		if (!b && i < SC_FAV && x>= XRES+1 && x< XRES+BARSIZE-1 && y>= ((YRES/numMenus)*i)+((YRES/numMenus)/2)+3 && y<((YRES/numMenus)*i)+((YRES/numMenus)/2)+17)
			menu_ui_v2(vid_buf, i); //draw the elements in the current menu if mouse inside
	}
}

//old menu function, with the elements drawn on the side
void menu_ui_v2(pixel *vid_buf, int i)
{
	int b=1, bq, mx, my, y, sy, numMenus = GetNumMenus(), someStrangeYValue = (((YRES/numMenus)*i)+((YRES/numMenus)/2))+3;
	int rows = (int)ceil((float)menuSections[i]->tools.size()/16.0f);
	int height = (int)(ceil((float)menuSections[i]->tools.size()/16.0f)*18);
	int width = (int)restrict_flt(menuSections[i]->tools.size()*31.0f, 0, 16*31);
	pixel *old_vid=(pixel *)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	if (!old_vid)
		return;
	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
	active_menu = i;
	sy = y = (((YRES/numMenus)*i)+((YRES/numMenus)/2))-(height/2)+(FONT_H/2)+6;
	//wipe out existing toolTips, to prevent weirdness inside here
	for (int j = toolTips.size()-1; j >= 0; j--)
		delete toolTips[j];
	toolTips.clear();

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);
		DrawToolTips();
		fillrect(vid_buf, (XRES-BARSIZE-width)-4, y-5, width+16, height+16+rows, 0, 0, 0, 100);
		drawrect(vid_buf, (XRES-BARSIZE-width)-4, y-5, width+16, height+16+rows, 255, 255, 255, 255);
		fillrect(vid_buf, (XRES-BARSIZE)+14, someStrangeYValue, 15, FONT_H+4, 0, 0, 0, 100);
		drawrect(vid_buf, (XRES-BARSIZE)+13, someStrangeYValue, 16, FONT_H+4, 255, 255, 255, 255);
		drawrect(vid_buf, (XRES-BARSIZE)+12, someStrangeYValue+1, 1, FONT_H+2, 0, 0, 0, 255);
		if (i) //if not in walls
			drawtext(vid_buf, 12, 12, "\bgPress 'o' to return to the original menu", 255, 255, 255, 255);
		
		Tool *over = menu_draw(mx, my, b, bq, active_menu);
		if (over)
		{
			int height = (int)(ceil((float)menuSections[active_menu]->tools.size()/16.0f)*18);
			int textY = (((YRES/GetNumMenus())*active_menu)+((YRES/GetNumMenus())/2))-(height/2)+(FONT_H/2)+25+height;
			menu_draw_text(over, textY);
		}

		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		memcpy(vid_buf, old_vid, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
		if (!(mx>=(XRES-BARSIZE-width)-4 && my>=sy-5 && my<sy+height+14) || (mx >= XRES-BARSIZE && !(my >= someStrangeYValue && my <= someStrangeYValue+FONT_H+4)))
		{
			break;
		}

		if (over)
			menu_select_element(b, over);

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	free(old_vid);

	//wipe out existing toolTips again, to prevent them from persisting once interface closes
	for (int i = toolTips.size()-1; i >= 0; i--)
		delete toolTips[i];
	toolTips.clear();
}

//current menu function
void menu_ui_v3(pixel *vid_buf, int i, int b, int bq, int mx, int my)
{
	Tool* over = menu_draw(mx, my, b, bq, i);
	if (over)
	{
		menu_draw_text(over, YRES-9);
		menu_select_element(b, over);
	}
}

int scrollbar(int fwidth, int mx, int y)
{
	int scrollSize, scrollbarx;
	float overflow, location;
	if (mx > XRES)
		mx = XRES;
	//if (mx < 15) //makes scrolling a little nicer at edges but apparently if you put hundreds of elements in a menu it makes the end not show ...
	//	mx = 15;
	scrollSize = (int)(((float)(XRES-BARSIZE))/((float)fwidth) * ((float)XRES-BARSIZE));
	scrollbarx = (int)(((float)mx/((float)XRES))*(float)(XRES-scrollSize));
	if (scrollSize+scrollbarx>XRES)
		scrollbarx = XRES-scrollSize;
	fillrect(vid_buf, scrollbarx, y+19, scrollSize, 3, 200, 200, 200, 255);

	overflow = (float)fwidth-XRES+10;
	location = ((float)XRES-3)/((float)(mx-(XRES-2)));
	return (int)(overflow / location);
}

Tool* menu_draw(int mx, int my, int b, int bq, int i)
{
	int x, y, n=0, xoff=0, fwidth = menuSections[i]->tools.size()*31;
	Tool* over = NULL;
	if (!old_menu || i >= SC_FAV)
	{
		x = XRES-BARSIZE-18;
		y = YRES+1;
	}
	else
	{
		int i2 = i;
		int height = (int)(ceil((float)menuSections[i]->tools.size()/16.0f)*18);

		if (i == SC_FAV2 || i == SC_HUD)
			i2 = SC_FAV;
		x = XRES-BARSIZE-23;
		y = (((YRES/GetNumMenus())*i2)+((YRES/GetNumMenus())/2))-(height/2)+(FONT_H/2)+11;
	}

	//draw everything in the decoration editor
	if (i == SC_DECO)
	{
		decoration_editor(vid_buf, b, bq, mx, my);

		int presetx = 6;
		for (n = DECO_PRESET_START; n < DECO_PRESET_START+NUM_COLOR_PRESETS; n++)
		{
			if (!bq && mx>=presetx-1 && mx<presetx+27 && my>=y && my< y+15)
			{
				drawrect(vid_buf, presetx-1, y-1, 29, 17, 255, 55, 55, 255);
				std::stringstream identifier;
				identifier << "DEFAULT_DECOR_COLOUR_" << colourlist[n-DECO_PRESET_START].descs;
				over =  new Tool(DECO_TOOL, n, identifier.str());
			}
			draw_tool_button(vid_buf, presetx, y, PIXPACK(colourlist[n-DECO_PRESET_START].colour), "");
			presetx += 31;
		}
	}
	else if (i == SC_HUD)
	{
		fwidth = 0;
		for (n = 0; n < HUD_NUM; n++)
		{
			if (hud_menu[n].menunum == hud_menunum || n == 0)
			{
				fwidth += 31;
			}
		}
	}

	//fancy scrolling
	if ((!old_menu || i >= SC_FAV) && fwidth > XRES-BARSIZE)
	{
		xoff = scrollbar(fwidth, mx, y);
	}

	//main loop, draws the tools and figures out which tool you are hovering over / selecting
	for (std::vector<Tool*>::iterator iter = menuSections[i]->tools.begin(), end = menuSections[i]->tools.end(); iter != end; ++iter)
	{
		Tool* current = *iter;
		//special cases for fake menu things in my mod
		if (i == SC_FAV || i == SC_FAV2 || i == SC_HUD)
		{
			last_fav_menu = i;
			if (i == SC_HUD && hud_menu[current->GetID()-HUD_START].menunum != hud_menunum && current->GetID() != HUD_START)
				continue;
			else if (i == SC_FAV && iter != menuSections[i]->tools.begin())
			{
				current = GetToolFromIdentifier(favMenu[iter-menuSections[i]->tools.begin()-1]);
				if (!current)
					continue;
			}
		}
		//if it's offscreen to the right or left, draw nothing
		if (x-xoff > XRES-26 || x-xoff < 0)
		{
			x -= 31;
			continue;
		}
		if (old_menu && x-26<=60 && i < SC_FAV)
		{
			x = XRES-BARSIZE-23;
			y += 19;
		}
		x -= draw_tool_xy(vid_buf, x-xoff, y, current)+5;

		if (i == SC_HUD) //HUD menu is special and should hopefully be removed someday ...
		{
			if (!bq && mx>=x+32-xoff && mx<x+58-xoff && my>=y && my< y+15)
			{
				drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 255, 55, 55, 255);
				over = current;
			}
			else if (current->GetID() >= HUD_REALSTART && currentHud[current->GetID()-HUD_REALSTART] && !strstr(hud_menu[current->GetID()-HUD_START].name, "#"))
			{
				drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 0, 255, 0, 255);
			}
		}
		else
		{
			//if mouse inside button
			if (!bq && mx>=x+32-xoff && mx<x+58-xoff && my>=y && my<y+15)
			{
				over = current;
				//draw rectangles around hovered on tools
				if (sdl_mod & (KMOD_LALT) && sdl_mod & (KMOD_CTRL|KMOD_META) && ((ElementTool*)current)->GetID() >= 0)
					drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 0, 255, 255, 255);
				else if (sdl_mod & (KMOD_SHIFT) && sdl_mod & (KMOD_CTRL|KMOD_META))
					drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 0, 255, 0, 255);
				else
					drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 255, 55, 55, 255);

				if (b && ((ToolTool*)current)->GetID() == TOOL_PROP)
					prop_edit_ui(vid_buf);
			}
			//draw rectangles around selected tools
			else if (activeTools[0]->GetIdentifier() == current->GetIdentifier())
				drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 255, 55, 55, 255);
			else if (activeTools[1]->GetIdentifier() == current->GetIdentifier())
				drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 55, 55, 255, 255);
			else if (activeTools[2]->GetIdentifier() == current->GetIdentifier())
				drawrect(vid_buf, x+30-xoff, y-1, 29, 17, 0, 255, 255, 255);
			else if (i == SC_FAV && menuSections[i]->tools.end()-iter <= locked)
				fillrect(vid_buf, x+31-xoff, y, 27, 15, 0, 0, 255, 127);
		}
	}
	return over;
}

void menu_draw_text(Tool* over, int y)
{
	std::stringstream toolTip;
	int toolID = over->GetID();
	if (over->GetType() == ELEMENT_TOOL)
	{
		toolTip << ptypes[toolID].descs;
	}
	else if (toolID >= HUD_START && toolID < HUD_START+HUD_NUM)
	{
		if (!strstr(hud_menu[toolID-HUD_START].name,"#"))
			toolTip << hud_menu[toolID-HUD_START].description;
		else
		{
			toolTip << hud_menu[toolID-HUD_START].description << currentHud[toolID-HUD_REALSTART] << " decimal places";
		}
	}
	else if (toolID >= FAV_START && toolID < FAV_END)
	{
		toolTip << fav[toolID-FAV_START].description;
		if (toolID == FAV_ROTATE)
		{
			if (globalSim->msRotation)
				toolTip << "on";
			else
				toolTip << "off";
		}
		if (toolID == FAV_HEAT)
		{
			if (!heatmode)
				toolTip << "normal: -273.15C - 9725.85C";
			else if (heatmode == 1)
				toolTip << "automatic: " << lowesttemp-273 << "C - " << highesttemp-273 << "C";
			else
				toolTip << "manual: " << lowesttemp-273 << "C - " << highesttemp-273 << "C";
		}
		/*else if (toolID == FAV_AUTOSAVE)
		{
			if (!autosave)
				toolTip << "off";
			else
				toolTip << "every " << autosave << " frames";
		}*/
		else if (toolID == FAV_REAL)
		{
			if (realistic)
				toolTip << "on";
			else
				toolTip << "off";
		}
		else if (toolID == FAV_FIND2)
		{
			if (finding &0x8)
				toolTip << "on";
			else
				toolTip << "off";
		}
		else if (toolID == FAV_DATE)
		{
			char *time;
			converttotime("1300000000", &time, -1, -1, -1);
			toolTip << time;
		}
	}
	else if (over->GetType() == GOL_TOOL)
		toolTip << golTypes[toolID].description;
	else if (toolID >= DECO_PRESET_START && toolID < DECO_PRESET_START + NUM_COLOR_PRESETS)
		toolTip << colourlist[toolID-DECO_PRESET_START].descs;
	else if (over->GetType() == WALL_TOOL)
		toolTip << wallTypes[toolID].descs;
	else if (over->GetType() == TOOL_TOOL)
		toolTip << toolTypes[toolID].descs;
	else if (over->GetType() == DECO_TOOL)
		toolTip << decoTypes[toolID].descs;
	if (toolTip.str().size())
		UpdateToolTip(toolTip.str(), Point(XRES-textwidth(toolTip.str().c_str())-BARSIZE, y), ELEMENTTIP, -1);
}

void menu_select_element(int b, Tool* over)
{
	int toolID = over->GetID();
	if (b==1 && over)
	{
		if (toolID >= FAV_START && toolID <= FAV_END)
		{
			if (toolID == FAV_MORE)
				active_menu = SC_FAV2;
			else if (toolID == FAV_BACK)
				active_menu = SC_FAV;
			else if (toolID == FAV_FIND)
			{
				if (finding & 0x4)
					finding &= 0x8;
				else if (finding &	0x2)
					finding |= 0x4;
				else if (finding &	0x1)
					finding |= 0x2;
				else
					finding |= 0x1;
			}
			else if (toolID == FAV_INFO)
				drawinfo = !drawinfo;
			else if (toolID == FAV_ROTATE)
				globalSim->msRotation = !globalSim->msRotation;
			else if (toolID == FAV_HEAT)
				heatmode = (heatmode + 1)%3;
			else if (toolID == FAV_LUA)
#ifdef LUACONSOLE
				ReadLuaCode();
#else
				error_ui(vid_buf, 0, "Lua console not enabled");
#endif
			else if (toolID == FAV_CUSTOMHUD)
				active_menu = SC_HUD;
			/*else if (toolID == FAV_AUTOSAVE)
			{
				autosave = atoi(input_ui(vid_buf,"Autosave","Input number of frames between saves, 0 = off","",""));
				if (autosave < 0)
					autosave = 0;
			}*/
			else if (toolID == FAV_REAL)
			{
				realistic = !realistic;
				if (realistic)
					ptypes[PT_FIRE].hconduct = 1;
				else
					ptypes[PT_FIRE].hconduct = 88;
			}
			else if (toolID == FAV_FIND2)
			{
				if (finding & 0x8)
					finding &= ~0x8;
				else
					finding |= 0x8;
			}
			else if (toolID == FAV_DATE)
			{
				if (dateformat == 5)
					dateformat = 12;
				else if (dateformat == 11)
					dateformat = 13;
				else if (dateformat == 12)
					dateformat = 0;
				else if (dateformat >= 13)
					dateformat = 6;
				else
					dateformat = dateformat + 1;
			}
			else if (toolID == FAV_SECR)
			{
				secret_els = !secret_els;
				FillMenus();
			}
		}
		else if (toolID >= HUD_START && toolID < HUD_START+HUD_NUM)
		{
			if (toolID == HUD_START)
			{
				if (hud_menunum != 0)
					hud_menunum = 0;
				else
					active_menu = SC_FAV2;
			}
			else if (toolID == HUD_START + 4)
			{
				HudDefaults();
				SetCurrentHud();
			}
			else if (toolID < HUD_REALSTART)
			{
				hud_menunum = toolID - HUD_START;
			}
			else
			{
				char hud_curr[16];
				sprintf(hud_curr,"%i",currentHud[toolID-HUD_REALSTART]);
				if (strstr(hud_menu[toolID-HUD_START].name,"#"))
					currentHud[toolID-HUD_REALSTART] = atoi(input_ui(vid_buf,(char*)hud_menu[toolID-HUD_START].name,"Enter number of decimal places",hud_curr,""));
				else
					currentHud[toolID-HUD_REALSTART] = !currentHud[toolID-HUD_REALSTART];
				if (DEBUG_MODE)
					memcpy(debugHud,currentHud,sizeof(debugHud));
				else
					memcpy(normalHud,currentHud,sizeof(normalHud));
			}
		}
		else if (toolID >= DECO_PRESET_START && toolID < DECO_PRESET_START+NUM_COLOR_PRESETS)
		{
			ARGBColour newDecoColor = colourlist[toolID-DECO_PRESET_START].colour;
			if (newDecoColor != decocolour)
				decocolour = newDecoColor;
			else
			{
				if (activeTools[0]->GetIdentifier() != "DEFAULT_DECOR_SET")
				{
					activeTools[0] = GetToolFromIdentifier("DEFAULT_DECOR_SET");
				}
			}
			currR = COLR(decocolour), currG = COLG(decocolour), currB = COLB(decocolour), currA = COLA(decocolour);
			RGB_to_HSV(currR, currG, currB, &currH, &currS, &currV);
		}
		else if ((sdl_mod & (KMOD_LALT)) && (sdl_mod & (KMOD_CTRL|KMOD_META)) && ((ElementTool*)over)->GetID() >= 0)
		{
			activeTools[2] = over;
		}
		else
		{
			int j, pos = 0, pos2 = 0, last = 17-locked, lock = 0;
			for (j = 0; j < 18; j++)
				if (favMenu[j] == over->GetIdentifier())
					pos = j; // If el is alredy on list, don't put it there twice
			pos2 = pos;
			if ((sdl_mod & (KMOD_SHIFT)) && (sdl_mod & (KMOD_CTRL|KMOD_META)) && locked < 18 && pos < 18-locked)
			{
				lock = 1;
			}
			if (pos > 18-locked && locked != 18) lock = 1;
			if (lock) last = 17;
			while (pos < last)
			{
				favMenu[pos] = favMenu[pos+1];
				pos = pos + 1;
			}
			if (last != 0)
				favMenu[last] = over->GetIdentifier();
			if ((sdl_mod & (KMOD_SHIFT)) && (sdl_mod & (KMOD_CTRL|KMOD_META)) && locked < 18 && pos2 <= 18-locked)
			{
				locked = locked + 1;
				save_presets(0);
			}
			else
			{

				activeTools[0] = over;
				activeToolID = 0;
			}
		}
	}
	if (b==4 && over)
	{
		if (toolID >= FAV_START && toolID <= FAV_END)
		{
			if (toolID == FAV_HEAT)
			{
				heatmode = 2;
				lowesttemp = atoi(input_ui(vid_buf,"Manual Heat Display","Enter a Minimum Temperature in Celcius","",""))+273;
				highesttemp = atoi(input_ui(vid_buf,"Manual Heat Display","Enter a Maximum Temperature in Celcius","",""))+273;
			}
			else if (toolID == FAV_DATE)
			{
				if (dateformat == 12)
					dateformat = 13;
				else if (dateformat == 13)
					dateformat = 12;
				else if (dateformat < 6)
					dateformat += 6;
				else
					dateformat -= 6;
			}
		}
		else if (toolID >= HUD_START && toolID < HUD_START+HUD_NUM)
		{
		}
		else if ((sdl_mod & (KMOD_LALT)) && (sdl_mod & (KMOD_CTRL|KMOD_META)) && ((ElementTool*)over)->GetID() >= 0)
		{
			activeTools[2] = over;
		}
		else
		{
			int j, pos = 0, last = 17-locked, lock = 0;
			for (j = 0; j < 18; j++)
				if (favMenu[j] == over->GetIdentifier())
					pos = j; // If el is alredy on list, don't put it there twice
			if (pos > 18-locked && locked != 18) lock = 1;
			if (lock) last = 17;
			if (pos > 17-locked && (sdl_mod & KMOD_SHIFT) && (sdl_mod & (KMOD_CTRL|KMOD_META)) && locked > 0 && active_menu == SC_FAV)
			{
				std::string temp = favMenu[pos];
				for (j = pos; j > 18-locked; j--)
					favMenu[j] = favMenu[j-1];
				favMenu[18-locked] = temp;
				locked = locked - 1;
				save_presets(0);
			}
			else
			{
				activeTools[1] = over;
				activeToolID = 1;
				while (pos < last)
				{
					favMenu[pos] = favMenu[pos+1];
					pos = pos + 1;
				}
				if (last != 0)
					favMenu[last] = over->GetIdentifier();
			}
		}
	}
}

char tabNames[10][255];
pixel* tabThumbnails[10];
int quickoptionsThumbnailFade = 0;
int clickedQuickoption = -1, hoverQuickoption = -1;
void quickoptions_menu(pixel *vid_buf, int b, int bq, int x, int y)
{
	int i = 0;
	bool isQuickoptionClicked = false, switchTabsOverride = false;
	//normal quickoptions
	if (!show_tabs && !(sdl_mod & (KMOD_CTRL|KMOD_META)))
	{
		while(quickmenu[i].icon!=NULL)
		{
			if(quickmenu[i].type == QM_TOGGLE)
			{
				drawrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 255, 255, 255, 255);
				if(*(quickmenu[i].variable) || clickedQuickoption == i)
				{
					fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 255, 255, 255, 255);
					drawtext(vid_buf, (XRES+BARSIZE)-11, (i*16)+5, quickmenu[i].icon, 0, 0, 0, 255);
				}
				else
				{
					fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 0, 0, 0, 255);
					drawtext(vid_buf, (XRES+BARSIZE)-11, (i*16)+5, quickmenu[i].icon, 255, 255, 255, 255);
				}
				if(x >= (XRES+BARSIZE)-16 && x <= (XRES+BARSIZE)-2 && y >= (i*16)+1 && y <= (i*16)+15)
				{
					UpdateToolTip(quickmenu[i].name, Point((XRES - 5) - textwidth(quickmenu[i].name), (i*16)+6), QTIP, -1);

					if (b == 1 && !bq)
					{
						if (clickedQuickoption == -1)
							clickedQuickoption = i;
						isQuickoptionClicked = true;
					}
					else if (b == 1 || bq == 1)
					{
						if (clickedQuickoption == i)
							isQuickoptionClicked = true;
					}
				}
			}
			i++;
		}
	}
	//tab menu
	else
	{
		while(i < num_tabs + 2 && i < 23-GetNumMenus())
		{
			char num[8];
			sprintf(num,"%d",i);
			drawrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 255, 255, 255, 255);
			if (i == 0)
			{
				fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 255, 255, 255, 255);
				drawtext(vid_buf, (XRES+BARSIZE)-11, (i*16)+5, quickmenu[i].icon, 0, 0, 0, 255);
			}
			else if (i == num_tabs + 1)
			{
				fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 0, 0, 0, 255);
				drawtext(vid_buf, (XRES+BARSIZE)-13, (i*16)+3, "\x89", 255, 255, 255, 255);
			}
			else if(tab_num == i)
			{
				fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 255, 255, 255, 255);
				drawtext(vid_buf, (XRES+BARSIZE)-11, (i*16)+5, num, 0, 0, 0, 255);
			}
			else
			{
				fillrect(vid_buf, (XRES+BARSIZE)-16, (i*16)+1, 14, 14, 0, 0, 0, 255);
				drawtext(vid_buf, (XRES+BARSIZE)-11, (i*16)+5, num, 255, 255, 255, 255);
			}
			if(x >= (XRES+BARSIZE)-16 && x <= (XRES+BARSIZE)-2 && y >= (i*16)+1 && y <= (i*16)+15)
			{
				std::string toolTip;
				if (i == 0)
					toolTip = quickmenu[i].name;
				else if (i == num_tabs + 1)
					toolTip = "Add tab \bg(ctrl+n)";
				else if (tab_num == i)
				{
					if (strlen(svf_name))
						toolTip = svf_name;
					else if (strlen(svf_filename))
						toolTip = svf_filename;
					else
						toolTip = "Untitled Simulation (current)";
				}
				else
					toolTip = tabNames[i-1];
				UpdateToolTip(toolTip, Point((XRES - 5) - textwidth(toolTip.c_str()), (i*16)+6), QTIP, -1);

				if (i > 0 && i <= num_tabs && tab_num != i)
				{
					quickoptionsThumbnailFade += 2;
					if (quickoptionsThumbnailFade > 12)
						quickoptionsThumbnailFade = 12;
					hoverQuickoption = i;
				}
				if (b && !bq)
				{
					if (clickedQuickoption == -1)
						clickedQuickoption = i;
					isQuickoptionClicked = true;
				}
				else if (b || bq)
				{
					if (clickedQuickoption == i)
						isQuickoptionClicked = true;
				}
			}
			i++;
		}
	}
	if (sdl_key == SDLK_TAB && (sdl_mod & KMOD_CTRL))
	{
		if (sdl_mod & KMOD_SHIFT)
		{
			if (tab_num > 1)
			{
				clickedQuickoption = tab_num - 1;
				switchTabsOverride = true;
			}
		}
		else if (tab_num < 22-GetNumMenus())
		{
			clickedQuickoption = tab_num + 1;
			switchTabsOverride = true;
		}
	}
	else if (!isQuickoptionClicked)
		clickedQuickoption = -1;
	if ((clickedQuickoption >= 0 && !b && bq) || switchTabsOverride)
	{
		if (!show_tabs && !switchTabsOverride && !(sdl_mod & (KMOD_CTRL|KMOD_META)))
		{
			if (bq == 1)
			{
				if (clickedQuickoption == 3)
				{
					if(!ngrav_enable)
						start_grav_async();
					else
						stop_grav_async();
				}
				else
					*(quickmenu[clickedQuickoption].variable) = !(*(quickmenu[clickedQuickoption].variable));
			}
		}
		else
		{
			if (bq == 1 || switchTabsOverride)
			{
				//toggle show_tabs
				if (clickedQuickoption == 0)
					*(quickmenu[clickedQuickoption].variable) = !(*(quickmenu[clickedQuickoption].variable));
				//start a new tab
				else if (clickedQuickoption == num_tabs + 1)
				{
					tab_save(tab_num, 0);
					num_tabs++;
					tab_num = num_tabs;
					if (sdl_mod & (KMOD_CTRL|KMOD_META))
						NewSim();
					tab_save(tab_num, 1);
				}
				//load a different tab
				else if (tab_num != clickedQuickoption)
				{
					tab_save(tab_num, 0);
					tab_load(clickedQuickoption);
					tab_num = clickedQuickoption;
				}
				//clicked current tab, do nothing
				else
				{
				}
			}
			else if (bq == 4 && clickedQuickoption > 0 && clickedQuickoption <= num_tabs)
			{
				if (num_tabs > 1)
				{
					char name[30], newname[30];
					//delete the tab that was closed, free thumbnail
					sprintf(name, "tabs%s%d.stm", PATH_SEP, clickedQuickoption);
					remove(name);
					free(tabThumbnails[clickedQuickoption-1]);
					tabThumbnails[clickedQuickoption-1] = NULL;

					//rename all the tabs and move other variables around
					for (i = clickedQuickoption; i < num_tabs; i++)
					{
						sprintf(name, "tabs%s%d.stm", PATH_SEP, i+1);
						sprintf(newname, "tabs%s%d.stm", PATH_SEP, i);
						rename(name, newname);
						strncpy(tabNames[i-1], tabNames[i], 254);
						tabThumbnails[i-1] = tabThumbnails[i];
					}

					num_tabs--;
					tabThumbnails[num_tabs] = NULL;
					if (clickedQuickoption == tab_num)
					{
						if (tab_num > 1)
							tab_num --;
						//if you deleted current tab, load the new one
						tab_load(tab_num);
					}
					else if (tab_num > clickedQuickoption && tab_num > 1)
						tab_num--;
				}
				else
				{
					NewSim();
					tab_save(tab_num, 1);
				}
			}
		}
	}

	if (quickoptionsThumbnailFade && tabThumbnails[hoverQuickoption-1])
	{
		drawrect(vid_buf, (XRES+BARSIZE)/3-1, YRES/3-1, (XRES+BARSIZE)/3+2, YRES/3+1, 0, 0, 255, quickoptionsThumbnailFade*21);
		draw_image(vid_buf, tabThumbnails[hoverQuickoption-1], (XRES+BARSIZE)/3, YRES/3, (XRES+BARSIZE)/3+1, YRES/3, quickoptionsThumbnailFade*21);
		quickoptionsThumbnailFade--;
	}
}

SDLKey MapNumpad(SDLKey key)
{
	switch(key)
	{
	case SDLK_KP8:
		return SDLK_UP;
	case SDLK_KP2:
		return SDLK_DOWN;
	case SDLK_KP6:
		return SDLK_RIGHT;
	case SDLK_KP4:
		return SDLK_LEFT;
	case SDLK_KP7:
		return SDLK_HOME;
	case SDLK_KP1:
		return SDLK_END;
	case SDLK_KP_PERIOD:
		return SDLK_DELETE;
	case SDLK_KP0:
	case SDLK_KP3:
	case SDLK_KP9:
		return SDLK_UNKNOWN;
	default:
		return key;
	}
}

int EventProcess(SDL_Event event)
{
	if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
	{
		if (event.key.keysym.unicode == 0)
		{
			// If unicode is zero, this could be a numpad key with numlock off, or numlock on and shift on (unicode is set to 0 by SDL or the OS in these circumstances. If numlock is on, unicode is the relevant digit character).
			// For some unknown reason, event.key.keysym.mod seems to be unreliable on some computers (keysum.mod&KEY_MOD_NUM is opposite to the actual value), so check keysym.unicode instead.
			// Note: unicode is always zero for SDL_KEYUP events, so this translation won't always work properly for keyup events.
			SDLKey newKey = MapNumpad(event.key.keysym.sym);
			if (newKey != event.key.keysym.sym)
			{
				event.key.keysym.sym = newKey;
				event.key.keysym.unicode = 0;
			}
		}
	}
	switch (event.type)
	{
	case SDL_KEYDOWN:
		sdl_key = event.key.keysym.sym;
		sdl_ascii = event.key.keysym.unicode;
		sdl_mod = static_cast<unsigned short>(SDL_GetModState());
		if (event.key.keysym.sym=='q' && (sdl_mod & (KMOD_CTRL|KMOD_META)))
		{
			if (confirm_ui(vid_buf, "You are about to quit", "Are you sure you want to quit?", "Quit"))
			{
				has_quit = 1;
				return 1;
			}
		}
#ifdef LUACONSOLE
		if (main_loop && !deco_disablestuff && sdl_key && !luacon_keyevent(sdl_key, sdl_mod, LUACON_KDOWN))
			sdl_key = 0;
#endif
		break;
	case SDL_KEYUP:
		sdl_rkey = event.key.keysym.sym;
		sdl_mod = static_cast<unsigned short>(SDL_GetModState());
#ifdef LUACONSOLE
		if (main_loop && !deco_disablestuff && sdl_rkey && !luacon_keyevent(sdl_rkey, sdl_mod, LUACON_KUP))
			sdl_rkey = 0;
#endif
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == SDL_BUTTON_WHEELUP)
			sdl_wheel++;
		if (event.button.button == SDL_BUTTON_WHEELDOWN)
			sdl_wheel--;
		break;
	case SDL_QUIT:
		if (fastquit)
			has_quit = 1;
		return 1;
	case SDL_SYSWMEVENT:
#if defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
		if (event.syswm.msg->subsystem != SDL_SYSWM_X11)
			break;
		sdl_wminfo.info.x11.lock_func();
		XEvent xe = event.syswm.msg->event.xevent;
		if (xe.type==SelectionClear)
		{
			if (clipboard_text!=NULL) {
				free(clipboard_text);
				clipboard_text = NULL;
			}
		}
		else if (xe.type==SelectionRequest)
		{
			XEvent xr;
			xr.xselection.type = SelectionNotify;
			xr.xselection.requestor = xe.xselectionrequest.requestor;
			xr.xselection.selection = xe.xselectionrequest.selection;
			xr.xselection.target = xe.xselectionrequest.target;
			xr.xselection.property = xe.xselectionrequest.property;
			xr.xselection.time = xe.xselectionrequest.time;
			if (xe.xselectionrequest.target==XA_TARGETS)
			{
				// send list of supported formats
				Atom targets[] = {XA_TARGETS, XA_STRING, XA_UTF8_STRING};
				xr.xselection.property = xe.xselectionrequest.property;
				XChangeProperty(sdl_wminfo.info.x11.display, xe.xselectionrequest.requestor, xe.xselectionrequest.property, XA_ATOM, 32, PropModeReplace, (unsigned char*)targets, (int)(sizeof(targets)/sizeof(Atom)));
			}
			// TODO: Supporting more targets would be nice
			else if ((xe.xselectionrequest.target==XA_STRING || xe.xselectionrequest.target==XA_UTF8_STRING) && clipboard_text)
			{
				XChangeProperty(sdl_wminfo.info.x11.display, xe.xselectionrequest.requestor, xe.xselectionrequest.property, xe.xselectionrequest.target, 8, PropModeReplace, (unsigned char*)clipboard_text, strlen(clipboard_text)+1);
			}
			else
			{
				// refuse clipboard request
				xr.xselection.property = None;
			}
			XSendEvent(sdl_wminfo.info.x11.display, xe.xselectionrequest.requestor, 0, 0, &xr);
		}
		sdl_wminfo.info.x11.unlock_func();
#elif WIN
		switch (event.syswm.msg->msg)
		{
		case WM_USER+614:
			if (!ptsaveOpenID && !saveURIOpen && num_tabs < 24-GetNumMenus() && main_loop)
				ptsaveOpenID = event.syswm.msg->lParam;
			//If we are already opening a save, we can't have it do another one, so just start it in a new process
			else
			{
				char *exename = exe_name(), args[64];
				sprintf(args, "ptsave noopen:%i", event.syswm.msg->lParam);
				if (exename)
				{
					ShellExecute(NULL, "open", exename, args, NULL, SW_SHOWNORMAL);
					free(exename);
				}
				//I doubt this will happen ... but as a last resort just open it in this window anyway
				else
					saveURIOpen = event.syswm.msg->lParam;
			}
			break;
		}
#else
		break;
#endif
	}
	return 0;
}

int FPSwait = 0, fastquit = 0;
int sdl_poll(void)
{
	SDL_Event event;
	sdl_key=sdl_rkey=sdl_wheel=sdl_ascii=0;
	if (has_quit)
		return 1;
	loop_time = SDL_GetTicks();
	if (main_loop > 0)
	{
		main_loop--;
		if (main_loop == 0)
		{
			SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
			FPSwait = 2;
		}
		else
			SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
	}

	while (SDL_PollEvent(&event))
	{
		int ret = EventProcess(event);
		if (ret)
			return ret;
	}

	sdl_mod = static_cast<unsigned short>(SDL_GetModState());
	limit_fps();
	return 0;
}

void stickmen_keys()
{
	//  4
	//1 8 2
	if (sdl_key == SDLK_RIGHT)
	{
		player.comm = (int)(player.comm)|0x02;  //Go right command
	}
	if (sdl_key == SDLK_LEFT)
	{
		player.comm = (int)(player.comm)|0x01;  //Go left command
	}
	if (sdl_key == SDLK_DOWN && ((int)(player.comm)&0x08)!=0x08)
	{
		player.comm = (int)(player.comm)|0x08;  //Use element command
	}
	if (sdl_key == SDLK_UP && ((int)(player.comm)&0x04)!=0x04)
	{
		player.comm = (int)(player.comm)|0x04;  //Jump command
	}

	if (sdl_key == SDLK_d)
	{
		player2.comm = (int)(player2.comm)|0x02;  //Go right command
	}
	if (sdl_key == SDLK_a)
	{
		player2.comm = (int)(player2.comm)|0x01;  //Go left command
	}
	if (sdl_key == SDLK_s && ((int)(player2.comm)&0x08)!=0x08)
	{
		player2.comm = (int)(player2.comm)|0x08;  //Use element command
	}
	if (sdl_key == SDLK_w && ((int)(player2.comm)&0x04)!=0x04)
	{
		player2.comm = (int)(player2.comm)|0x04;  //Jump command
	}

	if (sdl_rkey == SDLK_RIGHT || sdl_rkey == SDLK_LEFT)
	{
		player.pcomm = player.comm;  //Saving last movement
		player.comm = (int)(player.comm)&12;  //Stop command
	}
	if (sdl_rkey == SDLK_UP)
	{
		player.comm = (int)(player.comm)&11;
	}
	if (sdl_rkey == SDLK_DOWN)
	{
		player.comm = (int)(player.comm)&7;
	}

	if (sdl_rkey == SDLK_d || sdl_rkey == SDLK_a)
	{
		player2.pcomm = player2.comm;  //Saving last movement
		player2.comm = (int)(player2.comm)&12;  //Stop command
	}
	if (sdl_rkey == SDLK_w)
	{
		player2.comm = (int)(player2.comm)&11;
	}
	if (sdl_rkey == SDLK_s)
	{
		player2.comm = (int)(player2.comm)&7;
	}
}

int pastFPS = 0;
float FPSB2 = 60.0f;
double frameTimeAvg = 0.0, correctedFrameTimeAvg = 60.0;
void limit_fps()
{
	int frameTime = SDL_GetTicks() - currentTime;

	frameTimeAvg = frameTimeAvg * .8 + frameTime * .2;
	if (limitFPS > 2)
	{
		double offset = 1000.0 / limitFPS - frameTimeAvg;
		if (offset > 0)
			SDL_Delay((Uint32)(offset + 0.5));
	}

	int correctedFrameTime = SDL_GetTicks() - currentTime;
	correctedFrameTimeAvg = correctedFrameTimeAvg * 0.95 + correctedFrameTime * 0.05;
	elapsedTime = currentTime-pastFPS;
	if (elapsedTime >= 500)
	{
		if (!FPSwait)
			FPSB2 = 1000.0f/(float)correctedFrameTimeAvg;
		if (main_loop && FPSwait > 0)
			FPSwait--;
		pastFPS = currentTime;
	}
	currentTime = SDL_GetTicks();
}

void set_cmode(int cm) // sets to given view mode
{
	int cmode = cm;
	std::string toolTip = "Error: Incorrect Display Number";
	colour_mode = COLOUR_DEFAULT;
	
	free(render_modes);
	render_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
	render_modes[0] = RENDER_BASC;
	render_modes[1] = 0;
	
	free(display_modes);
	display_modes = (unsigned int*)calloc(1, sizeof(unsigned int));
	display_modes[0] = 0;

	if (cmode==CM_VEL)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(3, sizeof(unsigned int));
		render_modes[0] = RENDER_EFFE;
		render_modes[1] = RENDER_BASC;
		render_modes[2] = 0;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_modes[0] = DISPLAY_AIRV;
		display_modes[1] = 0;
		toolTip = "Velocity Display";
	}
	else if (cmode==CM_PRESS)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(3, sizeof(unsigned int));
		render_modes[0] = RENDER_EFFE;
		render_modes[1] = RENDER_BASC;
		render_modes[2] = 0;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_modes[0] = DISPLAY_AIRP;
		display_modes[1] = 0;
		toolTip = "Pressure Display";
	}
	else if (cmode==CM_PERS)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(3, sizeof(unsigned int));
		render_modes[0] = RENDER_EFFE;
		render_modes[1] = RENDER_BASC;
		render_modes[2] = 0;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_modes[0] = DISPLAY_PERS;
		display_modes[1] = 0;
		memset(pers_bg, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
		toolTip = "Persistent Display";
	}
	else if (cmode==CM_FIRE)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(5, sizeof(unsigned int));
		render_modes[0] = RENDER_FIRE;
		render_modes[1] = RENDER_SPRK;
		render_modes[2] = RENDER_EFFE;
		render_modes[3] = RENDER_BASC;
		render_modes[4] = 0;
		memset(fire_r, 0, sizeof(fire_r));
		memset(fire_g, 0, sizeof(fire_g));
		memset(fire_b, 0, sizeof(fire_b));
		toolTip = "Fire Display";
	}
	else if (cmode==CM_BLOB)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(5, sizeof(unsigned int));
		render_modes[0] = RENDER_FIRE;
		render_modes[1] = RENDER_SPRK;
		render_modes[2] = RENDER_EFFE;
		render_modes[3] = RENDER_BLOB;
		render_modes[4] = 0;
		memset(fire_r, 0, sizeof(fire_r));
		memset(fire_g, 0, sizeof(fire_g));
		memset(fire_b, 0, sizeof(fire_b));
		toolTip = "Blob Display";
	}
	else if (cmode==CM_HEAT)
	{
		colour_mode = COLOUR_HEAT;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_mode |= DISPLAY_AIRH;
		display_modes[0] = DISPLAY_AIRH;
		display_modes[1] = 0;
		toolTip = "Heat Display";
	}
	else if (cmode==CM_FANCY)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(7, sizeof(unsigned int));
		render_modes[0] = RENDER_FIRE;
		render_modes[1] = RENDER_SPRK;
		render_modes[2] = RENDER_GLOW;
		render_modes[3] = RENDER_BLUR;
		render_modes[4] = RENDER_EFFE;
		render_modes[5] = RENDER_BASC;
		render_modes[6] = 0;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_modes[0] = DISPLAY_WARP;
		display_modes[1] = 0;
		memset(fire_r, 0, sizeof(fire_r));
		memset(fire_g, 0, sizeof(fire_g));
		memset(fire_b, 0, sizeof(fire_b));
		toolTip = "Fancy Display";
	}
	else if (cmode==CM_NOTHING)
	{
		toolTip = "Nothing Display";
	}
	else if (cmode==CM_GRAD)
	{
		colour_mode = COLOUR_GRAD;
		toolTip = "Heat Gradient Display";
	}
	else if (cmode==CM_LIFE)
	{
		colour_mode = COLOUR_LIFE;
		toolTip = "Life Gradient Display";
	}
	else if (cmode==CM_CRACK)
	{
		free(render_modes);
		render_modes = (unsigned int*)calloc(3, sizeof(unsigned int));
		render_modes[0] = RENDER_EFFE;
		render_modes[1] = RENDER_BASC;
		render_modes[2] = 0;
		free(display_modes);
		display_modes = (unsigned int*)calloc(2, sizeof(unsigned int));
		display_modes[0] = DISPLAY_AIRC;
		display_modes[1] = 0;
		toolTip = "Alternate Velocity Display";
	}
	UpdateToolTip(toolTip, Point(XCNTR-textwidth(toolTip.c_str())/2, YCNTR-10), INFOTIP, 255);

	update_display_modes();// Update render_mode and display_mode from the relevant arrays
	save_presets(0);
}

char *download_ui(pixel *vid_buf, const char *uri, int *len)
{
	int dstate = 0;
	void *http = http_async_req_start(NULL, uri, NULL, 0, 0);
	int x0=(XRES-240)/2,y0=(YRES-MENUSIZE)/2;
	int done, total, i, ret, zlen, ulen;
	char str[16], *tmp, *res;
	
	if (svf_login)
	{
		http_auth_headers(http, svf_user, NULL, NULL);
	}

	while (!http_async_req_status(http))
	{
		sdl_poll();

		http_async_get_length(http, &total, &done);

		clearrect(vid_buf, x0-1, y0-1, 243, 63);
		drawrect(vid_buf, x0, y0, 240, 60, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, "Please wait", 255, 216, 32, 255);
		drawtext(vid_buf, x0+8, y0+26, "Downloading update...", 255, 255, 255, 255);

		if (total)
		{
			i = (236*done)/total;
			fillrect(vid_buf, x0+1, y0+45, i+1, 14, 255, 216, 32, 255);
			i = (100*done)/total;
			sprintf(str, "%d%%", i);
			if (i<50)
				drawtext(vid_buf, x0+120-textwidth(str)/2, y0+48, str, 192, 192, 192, 255);
			else
				drawtext(vid_buf, x0+120-textwidth(str)/2, y0+48, str, 0, 0, 0, 255);
		}
		else
			drawtext(vid_buf, x0+120-textwidth("Waiting...")/2, y0+48, "Waiting...", 255, 216, 32, 255);

		drawrect(vid_buf, x0, y0+44, 240, 16, 192, 192, 192, 255);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
	}

	tmp = http_async_req_stop(http, &ret, &zlen);
	if (ret!=200)
	{
		error_ui(vid_buf, ret, http_ret_text(ret));
		if (tmp)
			free(tmp);
		return NULL;
	}
	if (!tmp)
	{
		error_ui(vid_buf, 0, "Server did not return data");
		return NULL;
	}

	if (zlen<16)
	{
		printf("ZLen is not 16!\n");
		goto corrupt;
	}
	if (tmp[0]!=0x42 || tmp[1]!=0x75 || tmp[2]!=0x54 || tmp[3]!=0x54)
	{
		printf("Tmperr %d, %d, %d, %d\n", tmp[0], tmp[1], tmp[2], tmp[3]);
		goto corrupt;
	}

	ulen  = (unsigned char)tmp[4];
	ulen |= ((unsigned char)tmp[5])<<8;
	ulen |= ((unsigned char)tmp[6])<<16;
	ulen |= ((unsigned char)tmp[7])<<24;

	res = (char *)malloc(ulen);
	if (!res)
	{
		printf("No res!\n");
		goto corrupt;
	}
	dstate = BZ2_bzBuffToBuffDecompress((char *)res, (unsigned *)&ulen, (char *)(tmp+8), zlen-8, 0, 0);
	if (dstate)
	{
		printf("Decompression failure: %d, %d, %d!\n", dstate, ulen, zlen);
		free(res);
		goto corrupt;
	}
	
	free(tmp);
	if (len)
		*len = ulen;
	return res;

corrupt:
	error_ui(vid_buf, 0, "Downloaded update is corrupted");
	free(tmp);
	return NULL;
}

int search_ui(pixel *vid_buf)
{
	int nmp=-1,uih=0,nyu,nyd,b=1,bq,mx=0,my=0,mxq=0,myq=0,mmt=0,gi,gj,gx,gy,pos,i,mp,dp,dap,own,last_own=search_own,last_fav=search_fav,page_count=0,last_page=0,last_date=0,j,w,h,st=0,lv;
	int is_p1=0, exp_res=GRID_X*GRID_Y, tp, view_own=0, last_p1_extra=0, motdswap = rand()%2;
	int thumb_drawn[GRID_X*GRID_Y];
	pixel *v_buf = (pixel *)malloc(((YRES+MENUSIZE)*(XRES+BARSIZE))*PIXELSIZE);
	pixel *bthumb_rsdata = NULL;
	float ry;
	ui_edit ed;
	ui_richtext motd;


	Download *saveListDownload = NULL;
	char *last = NULL;
	int search = 0;

	Download *thumbnailDownloads[IMGCONNS];
	for (int i = 0; i < IMGCONNS; i++)
		thumbnailDownloads[i] = NULL;
	char *img_id[IMGCONNS];

	if (!v_buf)
		return 0;
	memset(v_buf, 0, ((YRES+MENUSIZE)*(XRES+BARSIZE))*PIXELSIZE);

	memset(img_id, 0, sizeof(img_id));

	memset(search_ids, 0, sizeof(search_ids));
	memset(search_dates, 0, sizeof(search_dates));
	memset(search_names, 0, sizeof(search_names));
	memset(search_scoreup, 0, sizeof(search_scoreup));
	memset(search_scoredown, 0, sizeof(search_scoredown));
	memset(search_publish, 0, sizeof(search_publish));
	memset(search_owners, 0, sizeof(search_owners));
	memset(search_thumbs, 0, sizeof(search_thumbs));
	memset(search_thsizes, 0, sizeof(search_thsizes));

	memset(thumb_drawn, 0, sizeof(thumb_drawn));

	do_open = 0;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	ui_edit_init(&ed, 65, 13, XRES-200, 14);
	ed.def = "[search terms]";
	ed.cursor = ed.cursorstart = strlen(search_expr);
	strcpy(ed.str, search_expr);

	motd.x = 20;
	motd.y = 33;
	motd.str[0] = 0;
	motd.printstr[0] = 0;

	sdl_wheel = 0;

	while (!sdl_poll())
	{
		uih = 0;
		bq = b;
		mxq = mx;
		myq = my;
		b = mouse_get_state(&mx, &my);

		if (mx!=mxq || my!=myq || sdl_wheel || b)
			mmt = 0;
		else if (mmt<TIMEOUT)
			mmt++;

		clearrect(vid_buf, 0, 0, XRES+BARSIZE, YRES+MENUSIZE);

		memcpy(vid_buf, v_buf, ((YRES+MENUSIZE)*(XRES+BARSIZE))*PIXELSIZE);

		drawtext(vid_buf, 11, 13, "Search:", 192, 192, 192, 255);
		if (!saveListDownload || saveListDownload->CheckStarted())
			drawtext(vid_buf, 51, 11, "\x8E", 192, 160, 32, 255);
		else
			drawtext(vid_buf, 51, 11, "\x8E", 32, 64, 160, 255);
		drawtext(vid_buf, 51, 11, "\x8F", 255, 255, 255, 255);
		drawrect(vid_buf, 48, 8, XRES-182, 16, 192, 192, 192, 255);

		if (!svf_login || search_fav)
		{
			search_own = 0;
			drawrect(vid_buf, XRES-64+16, 8, 56, 16, 96, 96, 96, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x94", 96, 80, 16, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x93", 128, 128, 128, 255);
			drawtext(vid_buf, XRES-46+16, 13, "My Own", 128, 128, 128, 255);
		}
		else if (search_own)
		{
			fillrect(vid_buf, XRES-65+16, 7, 58, 18, 255, 255, 255, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x94", 192, 160, 64, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x93", 32, 32, 32, 255);
			drawtext(vid_buf, XRES-46+16, 13, "My Own", 0, 0, 0, 255);
		}
		else
		{
			drawrect(vid_buf, XRES-64+16, 8, 56, 16, 192, 192, 192, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x94", 192, 160, 32, 255);
			drawtext(vid_buf, XRES-61+16, 11, "\x93", 255, 255, 255, 255);
			drawtext(vid_buf, XRES-46+16, 13, "My Own", 255, 255, 255, 255);
		}

		if(!svf_login)
		{
			search_fav = 0;
			drawrect(vid_buf, XRES-134, 8, 16, 16, 192, 192, 192, 255);
			drawtext(vid_buf, XRES-130, 11, "\xCC", 120, 120, 120, 255);
		}
		else if (search_fav)
		{
			fillrect(vid_buf, XRES-134, 7, 18, 18, 255, 255, 255, 255);
			drawtext(vid_buf, XRES-130, 11, "\xCC", 192, 160, 64, 255);
		}
		else
		{
			drawrect(vid_buf, XRES-134, 8, 16, 16, 192, 192, 192, 255);
			drawtext(vid_buf, XRES-130, 11, "\xCC", 192, 160, 32, 255);
		}

		if(search_fav)
		{
			search_date = 0;
			drawrect(vid_buf, XRES-129+16, 8, 60, 16, 96, 96, 96, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA9", 44, 48, 32, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA8", 32, 44, 32, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA7", 128, 128, 128, 255);
			drawtext(vid_buf, XRES-111+16, 13, "By votes", 128, 128, 128, 255);
		}
		else if (search_date)
		{
			fillrect(vid_buf, XRES-130+16, 7, 62, 18, 255, 255, 255, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA6", 32, 32, 32, 255);
			drawtext(vid_buf, XRES-111+16, 13, "By date", 0, 0, 0, 255);
		}
		else
		{
			drawrect(vid_buf, XRES-129+16, 8, 60, 16, 192, 192, 192, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA9", 144, 48, 32, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA8", 32, 144, 32, 255);
			drawtext(vid_buf, XRES-126+16, 11, "\xA7", 255, 255, 255, 255);
			drawtext(vid_buf, XRES-111+16, 13, "By votes", 255, 255, 255, 255);
		}

		if (search_page)
		{
			drawtext(vid_buf, 4, YRES+MENUSIZE-16, "\x96", 255, 255, 255, 255);
			drawrect(vid_buf, 1, YRES+MENUSIZE-20, 16, 16, 255, 255, 255, 255);
		}
		else if (page_count > exp_res && !(search_own || search_fav || search_date) && !strcmp(ed.str,""))
		{
			if (p1_extra)
				drawtext(vid_buf, 4, YRES+MENUSIZE-17, "\x85", 255, 255, 255, 255);
			else
				drawtext(vid_buf, 4, YRES+MENUSIZE-17, "\x89", 255, 255, 255, 255);
			drawrect(vid_buf, 1, YRES+MENUSIZE-20, 15, 15, 255, 255, 255, 255);
		}
		if (page_count > exp_res)
		{
			drawtext(vid_buf, XRES-15, YRES+MENUSIZE-16, "\x95", 255, 255, 255, 255);
			drawrect(vid_buf, XRES-18, YRES+MENUSIZE-20, 16, 16, 255, 255, 255, 255);
		}
		if (page_count)
		{
			char pagecount[16];
			sprintf(pagecount,"Page %i",search_page+1);
			drawtext(vid_buf, (XRES-textwidth(pagecount))/2, YRES+MENUSIZE-10, pagecount, 255, 255, 255, 255);
		}

		ui_edit_draw(vid_buf, &ed);

		if ((b && !bq && mx>=1 && mx<=17 && my>=YRES+MENUSIZE-20 && my<YRES+MENUSIZE-4) || sdl_wheel>0)
		{
			if (search_page)
				search_page --;
			else if (!(search_own || search_fav || search_date) && !sdl_wheel)
				p1_extra = !p1_extra;
			sdl_wheel = 0;
			uih = 1;
		}
		if ((b && !bq && mx>=XRES-18 && mx<=XRES-1 && my>=YRES+MENUSIZE-20 && my<YRES+MENUSIZE-4) || sdl_wheel<0)
		{
			if (page_count>exp_res)
			{
				search_page ++;
				page_count = exp_res;
			}
			sdl_wheel = 0;
			uih = 1;
		}

		tp = -1;
		if (is_p1)
		{	
			//Message of the day
			ui_richtext_process(mx, my, b, bq, &motd);
			ui_richtext_draw(vid_buf, &motd);
			//Popular tags
			drawtext(vid_buf, (XRES-textwidth("Popular tags:"))/2, 49, "Popular tags:", 255, 192, 64, 255);
			for (gj=0; gj<((GRID_Y-GRID_P)*YRES)/(GRID_Y*14); gj++)
				for (gi=0; gi<(GRID_X+1); gi++)
				{
					pos = gi+(GRID_X+1)*gj;
					if (pos>TAG_MAX || !tag_names[pos])
						break;
					if (tag_votes[0])
						i = 127+(128*tag_votes[pos])/tag_votes[0];
					else
						i = 192;
					w = textwidth(tag_names[pos]);
					if (w>XRES/(GRID_X+1)-5)
						w = XRES/(GRID_X+1)-5;
					gx = (XRES/(GRID_X+1))*gi;
					gy = gj*13 + 62;
					if (mx>=gx && mx<gx+(XRES/((GRID_X+1)+1)) && my>=gy && my<gy+14)
					{
						j = (i*5)/6;
						tp = pos;
					}
					else
						j = i;
					drawtextmax(vid_buf, gx+(XRES/(GRID_X+1)-w)/2, gy, XRES/(GRID_X+1)-5, tag_names[pos], j, j, i, 255);
				}
		}

		mp = dp = -1;
		dap = -1;
		st = 0;
		for (gj=0; gj<GRID_Y; gj++)
			for (gi=0; gi<GRID_X; gi++)
			{
				if (is_p1)
				{
					pos = gi+GRID_X*(gj-GRID_Y+GRID_P);
					if (pos<0)
						break;
				}
				else
					pos = gi+GRID_X*gj;
				if (!search_ids[pos])
					break;
				gx = ((XRES/GRID_X)*gi) + (XRES/GRID_X-XRES/GRID_S)/2;
				gy = ((((YRES-(MENUSIZE-20))+15)/GRID_Y)*gj) + ((YRES-(MENUSIZE-20))/GRID_Y-(YRES-(MENUSIZE-20))/GRID_S+10)/2 + 18;
				if (textwidth(search_names[pos]) > XRES/GRID_X-10)
				{
					char *tmp = (char*)malloc(strlen(search_names[pos])+4);
					strcpy(tmp, search_names[pos]);
					j = textwidthx(tmp, XRES/GRID_X-15);
					strcpy(tmp+j, "...");
					drawtext(vid_buf, gx+XRES/(GRID_S*2)-textwidth(tmp)/2, gy+YRES/GRID_S+7, tmp, 192, 192, 192, 255);
					free(tmp);
				}
				else
					drawtext(vid_buf, gx+XRES/(GRID_S*2)-textwidth(search_names[pos])/2, gy+YRES/GRID_S+7, search_names[pos], 192, 192, 192, 255);
				j = textwidth(search_owners[pos]);
				if (mx>=gx+XRES/(GRID_S*2)-j/2 && mx<=gx+XRES/(GRID_S*2)+j/2 &&
				        my>=gy+YRES/GRID_S+18 && my<=gy+YRES/GRID_S+29)
				{
					st = 1;
					drawtext(vid_buf, gx+XRES/(GRID_S*2)-j/2, gy+YRES/GRID_S+20, search_owners[pos], 128, 128, 160, 255);
				}
				else
					drawtext(vid_buf, gx+XRES/(GRID_S*2)-j/2, gy+YRES/GRID_S+20, search_owners[pos], 128, 128, 128, 255);
				if (search_thumbs[pos]&&thumb_drawn[pos]==0)
				{
					//render_thumb(search_thumbs[pos], search_thsizes[pos], 1, v_buf, gx, gy, GRID_S);
					int finh, finw;
					pixel *thumb_rsdata = NULL;
					pixel *thumb_imgdata = ptif_unpack(search_thumbs[pos], search_thsizes[pos], &finw, &finh);
					if(thumb_imgdata!=NULL){
						thumb_rsdata = resample_img(thumb_imgdata, finw, finh, XRES/GRID_S, YRES/GRID_S);
						draw_image(v_buf, thumb_rsdata, gx, gy, XRES/GRID_S, YRES/GRID_S, 255);					
						free(thumb_imgdata);
						free(thumb_rsdata);
					}
					thumb_drawn[pos] = 1;
				}
				own = (svf_login && (!strcmp(svf_user, search_owners[pos]) || svf_admin || svf_mod));
				if (mx>=gx-2 && mx<=gx+XRES/GRID_S+3 && my>=gy && my<=gy+YRES/GRID_S+29)
					mp = pos;
				if ((own || search_fav) && mx>=gx+XRES/GRID_S-4 && mx<=gx+XRES/GRID_S+6 && my>=gy-6 && my<=gy+4)
				{
					mp = -1;
					dp = pos;
				}
				if ((own || (unlockedstuff & 0x08)) && !search_dates[pos] && mx>=gx-6 && mx<=gx+4 && my>=gy+YRES/GRID_S-4 && my<=gy+YRES/GRID_S+6)
				{
					mp = -1;
					dap = pos;
				}
				drawrect(vid_buf, gx-2+(XRES/GRID_S)+5, gy-2, 6, YRES/GRID_S+3, 128, 128, 128, 255);
				fillrect(vid_buf, gx-2+(XRES/GRID_S)+5, gy-2, 6, 1+(YRES/GRID_S+3)/2, 0, 107, 10, 255);
				fillrect(vid_buf, gx-2+(XRES/GRID_S)+5, gy-2+((YRES/GRID_S+3)/2), 6, 1+(YRES/GRID_S+3)/2, 107, 10, 0, 255);

				if (mp==pos && !st)
					drawrect(vid_buf, gx-2, gy-2, XRES/GRID_S+3, YRES/GRID_S+3, 160, 160, 192, 255);
				else
					drawrect(vid_buf, gx-2, gy-2, XRES/GRID_S+3, YRES/GRID_S+3, 128, 128, 128, 255);
				if (own || search_fav)
				{
					if (dp == pos)
						drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x86", 255, 48, 32, 255);
					else
						drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x86", 160, 48, 32, 255);
					drawtext(vid_buf, gx+XRES/GRID_S-4, gy-6, "\x85", 255, 255, 255, 255);
				}
				if (!search_publish[pos])
				{
					drawtext(vid_buf, gx-6, gy-6, "\xCD", 255, 255, 255, 255);
					drawtext(vid_buf, gx-6, gy-6, "\xCE", 212, 151, 81, 255);
				}
				if (!search_dates[pos] && (own || (unlockedstuff & 0x08)))
				{
					fillrect(vid_buf, gx-5, gy+YRES/GRID_S-3, 7, 8, 255, 255, 255, 255);
					if (dap == pos) {
						drawtext(vid_buf, gx-6, gy+YRES/GRID_S-4, "\xA6", 200, 100, 80, 255);
					} else {
						drawtext(vid_buf, gx-6, gy+YRES/GRID_S-4, "\xA6", 160, 70, 50, 255);
					}
					//drawtext(vid_buf, gx-6, gy-6, "\xCE", 212, 151, 81, 255);
				}
				if (view_own || svf_admin || svf_mod || (unlockedstuff & 0x08))
				{
					char ts[64];
					sprintf(ts+1, "%d", search_votes[pos]);
					ts[0] = (char)0xBB;
					for (j=1; ts[j]; j++)
						ts[j] = (char)0xBC;
					ts[j-1] = (char)0xB9;
					ts[j] = (char)0xBA;
					ts[j+1] = 0;
					w = gx+XRES/GRID_S-2-textwidth(ts);
					h = gy+YRES/GRID_S-11;
					drawtext(vid_buf, w, h, ts, 16, 72, 16, 255);
					for (j=0; ts[j]; j++)
						ts[j] -= 14;
					drawtext(vid_buf, w, h, ts, 192, 192, 192, 255);
					sprintf(ts, "%d", search_votes[pos]);
					for (j=0; ts[j]; j++)
						if (ts[j] != '-')
							ts[j] += 127;
					drawtext(vid_buf, w+3, h, ts, 255, 255, 255, 255);
				}
				if (search_scoreup[pos]>0||search_scoredown[pos]>0)
				{
					lv = (search_scoreup[pos]>search_scoredown[pos]?search_scoreup[pos]:search_scoredown[pos]);

					if (((YRES/GRID_S+3)/2)>lv)
					{
						ry = ((float)((YRES/GRID_S+3)/2)/(float)lv);
						if (lv<8)
						{
							ry =  ry/(8-lv);
						}
						nyu = (int)(search_scoreup[pos]*ry);
						nyd = (int)(search_scoredown[pos]*ry);
					}
					else
					{
						ry = ((float)lv/(float)((YRES/GRID_S+3)/2));
						nyu = (int)(search_scoreup[pos]/ry);
						nyd = (int)(search_scoredown[pos]/ry);
					}


					fillrect(vid_buf, gx-1+(XRES/GRID_S)+5, gy-1+((YRES/GRID_S+3)/2)-nyu, 4, nyu, 57, 187, 57, 255);
					fillrect(vid_buf, gx-1+(XRES/GRID_S)+5, gy-2+((YRES/GRID_S+3)/2), 4, nyd, 187, 57, 57, 255);
					//drawrect(vid_buf, gx-2+(XRES/GRID_S)+5, gy-2+((YRES/GRID_S+3)/2)-nyu, 4, nyu, 0, 107, 10, 255);
					//drawrect(vid_buf, gx-2+(XRES/GRID_S)+5, gy-2+((YRES/GRID_S+3)/2)+1, 4, nyd, 107, 10, 0, 255);
				}
			}

		if (mp!=-1 && mmt>=TIMEOUT/5 && !st && my<YRES+MENUSIZE-25)
		{
			gi = mp % GRID_X;
			gj = mp / GRID_X;
			if (is_p1)
				gj += GRID_Y-GRID_P;
			gx = ((XRES/GRID_X)*gi) + (XRES/GRID_X-XRES/GRID_S)/2;
			gy = (((YRES+15)/GRID_Y)*gj) + (YRES/GRID_Y-YRES/GRID_S+10)/2 + 18;
			i = w = textwidth(search_names[mp]);
			h = YRES/GRID_Z+30;
			if (w<XRES/GRID_Z) w=XRES/GRID_Z;
			gx += XRES/(GRID_S*2)-w/2;
			gy += YRES/(GRID_S*2)-h/2;
			if (gx<2) gx=2;
			if (gx+w>=XRES-2) gx=XRES-3-w;
			if (gy<32) gy=32;
			if (gy+h>=YRES+(MENUSIZE-2)) gy=YRES+(MENUSIZE-3)-h;
			clearrect(vid_buf, gx-1, gy-2, w+3, h-1);
			drawrect(vid_buf, gx-2, gy-3, w+4, h, 160, 160, 192, 255);
			if (search_thumbs[mp]){
				if(mp != nmp && bthumb_rsdata){
					free(bthumb_rsdata);
					bthumb_rsdata = NULL;
				}
				if(!bthumb_rsdata){
					int finh, finw;
					pixel *thumb_imgdata = ptif_unpack(search_thumbs[mp], search_thsizes[mp], &finw, &finh);
					if(thumb_imgdata!=NULL){
						bthumb_rsdata = resample_img(thumb_imgdata, finw, finh, XRES/GRID_Z, YRES/GRID_Z);				
						free(thumb_imgdata);
					}
				}
				draw_image(vid_buf, bthumb_rsdata, gx+(w-(XRES/GRID_Z))/2, gy, XRES/GRID_Z, YRES/GRID_Z, 255);
				nmp = mp;
			}
			drawtext(vid_buf, gx+(w-i)/2, gy+YRES/GRID_Z+4, search_names[mp], 192, 192, 192, 255);
			drawtext(vid_buf, gx+(w-textwidth(search_owners[mp]))/2, gy+YRES/GRID_Z+16, search_owners[mp], 128, 128, 128, 255);
		}
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		ui_edit_process(mx, my, b, bq, &ed);

		if (sdl_key==SDLK_RETURN)
		{
			if (search_ids[0] && !search_ids[1])
			{
				bq = 0;
				b = 1;
				mp = 0;
			}
		}
		if (sdl_key==SDLK_ESCAPE)
		{
			strcpy(search_expr, ed.str);
			goto finish;
		}

		if (b && !bq && mx>=XRES-64+16 && mx<=XRES-8+16 && my>=8 && my<=24 && svf_login && !search_fav)
		{
			search_own = !search_own;
		}
		if (b && !bq && mx>=XRES-129+16 && mx<=XRES-65+16 && my>=8 && my<=24 && !search_fav)
		{
			search_date = !search_date;
		}
		if (b && !bq && mx>=XRES-134 && mx<=XRES-134+16 && my>=8 && my<=24 && svf_login)
		{
			search_fav = !search_fav;
			search_own = 0;
			search_date = 0;
		}

		if (b && !bq && dp!=-1)
		{
			if (search_fav){
				if(confirm_ui(vid_buf, "Remove from favorites?", search_names[dp], "Remove")){
					execute_unfav(vid_buf, search_ids[dp]);
					if (last)
					{
						free(last);
						last = NULL;
					}
				}
			} else {
				if (confirm_ui(vid_buf, "Do you want to delete?", search_names[dp], "Delete"))
				{
					execute_delete(vid_buf, search_ids[dp]);
					if (last)
					{
						free(last);
						last = NULL;
					}
				}
			}
		}
		if (b && !bq && dap!=-1)
		{
			sprintf(ed.str, "history:%s", search_ids[dap]);
		}

		if (b && !bq && tp!=-1)
		{
			strncpy(ed.str, tag_names[tp], 255);
		}

		if (b && !bq && mp!=-1 && st)
		{
			sprintf(ed.str, "user:%s", search_owners[mp]);
		}

		if (do_open==1)
		{
			mp = 0;
		}

		if ((b && !bq && mp!=-1 && !st && !uih) || do_open==1)
		{
			strcpy(search_expr, ed.str);
			if (open_ui(vid_buf, search_ids[mp], search_dates[mp]?search_dates[mp]:NULL, sdl_mod&(KMOD_CTRL|KMOD_META)) || do_open==1) {
				goto finish;
			}
		}

		if (!last)
		{
			search = 1;
		}
		else if (strcmp(last, ed.str) || last_own!=search_own || last_date!=search_date || last_page!=search_page || last_fav!=search_fav || last_p1_extra!=p1_extra)
		{
			search = 1;
			if (strcmp(last, ed.str) || last_own!=search_own || last_fav!=search_fav || last_date!=search_date)
			{
				search_page = 0;
				page_count = 0;
			}
			free(last);
			last = NULL;
		}
		else
			search = 0;

		if (search && (!saveListDownload || !saveListDownload->CheckStarted()))
		{
			std::stringstream uri;
			int start, count;
			last = mystrdup(ed.str);
			last_own = search_own;
			last_date = search_date;
			last_page = search_page;
			last_fav = search_fav;
			last_p1_extra = p1_extra;

			bool byvotes = !search_own && !search_date && !search_fav && !*last;
			if (byvotes)
			{
				if (search_page)
				{
					start = (search_page-1)*GRID_X*GRID_Y + GRID_X*GRID_P;
					count = GRID_X*GRID_Y;
				}
				else
				{
					start = 0;
					count = p1_extra ? GRID_X*GRID_Y : GRID_X*GRID_P;
				}
			}
			else
			{
				start = search_page*GRID_X*GRID_Y;
				count = GRID_X*GRID_Y;
			}
			exp_res = count; //-1 so that it can know if there is an extra save and show the next page button
			uri << "http://" << SERVER << "/Search.api?Start=" << start << "&Count=" << count+1 << "&ShowVotes=true";
			if (byvotes)
				uri << "&t=" << ((GRID_Y-GRID_P)*YRES)/(GRID_Y*14)*GRID_X; //what does this even mean? ...
			uri << "&Query=" << last;
			if (search_own)
				uri << " user:" << svf_user;
			if (search_fav)
				uri << " cat:favs";
			if (search_date)
				uri << " sort:date";

			if (!saveListDownload)
			{
				saveListDownload = new Download(uri.str(), true);
				if (svf_login)
					saveListDownload->AuthHeaders(svf_user_id, svf_session_id);
				saveListDownload->Start();
			}
			else
				saveListDownload->Reuse(uri.str());
		}

		if (saveListDownload && saveListDownload->CheckStarted() && saveListDownload->CheckDone())
		{
			int status;
			char *results = saveListDownload->Finish(NULL, &status);
			view_own = last_own;
			is_p1 = (exp_res < GRID_X*GRID_Y);
			if (status == 200)
			{
				page_count = search_results(results, last_own||svf_admin||svf_mod||(unlockedstuff&0x08));
				memset(thumb_drawn, 0, sizeof(thumb_drawn));
				memset(v_buf, 0, ((YRES+MENUSIZE)*(XRES+BARSIZE))*PIXELSIZE);
				nmp = -1;
				
				if (is_p1)
				{
					if (motdswap)
						sprintf(server_motd,"Links: \bt{a:http://powdertoy.co.uk|Powder Toy main page}\bg, \bt{a:http://powdertoy.co.uk/Discussions/Categories/Index.html|Forums}\bg, \bt{a:https://github.com/FacialTurd/The-Powder-Toy/tree/develop|TPT latest github}\bg, \bt{a:https://github.com/jacob1/The-Powder-Toy/tree/c++|Jacob1's Mod github}");
					motdswap = !motdswap;
				}
				ui_richtext_settext(server_motd, &motd);
				motd.x = (XRES-textwidth(motd.printstr))/2;
			}
			if (results)
				free(results);
			for (int i = 0; i < IMGCONNS; i++)
			{
				if (thumbnailDownloads[i] && thumbnailDownloads[i]->CheckStarted())
				{
					thumbnailDownloads[i]->Cancel();
					thumbnailDownloads[i] = NULL;
					if (img_id[i])
					{
						free(img_id[i]);
						img_id[i] = NULL;
					}
				}
			}
		}

		for (i=0; i<IMGCONNS; i++)
		{
			if (thumbnailDownloads[i] && thumbnailDownloads[i]->CheckStarted() && thumbnailDownloads[i]->CheckDone())
			{
				int status, len;
				char *thumb = thumbnailDownloads[i]->Finish(&len, &status);
				if (status != 200 || !thumb)
				{
					if (thumb)
						free(thumb);
					thumb = NULL;
				}
				else
					thumb_cache_add(img_id[i], thumb, len);
				for (pos=0; pos<GRID_X*GRID_Y; pos++) {
					if (search_dates[pos]) {
						char *id_d_temp = (char*)malloc(strlen(search_ids[pos])+strlen(search_dates[pos])+2);
						if (id_d_temp == 0)
						{
							break;
						}
						strcpy(id_d_temp, search_ids[pos]);
						strappend(id_d_temp, "_");
						strappend(id_d_temp, search_dates[pos]);
						//img_id[i] = mystrdup(id_d_temp);
						if (!strcmp(id_d_temp, img_id[i]) && !search_thumbs[pos]) {
							break;
						}
						free(id_d_temp);
					} else {
						if (search_ids[pos] && !strcmp(search_ids[pos], img_id[i])) {
							break;
						}
					}
				}
				if (thumb)
				{
					if (pos<GRID_X*GRID_Y)
					{
						search_thumbs[pos] = thumb;
						search_thsizes[pos] = len;
					}
					else
						free(thumb);
				}
				free(img_id[i]);
				img_id[i] = NULL;
			}
			if (!img_id[i] && !(saveListDownload && saveListDownload->CheckStarted()))
			{
				for (pos=0; pos<GRID_X*GRID_Y; pos++)
					if (search_ids[pos] && !search_thumbs[pos])
					{
						if (search_dates[pos])
						{
							char *id_d_temp = (char*)malloc(strlen(search_ids[pos])+strlen(search_dates[pos])+2);
							strcpy(id_d_temp, search_ids[pos]);
							strappend(id_d_temp, "_");
							strappend(id_d_temp, search_dates[pos]);
							
							for (gi=0; gi<IMGCONNS; gi++)
								if (img_id[gi] && !strcmp(id_d_temp, img_id[gi]))
									break;
									
							free(id_d_temp);
						}
						else
						{
							for (gi=0; gi<IMGCONNS; gi++)
								if (img_id[gi] && !strcmp(search_ids[pos], img_id[gi]))
									break;
						}
						if (gi<IMGCONNS)
							continue;
						break;
					}
				if (pos<GRID_X*GRID_Y)
				{
					std::stringstream uri;
					if (search_dates[pos])
					{
						std::stringstream tempID;
						tempID << search_ids[pos] << "_" << search_dates[pos];
						img_id[i] = mystrdup(tempID.str().c_str());
						uri << "http://" << STATICSERVER << "/" << search_ids[pos] << "_" << search_dates[pos] << "_small.pti";
					}
					else
					{
						img_id[i] = mystrdup(search_ids[pos]);
						uri << "http://" STATICSERVER "/" << search_ids[pos] << "_small.pti";
					}
					if (thumbnailDownloads[i] && !thumbnailDownloads[i]->CheckStarted())
					{
						thumbnailDownloads[i]->Reuse(uri.str());
					}
					else
					{
						if (thumbnailDownloads[i])
							thumbnailDownloads[i]->Cancel();
						thumbnailDownloads[i] = new Download(uri.str(), true);
						thumbnailDownloads[i]->Start();
					}
				}
			}
			if (!img_id[i] && thumbnailDownloads[i] && thumbnailDownloads[i]->CheckStarted())
			{
				thumbnailDownloads[i]->Cancel();
				thumbnailDownloads[i] = NULL;
			}
		}
	}

	strcpy(search_expr, ed.str);
finish:
	if (last)
		free(last);
	if (saveListDownload)
		saveListDownload->Cancel();
	for (int i = 0; i < IMGCONNS; i++)
	{
		if (thumbnailDownloads[i])
			thumbnailDownloads[i]->Cancel();
		if (img_id[i])
		{
			free(img_id[i]);
			img_id[i] = NULL;
		}
	}
			
	if(bthumb_rsdata){
		free(bthumb_rsdata);
		bthumb_rsdata = NULL;
	}

	search_results("", 0);

	free(v_buf);
	return 0;
}

int report_ui(pixel* vid_buf, char *save_id, bool bug)
{
	int b=1,bq,mx,my,messageHeight;
	ui_edit ed;
	char *message;
	if (bug)
		message = "Report bugs and feedback here. Do not suggest new elements or features, or report bugs with downloaded scripts.";
	else
		message = "Report saves or comments that break the rules here. Also, remember:\n"\
				  "\bw1) \bgWhen reporting stolen saves, please include the ID of the original save.\n"
				  "\bw2) \bgDo not ask for saves to be removed from front page.\n"
				  "\bw3) \bgDue to limitations you may only report each save once.";
	messageHeight = (int)(textwrapheight(message, XRES+BARSIZE-410)/2);

	ui_edit_init(&ed, 209, 159+messageHeight, (XRES+BARSIZE-400)-18, (YRES+MENUSIZE-300)-36);
	if (bug)
		ed.def = "Feedback";
	else
		ed.def = "Report details";
	ed.focus = 0;
	ed.multiline = 1;

	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	while (!sdl_poll())
	{
		fillrect(vid_buf, 200, 150-messageHeight, (XRES+BARSIZE-400), (YRES+MENUSIZE-300)+messageHeight*2, 0,0,0, 255);
		drawrect(vid_buf, 200, 150-messageHeight, (XRES+BARSIZE-400), (YRES+MENUSIZE-300)+messageHeight*2, 255, 255, 255, 255);

		drawtextwrap(vid_buf, 205, 155-messageHeight, (XRES+BARSIZE-410), 0, message, 255, 255, 255, 255);

		drawrect(vid_buf, 205, 155+messageHeight, (XRES+BARSIZE-400)-10, (YRES+MENUSIZE-300)-28, 255, 255, 255, 170);

		bq = b;
		b = mouse_get_state(&mx, &my);


		drawrect(vid_buf, 200, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 255);
		drawtext(vid_buf, 213, (YRES+MENUSIZE-150)-13+messageHeight, "Cancel", 255, 255, 255, 255);

		drawrect(vid_buf, (XRES+BARSIZE-400)+150, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 255);
		if (bug)
			drawtext(vid_buf, (XRES+BARSIZE-400)+163, (YRES+MENUSIZE-150)-13+messageHeight, "Send", 255, 255, 255, 255);
		else
			drawtext(vid_buf, (XRES+BARSIZE-400)+163, (YRES+MENUSIZE-150)-13+messageHeight, "Report", 255, 255, 255, 255);
		if (mx>(XRES+BARSIZE-400)+150 && my>(YRES+MENUSIZE-150)-18+messageHeight && mx<(XRES+BARSIZE-400)+200 && my<(YRES+MENUSIZE-150)+messageHeight)
		{
			if (b)
			{
				fillrect(vid_buf, (XRES+BARSIZE-400)+150, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 100);
			}
			else if (!b && bq == 1)
			{
				int ret = 0;
				if (bug)
					ret = execute_bug(vid_buf, ed.str);
				else
					ret = execute_report(vid_buf, save_id, ed.str);
				if (ret)
				{
					if (bug)
						info_ui(vid_buf, "Success", "Feedback has been sent");
					else
						info_ui(vid_buf, "Success", "This save has been reported");
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
				fillrect(vid_buf, (XRES+BARSIZE-400)+150, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 40);
		}
		if (mx>200 && my>(YRES+MENUSIZE-150)-18+messageHeight && mx<250 && my<(YRES+MENUSIZE-150)+messageHeight)
		{
			fillrect(vid_buf, 200, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 40);
			if (!b && bq)
				return 0;
			else if (b)
				fillrect(vid_buf, 200, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 100);
			else
				fillrect(vid_buf, 200, (YRES+MENUSIZE-150)-18+messageHeight, 50, 18, 255, 255, 255, 40);
		}
		ui_edit_draw(vid_buf, &ed);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		ui_edit_process(mx, my, b, bq, &ed);

		if (b && !bq && (mx < 200 || my < 150-messageHeight || mx > (XRES+BARSIZE-200) || my > (YRES+MENUSIZE-150)+messageHeight))
			return 0;
		if (sdl_key == SDLK_ESCAPE)
		{
			return 0;
		}
	}
	return 0;
}

void converttotime(char *timestamp, char **timestring, int show_day, int show_year, int show_time)
{
	int curr_tm_year, curr_tm_yday;
	char *tempstring = (char*)calloc(63,sizeof(char*));
	struct tm * stamptime, * currtime;
	time_t stamptime2 = atoi(timestamp), currtime2 = time(NULL);
	currtime = localtime(&currtime2);
	curr_tm_year = currtime->tm_year; curr_tm_yday = currtime->tm_yday;
	stamptime = localtime(&stamptime2);
	*timestring = (char*)calloc(63,sizeof(char*));

	if (dateformat >= 12 && (show_day || show_year))
	{
		sprintf(*timestring, "%.4i-%.2i-%.2i", stamptime->tm_year+1900, stamptime->tm_mon+1, stamptime->tm_mday);
	}
	else
	{
		if (show_day == 1 || show_day != 0 && (stamptime->tm_yday != curr_tm_yday || stamptime->tm_year != curr_tm_year)) //Different day or year, show date
		{
			if (dateformat%6 < 3) //Show weekday
			{
				sprintf(tempstring,asctime(stamptime));
				tempstring[4] = 0;
				strappend(*timestring,tempstring);
			}
			if (dateformat%3 == 1) // MM/DD/YY
			{
				sprintf(tempstring,"%i/%i",stamptime->tm_mon+1,stamptime->tm_mday);

			}
			else if (dateformat%3 == 2) // DD/MM/YY
			{
				sprintf(tempstring,"%i/%i",stamptime->tm_mday,stamptime->tm_mon+1);
			}
			else //Ex. Sun Jul 4
			{
				//sprintf(tempstring,asctime(stamptime));
				//tempstring[7] = 0;
				strncpy(tempstring,asctime(stamptime)+4,4);
				strappend(*timestring,tempstring);
				sprintf(tempstring,"%i",stamptime->tm_mday);
			}
			strappend(*timestring,tempstring);
		}
		if (show_year == 1 || show_year != 0 && stamptime->tm_year != curr_tm_year) //Show year
		{
			if (dateformat%3 != 0)
			{
				sprintf(tempstring,"/%i",(stamptime->tm_year+1900)%100);
				strappend(*timestring,tempstring);
			}
			else
			{
				sprintf(tempstring," %i",stamptime->tm_year+1900);
				strappend(*timestring,tempstring);
			}
		}
	}
	if (show_time == 1 || show_time != 0 && (dateformat < 6 || dateformat == 12 || (stamptime->tm_yday == curr_tm_yday && stamptime->tm_year == curr_tm_year))) //Show time
	{
		int hour = stamptime->tm_hour%12;
		char *ampm = "AM";
		if (stamptime->tm_hour > 11)
			ampm = "PM";
		if (hour == 0)
			hour = 12;
		sprintf(tempstring,"%s%i:%.2i:%.2i %s",strlen(*timestring)==0?"":" ",hour,stamptime->tm_min,stamptime->tm_sec,ampm);
		strappend(*timestring,tempstring);
	}
	free(tempstring);
	//strncpy(*timestring, asctime(stamptime), 63);
}

int open_ui(pixel *vid_buf, char *save_id, char *save_date, int instant_open)
{
	int b=1,bq,mx,my,cc=0,ccy=0,cix=0;
	int hasdrawninfo=0,hasdrawncthumb=0,hasdrawnthumb=0,authoritah=0,myown=0,queue_open=0,data_size=0,full_thumb_data_size=0,retval=0,bc=255,openable=1;
	int comment_scroll = 0, comment_page = 0, redraw_comments = 1, dofocus = 0, disable_scrolling = 0;
	int nyd,nyu,lv;
	float ryf, scroll_velocity = 0.0f;

	void *data = NULL;
	save_info *info = (save_info*)calloc(sizeof(save_info), 1);
	int lasttime = TIMEOUT, saveTotal, saveDone, infoTotal, infoDone, downloadDone, downloadTotal;
	int info_ready = 0, data_ready = 0, thumb_data_ready = 0;
	pixel *save_pic = NULL;
	pixel *save_pic_thumb = NULL;
	char *thumb_data = NULL;
	char viewcountbuffer[11];
	int thumb_data_size = 0;
	ui_edit ed;
	ui_copytext ctb;

	const char *profileToOpen = "";

	pixel *old_vid=(pixel *)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	if (!old_vid || !info)
		return 0;
	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	viewcountbuffer[0] = 0;

	fillrect(vid_buf, 50, 50, XRES+BARSIZE-100, YRES+MENUSIZE-100, 0, 0, 0, 255);
	drawrect(vid_buf, 50, 50, XRES+BARSIZE-100, YRES+MENUSIZE-100, 255, 255, 255, 255);
	drawrect(vid_buf, 50, 50, (XRES/2)+1, (YRES/2)+1, 255, 255, 255, 155);
	drawrect(vid_buf, 50+(XRES/2)+1, 50, XRES+BARSIZE-100-((XRES/2)+1), YRES+MENUSIZE-100, 155, 155, 155, 255);
	drawtext(vid_buf, 50+(XRES/4)-textwidth("Loading...")/2, 50+(YRES/4), "Loading...", 255, 255, 255, 128);

	ui_edit_init(&ed, 57+(XRES/2)+1, YRES+MENUSIZE-83, XRES+BARSIZE-114-((XRES/2)+1), 14);
	ed.def = "Add comment";
	ed.focus = svf_login?1:0;
	ed.multiline = 1;
	ed.resizable = 1;
	ed.limit = 1023;

	ctb.x = 100;
	ctb.y = YRES+MENUSIZE-20;
	ctb.width = textwidth(save_id)+12;
	ctb.height = 10+7;
	ctb.hover = 0;
	ctb.state = 0;
	strcpy(ctb.text, save_id);

	memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	
	//Try to load the thumbnail from the cache
	bool thumbFound;
	if(save_date)
	{
		char * id_d_temp = (char*)malloc(strlen(save_id)+strlen(save_date)+2);
		strcpy(id_d_temp, save_id);
		strappend(id_d_temp, "_");
		strappend(id_d_temp, save_date);
		
		thumbFound = thumb_cache_find(id_d_temp, (void**)(&thumb_data), &thumb_data_size);
		free(id_d_temp);
	}
	else
	{
		thumbFound = thumb_cache_find(save_id, (void**)(&thumb_data), &thumb_data_size);
	}
	if (!thumbFound)
	{
		thumb_data = NULL;	
	}
	else
	{
		//We found a thumbnail in the cache, we'll draw this one while we wait for the full image to load.
		int finw, finh;
		pixel *thumb_imgdata = ptif_unpack(thumb_data, thumb_data_size, &finw, &finh);
		if(thumb_imgdata!=NULL){
			save_pic_thumb = resample_img(thumb_imgdata, finw, finh, XRES/2, YRES/2);
			//draw_image(vid_buf, save_pic_thumb, 51, 51, XRES/2, YRES/2, 255);	
		}
		free(thumb_imgdata);
		//rescale_img(full_save, imgw, imgh, &thumb_w, &thumb_h, 2);
	}

	Download *saveDataDownload, *saveInfoDownload, *thumbnailDownload = NULL, *commentsDownload = NULL;
	//Begin Async loading of data
	if (save_date)
	{
		// We're loading a historical save
		std::stringstream uri;
		uri << "http://" << STATICSERVER << "/" << save_id << "_" << save_date << ".cps";
		saveDataDownload = new Download(uri.str());

		uri.str("");
		uri << "http://" << STATICSERVER << "/" << save_id << "_" << save_date << ".info";
		saveInfoDownload = new Download(uri.str());

		if (!instant_open)
		{
			uri.str("");
			uri << "http://" << STATICSERVER << "/" << save_id << "_" << save_date << "_large.pti";
			thumbnailDownload = new Download(uri.str());
		}
	}
	else
	{
		//We're loading a normal save
		std::stringstream uri;
		uri << "http://" << STATICSERVER << "/" << save_id << ".cps";
		saveDataDownload = new Download(uri.str());

		uri.str("");
		uri << "http://" << STATICSERVER << "/" << save_id << ".info";
		saveInfoDownload = new Download(uri.str());

		if (!instant_open)
		{
			uri.str("");
			uri << "http://" << STATICSERVER << "/" << save_id << "_large.pti";
			thumbnailDownload = new Download(uri.str());
		}
	}
	//authenticate requests
	if (svf_login)
	{
		saveDataDownload->AuthHeaders(svf_user_id, svf_session_id);
		saveInfoDownload->AuthHeaders(svf_user_id, svf_session_id);
	}
	saveDataDownload->Start();
	saveInfoDownload->Start();
	if (!instant_open)
	{
		std::stringstream uri;
		uri << "http://" << SERVER << "/Browse/Comments.json?ID=" << save_id << "&Start=0&Count=20";
		commentsDownload = new Download(uri.str(), true);
		commentsDownload->Start();

		thumbnailDownload->Start();
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);
		if (b == 1)
			redraw_comments = 1;

		if (saveDataDownload)
		{
			if (saveDataDownload->CheckDone())
			{
				int imgh, imgw, status;
				data = saveDataDownload->Finish(&data_size, &status);
				saveDone = saveTotal = data_size;
				if (status == 200)
				{
					pixel *full_save;
					if (!data || !data_size)
					{
						error_ui(vid_buf, 0, "Save data is empty (may be corrupt)");
						break;
					}
					full_save = prerender_save(data, data_size, &imgw, &imgh);
					if (full_save)
					{
						//save_pic = rescale_img(full_save, imgw, imgh, &thumb_w, &thumb_h, 2);
						data_ready = 1;
						free(full_save);
					}
					else
					{
						error_ui(vid_buf, 0, "Save may be from a newer version");
						break;
					}
				}
				saveDataDownload = NULL;
			}
			else
				saveDataDownload->CheckProgress(&saveTotal, &saveDone);
		}
		if (saveInfoDownload)
		{
			if (saveInfoDownload->CheckDone())
			{
				int status;
				char *info_data = saveInfoDownload->Finish(&infoTotal, &status);
				infoDone = infoTotal;
				if (status == 200 || !info_data)
				{
					info_ready = info_parse((char*)info_data, info);
					sprintf(viewcountbuffer, "%d", info->downloadcount);
					if (info_ready <= 0)
					{
						error_ui(vid_buf, 0, "Save info not found");
						break;
					}
				}
				if (info_data)
					free(info_data);
				saveInfoDownload = NULL;
			}
			else
				saveInfoDownload->CheckProgress(&infoTotal, &infoDone);
		}
		if (thumbnailDownload && thumbnailDownload->CheckDone())
		{
			int imgh, imgw, status;
			char *thumb_data_full = thumbnailDownload->Finish(&full_thumb_data_size, &status);
			if (status == 200)
			{
				pixel *full_thumb;
				if (!thumb_data_full || !full_thumb_data_size)
				{
					//error_ui(vid_buf, 0, "Save data is empty (may be corrupt)");
					//break;
				}
				else
				{
					full_thumb = ptif_unpack(thumb_data_full, full_thumb_data_size, &imgw, &imgh);//prerender_save(data, data_size, &imgw, &imgh);
					if (full_thumb)
					{
						save_pic = resample_img(full_thumb, imgw, imgh, XRES/2, YRES/2);
						thumb_data_ready = 1;
						free(full_thumb);
					}
				}
			}
			if(thumb_data_full)
				free(thumb_data_full);
			thumbnailDownload = NULL;
		}
		if (commentsDownload && commentsDownload->CheckStarted() && info_ready && commentsDownload && commentsDownload->CheckDone())
		{
			int status;
			char *comment_data = commentsDownload->Finish(NULL, &status);
			if (status == 200)
			{
				cJSON *root, *commentobj, *tmpobj;
				for (int i = comment_page*20; i < comment_page*20+20 && i < NUM_COMMENTS; i++)
				{
					if (info->comments[i].str) { info->comments[i].str[0] = 0; }
					if (info->commentauthors[i]) { free(info->commentauthors[i]); info->commentauthors[i] = NULL; }
					if (info->commentauthorsunformatted[i]) { free(info->commentauthorsunformatted[i]); info->commentauthorsunformatted[i] = NULL; }
					if (info->commentauthorIDs[i]) { free(info->commentauthorIDs[i]); info->commentauthorIDs[i] = NULL; }
					if (info->commenttimestamps[i]) { free(info->commenttimestamps[i]); info->commenttimestamps[i] = NULL; }
				}
				if(comment_data && (root = cJSON_Parse((const char*)comment_data)))
				{
					if (comment_page == 0)
						info->comment_count = cJSON_GetArraySize(root);
					else
						info->comment_count += cJSON_GetArraySize(root);
					if (info->comment_count > NUM_COMMENTS)
						info->comment_count = NUM_COMMENTS;
					for (int i = comment_page*20; i < info->comment_count; i++)
					{
						commentobj = cJSON_GetArrayItem(root, i%20);
						if(commentobj){
							if((tmpobj = cJSON_GetObjectItem(commentobj, "FormattedUsername")) && tmpobj->type == cJSON_String) { info->commentauthors[i] = (char*)calloc(63,sizeof(char*)); strncpy(info->commentauthors[i], tmpobj->valuestring, 63); }
							if((tmpobj = cJSON_GetObjectItem(commentobj, "Username")) && tmpobj->type == cJSON_String) { info->commentauthorsunformatted[i] = (char*)calloc(63,sizeof(char*)); strncpy(info->commentauthorsunformatted[i], tmpobj->valuestring, 63); }
							if((tmpobj = cJSON_GetObjectItem(commentobj, "UserID")) && tmpobj->type == cJSON_String) { info->commentauthorIDs[i] = (char*)calloc(16,sizeof(char*)); strncpy(info->commentauthorIDs[i], tmpobj->valuestring, 16); }
							//if((tmpobj = cJSON_GetObjectItem(commentobj, "Gravatar")) && tmpobj->type == cJSON_String) { info->commentauthors[i] = (char*)calloc(63,sizeof(char*)); strncpy(info->commentauthors[i], tmpobj->valuestring, 63); }
							if((tmpobj = cJSON_GetObjectItem(commentobj, "Text")) && tmpobj->type == cJSON_String)  { strncpy(info->comments[i].str, tmpobj->valuestring, 1023); }
							if((tmpobj = cJSON_GetObjectItem(commentobj, "Timestamp")) && tmpobj->type == cJSON_String) { converttotime(tmpobj->valuestring, &info->commenttimestamps[i], -1, -1, -1); }
						}
					}
					cJSON_Delete(root);
					redraw_comments = 1;
				}
			}
			if (comment_data)
				free(comment_data);
			disable_scrolling = 0;
		}
		if (!instant_open)
		{
			if (save_pic_thumb!=NULL && !hasdrawncthumb) {
				draw_image(vid_buf, save_pic_thumb, 51, 51, XRES/2, YRES/2, 255);
				free(save_pic_thumb);
				save_pic_thumb = NULL;
				hasdrawncthumb = 1;
				memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
			}
			if (thumb_data_ready && !hasdrawnthumb) {
				draw_image(vid_buf, save_pic, 51, 51, XRES/2, YRES/2, 255);
				free(save_pic);
				save_pic = NULL;
				hasdrawnthumb = 1;
				memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
			}
			if (info_ready && !hasdrawninfo) {
				//Render all the save information
				cix = drawtext(vid_buf, 60, (YRES/2)+60, info->name, 255, 255, 255, 255);
				cix = drawtext(vid_buf, 60, (YRES/2)+72, "Author:", 255, 255, 255, 155);
				cix = drawtext(vid_buf, cix+4, (YRES/2)+72, info->author, 255, 255, 255, 255);
				cix = drawtext(vid_buf, cix+4, (YRES/2)+72, "Date Updated:", 255, 255, 255, 155);
				cix = drawtext(vid_buf, cix+4, (YRES/2)+72, info->date, 255, 255, 255, 255);
				if(info->downloadcount){
					drawtext(vid_buf, 48+(XRES/2)-textwidth(viewcountbuffer)-textwidth("Views:")-4, (YRES/2)+72, "Views:", 255, 255, 255, 155);
					drawtext(vid_buf, 48+(XRES/2)-textwidth(viewcountbuffer), (YRES/2)+72, viewcountbuffer, 255, 255, 255, 255);
				}
				drawtextwrap(vid_buf, 62, (YRES/2)+86, (XRES/2)-24, 0, info->description, 255, 255, 255, 200);

				//Draw the score bars
				if (info->voteup>0||info->votedown>0)
				{
					lv = (info->voteup>info->votedown)?info->voteup:info->votedown;
					lv = (lv>10)?lv:10;

					if (50>lv)
					{
						ryf = 50.0f/((float)lv);
						//if(lv<8)
						//{
						//	ry =  ry/(8-lv);
						//}
						nyu = (int)(info->voteup*ryf);
						nyd = (int)(info->votedown*ryf);
					}
					else
					{
						ryf = ((float)lv)/50.0f;
						nyu = (int)(info->voteup/ryf);
						nyd = (int)(info->votedown/ryf);
					}
					nyu = nyu>50?50:nyu;
					nyd = nyd>50?50:nyd;

					fillrect(vid_buf, 48+(XRES/2)-51, (YRES/2)+53, 52, 6, 0, 107, 10, 255);
					fillrect(vid_buf, 48+(XRES/2)-51, (YRES/2)+59, 52, 6, 107, 10, 0, 255);
					drawrect(vid_buf, 48+(XRES/2)-51, (YRES/2)+53, 52, 6, 128, 128, 128, 255);
					drawrect(vid_buf, 48+(XRES/2)-51, (YRES/2)+59, 52, 6, 128, 128, 128, 255);

					fillrect(vid_buf, 48+(XRES/2)-nyu, (YRES/2)+54, nyu, 4, 57, 187, 57, 255);
					fillrect(vid_buf, 48+(XRES/2)-nyd, (YRES/2)+60, nyd, 4, 187, 57, 57, 255);
				}

				hasdrawninfo = 1;
				myown = svf_login && !strcmp(info->author, svf_user);
				authoritah = svf_login && (!strcmp(info->author, svf_user) || svf_admin || svf_mod);
				memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
			}
			if (info_ready)// && redraw_comments) // draw the comments
			{
				int commentNum = 0;
				ccy = 0;
				info->comments[0].y = 72+comment_scroll;
				clearrect(vid_buf, 52+(XRES/2), 51, XRES+BARSIZE-100-((XRES/2)+2), YRES+MENUSIZE-101);
				for (cc=0; cc<info->comment_count; cc++)
				{
					 //Try not to draw off the screen
					if (ccy + 72 + comment_scroll<YRES+MENUSIZE-56 && info->comments[cc].str)
					{
						if (ccy+comment_scroll >= 0 && info->commentauthors[cc]) //Don't draw above the screen either
						{
							int r = 255, g = 255, bl = 255;
							if (!strcmp(info->commentauthors[cc], svf_user))
							{
								bl = 100;
							}
							else if (!strcmp(info->commentauthors[cc], info->author))
							{
								g = 100;
								bl = 100;
							}

							if (show_ids && info->commentauthorIDs[cc]) //Draw author id
							{
								drawtext(vid_buf, 265+(XRES/2)-textwidth(info->commentauthorIDs[cc]), ccy+60+comment_scroll, info->commentauthorIDs[cc], 255, 255, 0, 255);
								if (b && !bq && mx > 265+(XRES/2)-textwidth(info->commentauthorIDs[cc]) && mx < 265+(XRES/2) && my > ccy+58+comment_scroll && my < ccy+70+comment_scroll && my < YRES+MENUSIZE-76-ed.h+2)
									show_ids = 0;
							}
							else if (info->commenttimestamps[cc]) //, or draw timestamp
							{
								drawtext(vid_buf, 265+(XRES/2)-textwidth(info->commenttimestamps[cc]), ccy+60+comment_scroll, info->commenttimestamps[cc], 255, 255, 0, 255);
								if (b && !bq && mx > 265+(XRES/2)-textwidth(info->commenttimestamps[cc]) && mx < 265+(XRES/2) && my > ccy+58+comment_scroll && my < ccy+70+comment_scroll && my < YRES+MENUSIZE-76-ed.h+2)
									show_ids = 1;
							}
							drawtext(vid_buf, 61+(XRES/2), ccy+60+comment_scroll, info->commentauthors[cc], r, g, bl, 255); //Draw author

							if (b && !bq && mx > 61+(XRES/2) && mx < 61+(XRES/2)+textwidth(info->commentauthors[cc]) && my > ccy+58+comment_scroll && my < ccy+70+comment_scroll && my < YRES+MENUSIZE-76-ed.h+2)
								if (sdl_mod & (KMOD_CTRL|KMOD_META)) //open profile
								{
									/*char link[128];
									strcpy(link, "http://" SERVER "/User.html?Name=");
									strcaturl(link, info->commentauthorsunformatted[cc]);
									open_link(link);*/
									profileToOpen = info->commentauthorsunformatted[cc];
								}
								else if (sdl_mod & KMOD_SHIFT) //, or search for a user's saves
								{
									sprintf(search_expr,"user:%s", info->commentauthorsunformatted[cc]);
									search_own = 0;
									search_ui(vid_buf);
									retval = 1;
									goto finish;
								}
								else //copy name to comment box
								{
									if (strlen(ed.str) + strlen(info->commentauthorsunformatted[cc]) < 1023)
									{
										strappend(ed.str, info->commentauthorsunformatted[cc]);
										strappend(ed.str, ": ");
										dofocus = 1;
									}
								}
						}

						ccy += 12;
						if (ccy + 72 + comment_scroll<YRES+MENUSIZE-56) // Check again if the comment is off the screen, incase the author line made it too long
						{
							int change, commentboxy = YRES+MENUSIZE-70-ed.h+2-5;
							if (ccy+comment_scroll < 0) // if above screen set height to negative, how long until it can start being drawn
								info->comments[cc].maxHeight = ccy+comment_scroll-10;
							else                        // else set how much can be drawn until it goes off the screen
								info->comments[cc].maxHeight = YRES+MENUSIZE-41 - (ccy + 72 + comment_scroll);

							change = ui_label_draw(vid_buf, &info->comments[cc]); // draw the comment
							ui_label_process(mx, my, b, bq, &info->comments[cc]); // process copying

							if (svf_login && b && !bq && mx > 50+(XRES/2)+1 && mx < 50 + XRES+BARSIZE-100 && my > commentboxy - 2 && my < commentboxy + ed.h+2) // defocus comments that are under textbox
								info->comments[cc].focus = info->comments[cc].cursor = info->comments[cc].cursorstart = info->comments[cc].numClicks = 0;

							ccy += change + 10;
							if (cc < NUM_COMMENTS-1)
								info->comments[cc+1].y = info->comments[cc].y + change + 22;

							if (ccy+comment_scroll < 50 && cc == info->comment_count-1 && commentsDownload->CheckStarted()) // disable scrolling until more comments have loaded
							{
								disable_scrolling = 1;
								scroll_velocity = 0.0f;
							}
							if (ccy+comment_scroll < 0 && cc == info->comment_count-1 && !commentsDownload->CheckStarted()) // reset to top of comments
							{
								comment_scroll = 0;
								scroll_velocity = 0.0f;
							}

							//draw the line that separates comments
							if (ccy+52+comment_scroll<YRES+MENUSIZE-56 && ccy+comment_scroll>-2)
							{
								draw_line(vid_buf, 50+(XRES/2)+2, ccy+52+comment_scroll, XRES+BARSIZE-51, ccy+52+comment_scroll, (cc == info->comment_count-1 && (info->comment_count%20))?175:100, 100, 100, XRES+BARSIZE);
							}
						}
					}
					else
					{
						if (!commentNum)
							commentNum = cc;
						break;
					}
					if (cc == info->comment_count-1 && !commentsDownload->CheckStarted() && comment_page < NUM_COMMENTS/20 && !(info->comment_count%20))
					{
						std::stringstream uri;
						uri << "http://" << SERVER << "/Browse/Comments.json?ID=" << save_id << "&Start=" << (comment_page+1)*20 << "&Count=20";
						commentsDownload->Reuse(uri.str());

						comment_page++;
					}
				}

				if (!commentNum)
					commentNum = info->comment_count;
				char pageText[128];
				sprintf(pageText, "Page %i of %i%s", commentNum/20+1, info->comment_count/20+1, (info->comment_count%20)?"":"+");
				drawtext(vid_buf, XRES+BARSIZE-190, YRES+MENUSIZE-43, pageText, 255, 255, 255, 255);

				//memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
				redraw_comments = 0;
			}
			//Render the comment box.
			if (info_ready && svf_login)
			{
				fillrect(vid_buf, 51+(XRES/2), ed.y-6, XRES+BARSIZE-100-((XRES/2)+1), ed.h+25, 0, 0, 0, 255);
				drawrect(vid_buf, 51+(XRES/2), ed.y-6, XRES+BARSIZE-100-((XRES/2)+1), ed.h+25, 200, 200, 200, 255);

				if (strlen(ed.str) <= 500)
					drawrect(vid_buf, 55+(XRES/2), ed.y-3, XRES+BARSIZE-108-((XRES/2)+1), ed.h, 255, 255, 255, 200);
				else
					drawrect(vid_buf, 55+(XRES/2), ed.y-3, XRES+BARSIZE-108-((XRES/2)+1), ed.h, 255, 150, 150, 200);

				ed.y = YRES+MENUSIZE-71-ui_edit_draw(vid_buf, &ed);

				drawrect(vid_buf, XRES+BARSIZE-100, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 255);
				drawtext(vid_buf, XRES+BARSIZE-90, YRES+MENUSIZE-63, "Submit", 255, 255, 255, 255);
			}

			//Save ID text and copybox
			cix = textwidth("Save ID: ");
			cix += ctb.width;
			ctb.x = textwidth("Save ID: ")+(XRES+BARSIZE-cix)/2;
			//ctb.x =
			drawtext(vid_buf, (XRES+BARSIZE-cix)/2, YRES+MENUSIZE-15, "Save ID: ", 255, 255, 255, 255);
			ui_copytext_draw(vid_buf, &ctb);
			ui_copytext_process(mx, my, b, bq, &ctb);

			//Open Button
			bc = openable?255:150;
			drawrect(vid_buf, 50, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, bc);
			drawtext(vid_buf, 73, YRES+MENUSIZE-63, "Open", 255, 255, 255, bc);
			drawtext(vid_buf, 58, YRES+MENUSIZE-64, "\x81", 255, 255, 255, bc);
			//Fav Button
			bc = svf_login?255:150;
			drawrect(vid_buf, 100, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, bc);
			if(info->myfav && svf_login){
				drawtext(vid_buf, 122, YRES+MENUSIZE-63, "Unfav.", 255, 230, 230, bc);
			} else {
				drawtext(vid_buf, 122, YRES+MENUSIZE-63, "Fav.", 255, 255, 255, bc);
			}
			drawtext(vid_buf, 107, YRES+MENUSIZE-64, "\xCC", 255, 255, 255, bc);
			//Report Button
			bc = (svf_login && info_ready)?255:150;
			drawrect(vid_buf, 150, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, bc);
			drawtext(vid_buf, 168, YRES+MENUSIZE-63, "Report", 255, 255, 255, bc);
			drawtext(vid_buf, 155, YRES+MENUSIZE-64, "\xE4", 255, 255, 255, bc);
			//Delete Button
			bc = authoritah?255:150;
			drawrect(vid_buf, 200, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, bc);
			drawtext(vid_buf, 218, YRES+MENUSIZE-63, "Delete", 255, 255, 255, bc);
			drawtext(vid_buf, 206, YRES+MENUSIZE-64, "\xAA", 255, 255, 255, bc);
			//Open in browser button
			bc = 255;
			drawrect(vid_buf, 250, YRES+MENUSIZE-68, 107, 18, 255, 255, 255, bc);
			drawtext(vid_buf, 273, YRES+MENUSIZE-63, "Open in Browser", 255, 255, 255, bc);
			drawtext(vid_buf, 258, YRES+MENUSIZE-64, "\x81", 255, 255, 255, bc);

			//Open Button
			//if (sdl_key==SDLK_RETURN && openable) {
			//	queue_open = 1;
			//}
			if (mx > 50 && mx < 50+50 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50 && openable && !queue_open) {
				fillrect(vid_buf, 50, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 40);
				if (b && !bq) {
					//Button Clicked
					queue_open = 1;
				}
			}
			//Fav Button
			if (mx > 100 && mx < 100+50 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50 && svf_login && !queue_open) {
				fillrect(vid_buf, 100, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 40);
				if (b && !bq) {
					//Button Clicked
					if(info->myfav){
						fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
						info_box(vid_buf, "Removing from favourites...");
						execute_unfav(vid_buf, save_id);
						info->myfav = 0;
					} else {
						fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
						info_box(vid_buf, "Adding to favourites...");
						execute_fav(vid_buf, save_id);
						info->myfav = 1;
					}
				}
			}
			//Report Button
			if (mx > 150 && mx < 150+50 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50 && svf_login && info_ready && !queue_open) {
				fillrect(vid_buf, 150, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 40);
				if (b && !bq) {
					//Button Clicked
					if (report_ui(vid_buf, save_id, false)) {
						retval = 0;
						break;
					}
				}
			}
			//Delete Button
			if (mx > 200 && mx < 200+50 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50 && (authoritah || myown) && !queue_open) {
				fillrect(vid_buf, 200, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 40);
				if (b && !bq) {
					//Button Clicked
					if (myown || !info->publish) {
						if (confirm_ui(vid_buf, "Are you sure you wish to delete this?", "You will not be able recover it.", "Delete")) {
							fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
							info_box(vid_buf, "Deleting...");
							if (execute_delete(vid_buf, save_id)) {
								retval = 0;
								break;
							}
						}
					} else {
						if (confirm_ui(vid_buf, "Are you sure?", "This save will be removed from the search index.", "Remove")) {
							fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
							info_box(vid_buf, "Removing...");
							if (execute_delete(vid_buf, save_id)) {
								retval = 0;
								break;
							}
						}
					}
				}
			}
			//Open in browser button
			if (mx > 250 && mx < 250+107 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50  && !queue_open) {
				fillrect(vid_buf, 250, YRES+MENUSIZE-68, 107, 18, 255, 255, 255, 40);
				if (b && !bq)
				{
					std::stringstream browserLink;
					browserLink << "http://" << SERVER << "/Browse/View.html?ID=" << save_id;
					open_link(browserLink.str());
				}
			}
			//Submit Button
			if (mx > XRES+BARSIZE-100 && mx < XRES+BARSIZE-100+50 && my > YRES+MENUSIZE-68 && my < YRES+MENUSIZE-50 && svf_login && info_ready && !queue_open) {
				fillrect(vid_buf, XRES+BARSIZE-100, YRES+MENUSIZE-68, 50, 18, 255, 255, 255, 40+(!commentsDownload->CheckStarted() ? 0 : 1)*80);
				if (b && !bq && !commentsDownload->CheckStarted())
				{
					//Button Clicked
					fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
					info_box(vid_buf, "Submitting Comment...");
					if (!execute_submit(vid_buf, save_id, ed.str))
					{
						std::stringstream uri;
						uri << "http://" << SERVER << "/Browse/Comments.json?ID=" << save_id << "&Start=0&Count=20";
						commentsDownload->Reuse(uri.str());

						for (int i = 0; i < NUM_COMMENTS; i++)
						{
							if (info->comments[i].str) { info->comments[i].str[0] = 0; }
							if (info->commentauthors[i]) { free(info->commentauthors[i]); info->commentauthors[i] = NULL; }
							if (info->commentauthorsunformatted[i]) { free(info->commentauthorsunformatted[i]); info->commentauthorsunformatted[i] = NULL; }
							if (info->commentauthorIDs[i]) { free(info->commentauthorIDs[i]); info->commentauthorIDs[i] = NULL; }
							if (info->commenttimestamps[i]) { free(info->commenttimestamps[i]); info->commenttimestamps[i] = NULL; }
						}
						comment_page = 0;
						info->comment_count = 0;
						comment_scroll = 0;
						scroll_velocity = 0.0f;
						ed.str[0] = 0;
					}
				}
			}
			if (scroll_velocity)
			{
				comment_scroll += (int)scroll_velocity;
				if (comment_scroll > 0)
					comment_scroll = 0;
				scroll_velocity *= .95f;
				if (abs(scroll_velocity) < .5f)
					scroll_velocity = 0.0f;
			}
			if (sdl_wheel)
			{
				if (!disable_scrolling || sdl_wheel > 0)
				{
					comment_scroll += 6*sdl_wheel;
					scroll_velocity += sdl_wheel;
					if (comment_scroll > 0)
						comment_scroll = 0;
					redraw_comments = 1;
				}
			}
			if (sdl_key=='[' && !ed.focus)
			{
				comment_scroll += 10;
				if (comment_scroll > 0)
					comment_scroll = 0;
				redraw_comments = 1;
			}
			if (sdl_key==']' && !ed.focus && !disable_scrolling)
			{
				comment_scroll -= 10;
				redraw_comments = 1;
			}
			//If mouse was clicked outside of the window bounds.
			if (!(mx>50 && my>50 && mx<XRES+BARSIZE-50 && my<YRES+MENUSIZE-50) && !(mx >= ctb.x && mx <= ctb.x+ctb.width && my >= ctb.y && my <= ctb.y+ctb.height) && b && !bq && !queue_open) {
				retval = 0;
				break;
			}
		}

		//Download completion
		downloadTotal = saveTotal+infoTotal;
		downloadDone = saveDone+infoDone;
		if(downloadTotal>downloadDone)
		{
			clearrect(vid_buf, 52, (YRES/2)+38, (XRES)/2-1, 13);
			fillrect(vid_buf, 51, (YRES/2)+38, (int)((((float)XRES-2)/2.0f)*((float)downloadDone/(float)downloadTotal)), 12, 255, 200, 0, 255);
			if(((float)downloadDone/(float)downloadTotal)>0.5f)
				drawtext(vid_buf, 51+(((XRES/2)-textwidth("Downloading"))/2), (YRES/2)+40, "Downloading", 0, 0, 0, 255);
			else
				drawtext(vid_buf, 51+(((XRES/2)-textwidth("Downloading"))/2), (YRES/2)+40, "Downloading", 255, 255, 255, 255);
		}

		//User opened the save, wait until we've got all the data first...
		if (queue_open || instant_open)
		{
			if (info_ready && data_ready)
			{
				// Do Open!
				int status = parse_save(data, data_size, 1, 0, 0, bmap, vx, vy, pv, fvx, fvy, signs, parts, pmap);
				if (!status)
				{
					if(svf_last)
						free(svf_last);
					svf_last = data;
					data = NULL; //so we don't free it when returning
					svf_lsize = data_size;

					svf_open = 1;
					svf_own = svf_login && !strcmp(info->author, svf_user);
					svf_publish = info->publish && svf_login && !strcmp(info->author, svf_user);

					strcpy(svf_id, save_id);
					strcpy(svf_name, info->name);
					strcpy(svf_description, info->description);
					strncpy(svf_author, info->author, 63);
					if (info->tags)
					{
						strncpy(svf_tags, info->tags, 255);
						svf_tags[255] = 0;
					} else {
						svf_tags[0] = 0;
					}
					svf_myvote = info->myvote;
					svf_filename[0] = 0;
					svf_fileopen = 0;
					retval = 1;
					ctrlzSnapshot();
					break;
				}
				else
				{
					queue_open = 0;

					svf_open = 0;
					svf_filename[0] = 0;
					svf_fileopen = 0;
					svf_publish = 0;
					svf_own = 0;
					svf_myvote = 0;
					svf_id[0] = 0;
					svf_name[0] = 0;
					svf_description[0] = 0;
					svf_author[0] = 0;
					svf_tags[0] = 0;
					if (svf_last)
						free(svf_last);
					svf_last = NULL;
					error_ui(vid_buf, 0, "An error occurred when parsing the save");
				}
			}
			else
			{
				fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 190);
				drawtext(vid_buf, 50+(XRES/4)-textwidth("Loading...")/2, 50+(YRES/4), "Loading...", 255, 255, 255, 128);
			}
		}
		if (!info_ready || !data_ready)
		{
			info_box(vid_buf, "Loading");
		}
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		memcpy(vid_buf, old_vid, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);
		if (info_ready && svf_login) {
			ui_edit_process(mx, my, b, bq, &ed);
		}
		if (dofocus)
		{
			ed.focus = 1;
			ed.cursor = ed.cursorstart = strlen(ed.str);
			dofocus = 0;
		}

		if (sdl_key==SDLK_ESCAPE) {
			retval = 0;
			break;
		}

		if (lasttime<TIMEOUT)
			lasttime++;

		if (strcmp(profileToOpen, ""))
		{
			ProfileViewer *temp = new ProfileViewer(profileToOpen);
			delete temp;
			profileToOpen = "";
		}
	}
	//Prevent those mouse clicks being passed down.
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	finish:
	//Let the download threads finish downloads and delete the data
	if (saveDataDownload)
		saveDataDownload->Cancel();
	if (saveInfoDownload)
		saveInfoDownload->Cancel();
	if (thumbnailDownload)
		thumbnailDownload->Cancel();
	if (commentsDownload)
		commentsDownload->Cancel();
	info_parse("", info);
	free(info);
	free(old_vid);
	if (data) free(data);
	if (thumb_data) free(thumb_data);
	if (save_pic) free(save_pic);
	if (save_pic_thumb) free(save_pic_thumb);
	return retval;
}

int info_parse(char *info_data, save_info *info)
{
	int i,j;
	char *p,*q,*s;

	if (info->title) free(info->title);
	if (info->name) free(info->name);
	if (info->author) free(info->author);
	if (info->date) free(info->date);
	if (info->description) free(info->description);
	if (info->tags) free(info->tags);
	for (i=0;i<NUM_COMMENTS;i++)
	{
		if (info->commentauthors[i]) free(info->commentauthors[i]);
		if (info->commentauthorsunformatted[i]) free(info->commentauthorsunformatted[i]);
		if (info->commentauthorIDs[i]) free(info->commentauthorIDs[i]);
		if (info->commenttimestamps[i]) free(info->commenttimestamps[i]);
	}
	memset(info, 0, sizeof(save_info));
	for (i = 0; i < NUM_COMMENTS; i++)
	{
		ui_label_init(&info->comments[i], 61+(XRES/2), 0, XRES+BARSIZE-107-(XRES/2), 0);
	}

	if (!info_data || !*info_data)
		return 0;

	i = 0;
	j = 0;
	s = NULL;
	do_open = 0;
	while (1)
	{
		if (!*info_data)
			break;
		p = strchr(info_data, '\n');
		if (!p)
			p = info_data + strlen(info_data);
		else
			*(p++) = 0;

		if (!strncmp(info_data, "TITLE ", 6))
		{
			info->title = mystrdup(info_data+6);
			j++;
		}
		else if (!strncmp(info_data, "NAME ", 5))
		{
			info->name = mystrdup(info_data+5);
			j++;
		}
		else if (!strncmp(info_data, "AUTHOR ", 7))
		{
			info->author = mystrdup(info_data+7);
			j++;
		}
		else if (!strncmp(info_data, "DATE ", 5))
		{
			info->date = mystrdup(info_data+5);
			j++;
		}
		else if (!strncmp(info_data, "DESCRIPTION ", 12))
		{
			info->description = mystrdup(info_data+12);
			j++;
		}
		else if (!strncmp(info_data, "VOTEUP ", 7))
		{
			info->voteup = atoi(info_data+7);
			j++;
		}
		else if (!strncmp(info_data, "VOTEDOWN ", 9))
		{
			info->votedown = atoi(info_data+9);
			j++;
		}
		else if (!strncmp(info_data, "VOTE ", 5))
		{
			info->vote = atoi(info_data+5);
			j++;
		}
		else if (!strncmp(info_data, "MYVOTE ", 7))
		{
			info->myvote = atoi(info_data+7);
			j++;
		}
		else if (!strncmp(info_data, "DOWNLOADS ", 10))
		{
			info->downloadcount = atoi(info_data+10);
			j++;
		}
		else if (!strncmp(info_data, "MYFAV ", 6))
		{
			info->myfav = atoi(info_data+6);
			j++;
		}
		else if (!strncmp(info_data, "PUBLISH ", 8))
		{
			info->publish = atoi(info_data+8);
			j++;
		}
		else if (!strncmp(info_data, "TAGS ", 5))
		{
			info->tags = mystrdup(info_data+5);
			j++;
		}
		else if (!strncmp(info_data, "COMMENT ", 8))
		{
			if (info->comment_count>=NUM_COMMENTS) {
				info_data = p;
				continue;
			} else {
				q = strchr(info_data+8, ' ');
				*(q++) = 0;
				info->commentauthors[info->comment_count] = mystrdup(info_data+8);
				info->commentauthorsunformatted[info->comment_count] = mystrdup(info_data+8);
				strncpy(info->comments[info->comment_count].str,q, 1023);
				info->comment_count++;
			}
			j++;
		}
		info_data = p;
	}
	if (j>=8) {
		return 1;
	} else {
		return -1;
	}
}

int search_results(char *str, int votes)
{
	int i,j;
	char *p,*q,*r,*s,*vu,*vd,*pu,*sd;

	for (i=0; i<GRID_X*GRID_Y; i++)
	{
		if (search_ids[i])
		{
			free(search_ids[i]);
			search_ids[i] = NULL;
		}
		if (search_names[i])
		{
			free(search_names[i]);
			search_names[i] = NULL;
		}
		if (search_dates[i])
		{
			free(search_dates[i]);
			search_dates[i] = NULL;
		}
		if (search_owners[i])
		{
			free(search_owners[i]);
			search_owners[i] = NULL;
		}
		if (search_thumbs[i])
		{
			free(search_thumbs[i]);
			search_thumbs[i] = NULL;
			search_thsizes[i] = 0;
		}
	}
	for (j=0; j<TAG_MAX; j++)
		if (tag_names[j])
		{
			free(tag_names[j]);
			tag_names[j] = NULL;
		}
	server_motd[0] = 0;

	if (!str || !*str)
		return 0;

	i = 0;
	j = 0;
	s = NULL;
	do_open = 0;
	while (1)
	{
		if (!*str)
			break;
		p = strchr(str, '\n');
		if (!p)
			p = str + strlen(str);
		else
			*(p++) = 0;
		if (!strncmp(str, "OPEN ", 5))
		{
			do_open = 1;
			if (i>=GRID_X*GRID_Y)
				break;
			if (votes)
			{
				pu = strchr(str+5, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				s = strchr(pu, ' ');
				if (!s)
					return i;
				*(s++) = 0;
				vu = strchr(s, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			else
			{
				pu = strchr(str+5, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				vu = strchr(pu, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			if (!q)
				return i;
			*(q++) = 0;
			r = strchr(q, ' ');
			if (!r)
				return i;
			*(r++) = 0;
			search_ids[i] = mystrdup(str+5);

			search_publish[i] = atoi(pu);
			search_scoreup[i] = atoi(vu);
			search_scoredown[i] = atoi(vd);

			search_owners[i] = mystrdup(q);
			search_names[i] = mystrdup(r);

			if (s)
				search_votes[i] = atoi(s);
			thumb_cache_find(str+5, search_thumbs+i, search_thsizes+i);
			i++;
		}
		else if (!strncmp(str, "HISTORY ", 8))
		{
			char * id_d_temp = NULL;
			if (i>=GRID_X*GRID_Y)
				break;
			if (votes)
			{
				sd = strchr(str+8, ' ');
				if (!sd)
					return i;
				*(sd++) = 0;
				pu = strchr(sd, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				s = strchr(pu, ' ');
				if (!s)
					return i;
				*(s++) = 0;
				vu = strchr(s, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			else
			{
				sd = strchr(str+8, ' ');
				if (!sd)
					return i;
				*(sd++) = 0;
				pu = strchr(sd, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				vu = strchr(pu, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			if (!q)
				return i;
			*(q++) = 0;
			r = strchr(q, ' ');
			if (!r)
				return i;
			*(r++) = 0;
			search_ids[i] = mystrdup(str+8);

			search_dates[i] = mystrdup(sd);

			search_publish[i] = atoi(pu);
			search_scoreup[i] = atoi(vu);
			search_scoredown[i] = atoi(vd);

			search_owners[i] = mystrdup(q);
			search_names[i] = mystrdup(r);

			if (s)
				search_votes[i] = atoi(s);
				
			//Build thumb cache ID and find
			id_d_temp = (char*)malloc(strlen(search_ids[i])+strlen(search_dates[i])+2);
			strcpy(id_d_temp, search_ids[i]);
			strappend(id_d_temp, "_");
			strappend(id_d_temp, search_dates[i]);
			thumb_cache_find(id_d_temp, search_thumbs+i, search_thsizes+i);
			free(id_d_temp);
			
			i++;
		}
		else if (!strncmp(str, "MOTD ", 5))
		{
			strncpy(server_motd, str+5, 511);
		}
		else if (!strncmp(str, "TAG ", 4))
		{
			if (j >= TAG_MAX)
			{
				str = p;
				continue;
			}
			q = strchr(str+4, ' ');
			if (!q)
			{
				str = p;
				continue;
			}
			*(q++) = 0;
			tag_names[j] = mystrdup(str+4);
			tag_votes[j] = atoi(q);
			j++;
		}
		else
		{
			if (i>=GRID_X*GRID_Y)
				break;
			if (votes)
			{
				pu = strchr(str, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				s = strchr(pu, ' ');
				if (!s)
					return i;
				*(s++) = 0;
				vu = strchr(s, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			else
			{
				pu = strchr(str, ' ');
				if (!pu)
					return i;
				*(pu++) = 0;
				vu = strchr(pu, ' ');
				if (!vu)
					return i;
				*(vu++) = 0;
				vd = strchr(vu, ' ');
				if (!vd)
					return i;
				*(vd++) = 0;
				q = strchr(vd, ' ');
			}
			if (!q)
				return i;
			*(q++) = 0;
			r = strchr(q, ' ');
			if (!r)
				return i;
			*(r++) = 0;
			search_ids[i] = mystrdup(str);

			search_publish[i] = atoi(pu);
			search_scoreup[i] = atoi(vu);
			search_scoredown[i] = atoi(vd);

			search_owners[i] = mystrdup(q);
			search_names[i] = mystrdup(r);

			if (s)
				search_votes[i] = atoi(s);
			thumb_cache_find(str, search_thumbs+i, search_thsizes+i);
			i++;
		}
		str = p;
	}
	if (*str)
		i++;
	return i;
}

int execute_tagop(pixel *vid_buf, char *op, char *tag)
{
	int status;
	char *result;

	const char *const names[] = {"ID", "Tag", NULL};
	const char *const parts[] = {svf_id, tag, NULL};

	char *uri = (char*)malloc(strlen(SERVER)+strlen(op)+36);
	sprintf(uri, "http://" SERVER "/Tag.api?Op=%s", op);

	result = http_multipart_post(
	             uri,
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	free(uri);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 1;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return 1;
	}

	if (result && result[2])
	{
		strncpy(svf_tags, result+3, 255);
		svf_id[15] = 0;
	}

	if (result)
		free(result);
	else
		error_ui(vid_buf, 0, "Could not add tag");

	return 0;
}

int execute_save(pixel *vid_buf)
{
	int status;
	char *result;

	const char *names[] = {"Name","Description", "Data:save.bin", "Thumb:thumb.bin", "Publish", "ID", NULL};
	char *uploadparts[6];
	size_t plens[6];

	uploadparts[0] = svf_name;
	plens[0] = strlen(svf_name);
	uploadparts[1] = svf_description;
	plens[1] = strlen(svf_description);
	int len;
	uploadparts[2] = (char*)build_save(&len, 0, 0, XRES, YRES, bmap, vx, vy, pv, fvx, fvy, signs, parts);
	plens[2] = len;
	if (!uploadparts[2])
	{
		error_ui(vid_buf, 0, "Error creating save");
		return 1;
	}
	uploadparts[3] = (char*)build_thumb(&len, 1);
	plens[3] = len;
	uploadparts[4] = (char*)((svf_publish==1)?"Public":"Private");
	plens[4] = strlen((svf_publish==1)?"Public":"Private");

	if (svf_id[0])
	{
		uploadparts[5] = svf_id;
		plens[5] = strlen(svf_id);
	}
	else
		names[5] = NULL;

	result = http_multipart_post(
	             "http://" SERVER "/Save.api",
	             names, uploadparts, plens,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (svf_last)
		free(svf_last);
	svf_last = uploadparts[2];
	svf_lsize = plens[2];

	if (uploadparts[3])
		free(uploadparts[3]);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 1;
	}
	if (!result || strncmp(result, "OK", 2))
	{
		if (!result)
			result = mystrdup("Could not save - no reply from server");
		error_ui(vid_buf, 0, result);
		free(result);
		return 1;
	}

	if (result && result[2])
	{
		strncpy(svf_id, result+3, 15);
		svf_id[15] = 0;
	}

	if (!svf_id[0])
	{
		error_ui(vid_buf, 0, "No ID supplied by server");
		free(result);
		return 1;
	}

	thumb_cache_inval(svf_id);

	svf_own = 1;
	if (result)
		free(result);
	return 0;
}

int execute_delete(pixel *vid_buf, char *id)
{
	int status;
	char *result;

	const char *const names[] = {"ID", NULL};
	char *parts[1];

	parts[0] = id;

	result = http_multipart_post(
	             "http://" SERVER "/Delete.api",
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 0;
	}
	if (result && strncmp(result, "INFO: ", 6)==0)
	{
		info_ui(vid_buf, "Info", result+6);
		free(result);
		return 0;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return 0;
	}

	if (result)
		free(result);
	return 1;
}

bool ParseServerReturn(char *result, int status)
{
	if (status != 200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		return true;
	}

	std::istringstream datastream(result);
	Json::Value root;

	try
	{
		datastream >> root;
		// assume everything is fine if an empty [] is returned
		if (root.size() == 0)
		{
			return false;
		}
		int status = root["Status"].asInt();
		if (status != 1)
		{
			const char *err = root["Error"].asCString();
			error_ui(vid_buf, 0, err);
			return true;
		}
	}
	catch (std::exception &e)
	{
		if (strstr(result, "Error: ") == result)
		{
			status = atoi(result+7);
			error_ui(vid_buf, status, http_ret_text(status));
			return true;
		}
		error_ui(vid_buf, 0, "Could not read response");
		return true;
	}
	return false;
}

bool execute_submit(pixel *vid_buf, char *id, char *message)
{
	int status;
	char *result;

	std::stringstream url;
	url <<  "http://" << SERVER << "/Browse/Comments.json?ID=" << id;
	Download *comment = new Download(url.str());
	comment->AuthHeaders(svf_user_id, svf_session_id);
	comment->AddPostData(std::pair<std::string, std::string>("Comment", message));

	comment->Start();
	result = comment->Finish(NULL, &status);


	bool ret = ParseServerReturn(result, status);
	return ret;
}

int execute_report(pixel *vid_buf, char *id, char *reason)
{
	int status;
	char *result;

	const char *const names[] = {"ID", "Reason", NULL};
	const char *parts[2];

	parts[0] = id;
	parts[1] = reason;

	result = http_multipart_post(
	             "http://" SERVER "/Report.api",
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 0;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return 0;
	}

	if (result)
		free(result);
	return 1;
}

int execute_bug(pixel *vid_buf, char *feedback)
{
	int status;
	char *result;
	std::string bug = "bug=";
	bug.append(URLEncode(feedback));

	void *http = http_async_req_start(NULL, "http://" UPDATESERVER "/jacob1/bug.lua", bug.c_str(), bug.length(), 0);
	if (svf_login)
	{
		http_auth_headers(http, svf_user, NULL, NULL);
	}
	result = http_async_req_stop(http, &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 0;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return 0;
	}

	if (result)
		free(result);
	return 1;
}

void execute_fav(pixel *vid_buf, char *id)
{
	int status;
	char *result;

	const char *const names[] = {"ID", NULL};
	const char *parts[1];

	parts[0] = id;

	result = http_multipart_post(
	             "http://" SERVER "/Favourite.api",
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return;
	}

	if (result)
		free(result);
}

void execute_unfav(pixel *vid_buf, char *id)
{
	int status;
	char *result;

	const char *const names[] = {"ID", NULL};
	const char *parts[1];

	parts[0] = id;

	result = http_multipart_post(
	             "http://" SERVER "/Favourite.api?Action=Remove",
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return;
	}

	if (result)
		free(result);
}

int execute_vote(pixel *vid_buf, char *id, char *action)
{
	int status;
	char *result;

	const char *const names[] = {"ID", "Action", NULL};
	const char *parts[2];

	parts[0] = id;
	parts[1] = action;

	result = http_multipart_post(
	             "http://" SERVER "/Vote.api",
	             names, parts, NULL,
	             svf_user_id, /*svf_pass*/NULL, svf_session_id,
	             &status, NULL);

	if (status!=200)
	{
		error_ui(vid_buf, status, http_ret_text(status));
		if (result)
			free(result);
		return 0;
	}
	if (result && strncmp(result, "OK", 2))
	{
		error_ui(vid_buf, 0, result);
		free(result);
		return 0;
	}

	if (result)
		free(result);
	return 1;
}
void open_link(std::string uri)
{
#ifdef WIN
	ShellExecute(0, "OPEN", uri.c_str(), NULL, NULL, 0);
#elif MACOSX
	std::string command = "open " + uri;
	system(command.c_str());
#elif LIN
	std::string command = "xdg-open " + uri;
	system(command.c_str());
#else
	printf("Cannot open browser\n");
#endif
}

struct command_match
{
	const char *min_match;
	const char *command;
};

const static struct command_match matches [] = {
	{"dof", "dofile(\""},
	{"au", "autorun.lua\")"},
	{".lu", ".lua\")"},
	{".tx", ".txt\")"},

	{"tpt.dr", "tpt.drawtext("},
	{"tpt.set_pa", "tpt.set_pause("},
	{"tpt.to", "tpt.toggle_pause()"},
	{"tpt.set_c", "tpt.set_console("},
	{"tpt.set_pre", "tpt.set_pressure("},
	{"tpt.set_ah", "tpt.set_aheat("},
	{"tpt.set_v", "tpt.set_velocity("},
	{"tpt.set_g", "tpt.set_gravity("},
	{"tpt.reset_g", "tpt.reset_gravity_field()"},
	{"tpt.reset_v", "tpt.reset_velocity()"},
	{"tpt.reset_s", "tpt.reset_spark()"},
	{"tpt.set_w", "tpt.set_wallmap("},
	{"tpt.get_w", "tpt.get_wallmap("},
	{"tpt.set_e", "tpt.set_elecmap("},
	{"tpt.get_e", "tpt.get_elecmap("},
	{"tpt.drawp", "tpt.drawpixel("},
	{"tpt.drawr", "tpt.drawrect("},
	{"tpt.drawc", "tpt.drawcircle("},
	{"tpt.fillc", "tpt.fillcircle("},
	{"tpt.drawl", "tpt.drawline("},
	{"tpt.get_n", "tpt.get_name()"},
	{"tpt.set_s", "tpt.set_shortcuts("},
	{"tpt.de", "tpt.delete("},
	{"tpt.register_m", "tpt.register_mouseevent("},
	{"tpt.unregister_m", "tpt.unregister_mouseevent("},
	{"tpt.register_k", "tpt.register_keyevent("},
	{"tpt.unregister_k", "tpt.unregister_keyevent("},
	{"tpt.reg", "tpt.register_"},
	{"tpt.un", "tpt.unregister_"},
	{"tpt.get_nu", "tpt.get_numOfParts()"},
	{"tpt.st", "tpt.start_getPartIndex()"},
	{"tpt.ne", "tpt.next_getPartIndex()"},
	{"tpt.getP", "tpt.getPartIndex()"},
	{"tpt.ac", "tpt.active_menu("},
	{"tpt.di", "tpt.display_mode("},
	{"tpt.th", "tpt.throw_error(\""},
	{"tpt.he", "tpt.heat("},
	{"tpt.setfi", "tpt.setfire("},
	{"tpt.setd", "tpt.setdebug("},
	{"tpt.setf", "tpt.setfpscap("},
	{"tpt.gets", "tpt.getscript(\""},
	{"tpt.setw", "tpt.setwindowsize("},
	{"tpt.wa", "tpt.watertest("},
	{"tpt.sc", "tpt.screenshot("},
	{"tpt.element_", "tpt.element_func("},
	{"tpt.ele", "tpt.element("},
	{"tpt.gr", "tpt.graphics_func("},
	{"tpt.bu", "tpt.bubble("},
	{"tpt.ma", "tpt.maxframes("},
	{"tpt.ind", "tpt.indestructible("},
	{"tpt.ol", "tpt.oldmenu("},
	{"tpt.res", "tpt.reset_"},
	{"tpt.create(", "tpt.c"},
	{"tpt.log(", "tpt.l"},
	{"tpt.set_property(\"", "tpt.s"},
	{"tpt.get_property(\"", "tpt.g"},
	{"tpt.fillrect(", "tpt.f"},
	{"tpt.textwidth(", "tpt.t"},
	{"tpt.register_step(", "tpt.r"},
	{"tpt.unregister_step(", "tpt.u"},
	{"tpt.input(\"", "tpt.i"},
	{"tpt.message_box(\"", "tpt.m"},
	{"tpt.hud(", "tpt.h"},
	{"tpt.newtonian_gravity(", "tpt.n"},
	{"tpt.ambient_heat(", "tpt.a"},
	{"tpt.decorations_enable(", "tpt.d"},

	{"sim.partN", "sim.partNeighbors("},
	{"sim.partCh", "sim.partChangeType("},
	{"sim.partCr", "sim.partCreate("},
	{"sim.partI", "sim.partID("},
	{"sim.partPr", "sim.partProperty("},
	{"sim.partPo", "sim.partPosition("},
	{"sim.partK", "sim.partKill("},
	{"sim.pr", "sim.pressure("},
	{"sim.ambientH", "sim.ambientHeat("},
	{"sim.velocity", "sim.velocity"},
	{"sim.gravM", "sim.gravMap("},
	{"sim.createP", "sim.createParts("},
	{"sim.createL", "sim.createLine("},
	{"sim.createB", "sim.createBox("},
	{"sim.floodP", "sim.floodParts("},
	{"sim.createWallL", "sim.createWallLine("},
	{"sim.createWallB", "sim.createWallBox("},
	{"sim.floodW", "sim.floodWalls("},
	{"sim.toolBr", "sim.toolBrush("},
	{"sim.toolL", "sim.toolLine("},
	{"sim.toolBo", "sim.toolBox("},
	{"sim.decoBr", "sim.decoBrush("},
	{"sim.declL", "sim.decoLine("},
	{"sim.decoB", "sim.decoBox("},
	{"sim.floodD", "sim.floodDeco("},
	{"sim.decoC", "sim.decoColor("},
	{"sim.cl", "sim.clearSim("},
	{"sim.resetT", "sim.resetTemp("},
	{"sim.resetP", "sim.resetPressure("},
	{"sim.s", "sim.saveStamp("},
	{"sim.loadSt", "sim.loadStamp("},
	{"sim.del", "sim.deleteStamp("},
	{"sim.loadS", "sim.loadSave("},
	{"sim.rel", "sim.reloadSave("},
	{"sim.ge", "sim.getSaveID("},
	{"sim.ad", "sim.adjustCoords("},
	{"sim.pret", "sim.prettyPowders("},
	{"sim.gravityG", "sim.gravityGrid("},
	{"sim.ed", "sim.edgeMode("},
	{"sim.gravityM", "sim.gravityMode("},
	{"sim.ai", "sim.airMode("},
	{"sim.w", "sim.waterEqualization("},
	{"sim.ambientA", "sim.ambientAirTemp("},
	{"sim.el", "sim.elementCount("},
	{"sim.ca", "sim.can_move("},
	{"sim.parts", "sim.parts("},
	{"sim.pm", "sim.pmap("},
	{"sim.n", "sim.neighbors("},

	{"sim.p", "sim.part"},
	{"sim.a", "sim.ambient"},
	{"sim.g", "sim.grav"},
	{"sim.createW", "sim.createWall"},
	{"sim.c", "sim.create"},
	{"sim.f", "sim.flood"},
	{"sim.t", "sim.tool"},
	{"sim.d", "sim.deco"},
	{"sim.l", "sim.load"},

	{"ren.r", "ren.renderModes("},
	{"ren.di", "ren.displayModes("},
	{"ren.colou", "ren.colourMode("},
	{"ren.c", "ren.colourMode("},
	{"ren.dec", "ren.decorations("},
	{"ren.g", "ren.grid("},
	{"ren.deb", "ren.debugHUD("},

	{"fs.l", "fs.list(\""},
	{"fs.e", "fs.exists(\""},
	{"fs.i", "fs.is"},
	{"fs.isF", "fs.isFile(\""},
	{"fs.isD", "fs.isDirectory(\""},
	{"fs.ma", "fs.makeDirectory(\""},
	{"fs.r", "fs.remove"},
	{"fs.removeD", "fs.removeDirectory(\""},
	{"fs.removeF", "fs.removeFile(\""},
	{"fs.m", "fs.move(\""},
	{"fs.c", "fs.copy(\""},

	{"gfx.te", "gfx.textSize(\""},
	{"gfx.drawT", "gfx.drawText("},
	{"gfx.drawL", "gfx.drawLine("},
	{"gfx.drawR", "gfx.drawRect("},
	{"gfx.drawC", "gfx.drawCircle("},
	{"gfx.fillR", "gfx.fillRect("},
	{"gfx.fillC", "gfx.fillCircle("},
	{"gfx.getC", "gfx.getColors("},
	{"gfx.getH", "gfx.getHexColor("},
	{"gfx.to", "gfx.toolTip("},
	{"gfx.d", "gfx.draw"},
	{"gfx.f", "gfx.fill"},

	{"elem.a", "elem.allocate("},
	{"elem.e", "elem.element("},
	{"elem.p", "elem.property("},
	{"elem.f", "elem.free("},
	{"elem.l", "elem.loadDefault("},

	{"bit.tob", "bit.tobit("},
	{"bit.bn", "bit.bnot("},
	{"bit.ba", "bit.band("},
	{"bit.bx", "bit.bxor("},
	{"bit.l", "bit.lshift("},
	{"bit.rs", "bit.rshift("},
	{"bit.a", "bit.arshift("},
	{"bit.bs", "bit.bswap("},
	{"bit.toh", "bit.tohex("}
};

// limits console to only have 20 (limit) previous commands stored
void console_limit_history(int limit, command_history *commandList)
{
	if (commandList==NULL)
		return;
	console_limit_history(limit-1, commandList->prev_command);
	if (limit <= 0)
	{
		free(commandList);
		commandList = NULL;
	}
	else if (limit == 1)
		commandList->prev_command = NULL;
}

// draws and processes all the history, which are in ui_labels (which aren't very nice looking, but get the job done).
// returns if one of them is focused, to fix copying (since the textbox is usually always focused and would overwrite the copy after)
int console_draw_history(command_history *commandList, command_history *commandresultList, int limit, int divideX, int mx, int my, int b, int bq)
{
	int cc, focused = 0;
	for (cc = 0; cc < limit; cc++)
	{
		if (commandList == NULL)
			break;

		cc += (commandList->command.h/14);
		if (cc >= limit)
			break;

		ui_label_draw(vid_buf, &commandList->command);
		ui_label_process(mx, my, b, bq, &commandList->command);
		ui_label_draw(vid_buf, &commandresultList->command);
		ui_label_process(mx, my, b, bq, &commandresultList->command);
		if (commandList->command.focus || commandresultList->command.focus)
			focused = 1;

		if (commandList->prev_command == NULL)
			break;
		else
		{
			commandList = commandList->prev_command;
			commandresultList = commandresultList->prev_command;
		}
	}
	return focused;
}

// reset the locations of all the history labels when the divider is dragged or a new command added
void console_set_history_X(command_history *commandList, command_history *commandresultList, int divideX)
{
	int cc;
	for (cc = 0; ; cc++)
	{
		int commandHeight, resultHeight;
		if (commandList == NULL)
			break;

		commandHeight = drawtextwrap(vid_buf, 15, 175-(cc*14), divideX-30, 0, commandList->command.str, 0, 0, 0, 0);
		resultHeight = drawtextwrap(vid_buf, divideX+15, 175-(cc*14), XRES-divideX-30, 0, commandresultList->command.str, 0, 0, 0, 0);
		if (resultHeight > commandHeight)
			commandHeight = resultHeight;
		cc += (commandHeight/14);

		commandList->command.y = 175-(cc*14);
		commandList->command.w = divideX-30+14;
		commandList->command.h = commandHeight;
		commandresultList->command.x = divideX+15;
		commandresultList->command.y = 175-(cc*14);
		commandresultList->command.w = XRES-divideX-30+14;
		commandresultList->command.h = commandHeight;

		if (commandList->prev_command == NULL)
			break;
		else
		{
			commandList = commandList->prev_command;
			commandresultList = commandresultList->prev_command;
		}
	}
}

command_history *last_command = NULL;
command_history *last_command_result = NULL;
int divideX = XRES/2-50;
int console_ui(pixel *vid_buf)
{
	int i, mx, my, b = 0, bq, selectedCommand = -1, commandHeight = 12;
	char *match = 0, laststr[1024] = "", draggingDivindingLine = 0, focusTextbox = 1;
	pixel *old_buf = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	command_history *currentcommand = last_command;
	command_history *currentcommand_result = last_command_result;
	ui_edit ed;
	ui_edit_init(&ed, 15, 207, divideX-15, 14);
	ed.multiline = 1;
	ed.limit = 1023;
	ed.resizable = 1;
	ed.alwaysFocus = 1;

	if (!old_buf)
		return 0;
	memcpy(old_buf,vid_buf,(XRES+BARSIZE)*(YRES+MENUSIZE)*PIXELSIZE);

	console_limit_history(20, currentcommand);
	console_limit_history(20, currentcommand_result);
	console_set_history_X(currentcommand, currentcommand_result, divideX);

	for (i = 0; i < 80; i++) //make background at top slightly darker
		fillrect(old_buf, -1, -1+i, XRES+BARSIZE+1, 2, 0, 0, 0, 160-(i*2));

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);
		//everything for dragging the line in the center
		if (mx > divideX - 10 && mx < divideX + 10 && my < 202 && !bq)
		{
			if (b)
				draggingDivindingLine = 2;
			else if (!draggingDivindingLine)
				draggingDivindingLine = 1;
		}
		else if (!b)
			draggingDivindingLine = 0;
		if (draggingDivindingLine == 2)
		{
			int newLine = mx;
			if (newLine < 100)
				newLine = 100;
			if (newLine > XRES-100)
				newLine = XRES-100;
			divideX = newLine;
			ed.w = divideX - 15;
			console_set_history_X(currentcommand, currentcommand_result, divideX);
			if (!b)
				draggingDivindingLine = 1;
		}

		//draw most of the things to the screen
		memcpy(vid_buf,old_buf,(XRES+BARSIZE)*(YRES+MENUSIZE)*PIXELSIZE);
		fillrect(vid_buf, -1, -1, XRES+BARSIZE+1, 208+commandHeight, 0, 0, 0, 190);
		blend_line(vid_buf, 0, 207+commandHeight, XRES+BARSIZE-1, 207+commandHeight, 228, 228, 228, 255);
		blend_line(vid_buf, divideX, 0, divideX, 207+commandHeight, 255, 255, 255, 155+50*draggingDivindingLine);
#if defined(LUACONSOLE)
		drawtext(vid_buf, 15, 15, "Welcome to The Powder Toy console v.5 (by cracker64/jacob1, Lua enabled)", 255, 255, 255, 255);
#else
		drawtext(vid_buf, 15, 15, "Welcome to The Powder Toy console v.3 (by cracker64)", 255, 255, 255, 255);
#endif

		//draw the visible console history
		currentcommand = last_command;
		currentcommand_result = last_command_result;
		focusTextbox = !console_draw_history(currentcommand, currentcommand_result, 11, divideX, mx, my, b, bq);

		//find matches, for tab autocomplete
		if (strcmp(laststr,ed.str))
		{
			char *str = ed.str;
			for (i = 0; i < sizeof(matches)/sizeof(*matches); i++)
			{
				match = 0;
				while (strstr(str,matches[i].min_match)) //find last match
				{
					match = strstr(str,matches[i].min_match);
					str = match + 1;
				}
				if (match && !strstr(str-1,matches[i].command) && strstr(matches[i].command,match) && strlen(ed.str)-strlen(match)+strlen(matches[i].command) < 256) //if match found
				{
					break;
				}
				else
					match = 0;
			}
		}
		if (match)
			drawtext(vid_buf,ed.x+textwidth(ed.str)-textwidth(match),ed.y,matches[i].command,255,255,255,127);

		drawtext(vid_buf, 5, 207, ">", 255, 255, 255, 240);

		strncpy(laststr, ed.str, 256);
		commandHeight = ui_edit_draw(vid_buf, &ed);
		if (focusTextbox)
			ed.alwaysFocus = 1;
		else
			ed.alwaysFocus = 0;
		ui_edit_process(mx, my, b, bq, &ed);
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
#ifdef OGLR
		clearScreenNP(1.0f);
		draw_parts_fbo();
#endif
		if (sdl_key==SDLK_TAB && match)
		{
			strncpy(match,matches[i].command,strlen(matches[i].command));
			match[strlen(matches[i].command)] = '\0';
			ed.cursor = ed.cursorstart = strlen(ed.str);
		}
		if (sdl_key==SDLK_RETURN) //execute the command, create a new history item
		{
			char *command = mystrdup(ed.str);
			char *result = NULL;
#ifdef LUACONSOLE
			if (process_command_lua(vid_buf, command, &result) == -1)
#else
			if (process_command_old(vid_buf, command, &result) == -1)
#endif
			{
				free(old_buf);
				return -1;
			}

			currentcommand = (command_history*)malloc(sizeof(command_history));
			memset(currentcommand, 0, sizeof(command_history));
			currentcommand->prev_command = last_command;
			ui_label_init(&currentcommand->command, 15, 0, 0, 0);
			if (command)
			{
				strncpy(currentcommand->command.str, command, 1023);
				free(command);
			}
			else
				strcpy(currentcommand->command.str, "");
			last_command = currentcommand;

			currentcommand_result = (command_history*)malloc(sizeof(command_history));
			memset(currentcommand_result, 0, sizeof(command_history));
			currentcommand_result->prev_command = last_command_result;
			ui_label_init(&currentcommand_result->command, 0, 0, 0, 0);
			if (result)
			{
				strncpy(currentcommand_result->command.str, result, 1023);
				free(result);
			}
			else
				strcpy(currentcommand_result->command.str, "");
			last_command_result = currentcommand_result;

			console_limit_history(20, currentcommand);
			console_limit_history(20, currentcommand_result);
			console_set_history_X(currentcommand, currentcommand_result, divideX); // initialize the ui_label locations here, so they can all be changed

			strcpy(ed.str, "");
			ed.cursor = ed.cursorstart = 0;
			selectedCommand = -1;
		}
		if (sdl_key==SDLK_ESCAPE || (sdl_key==SDLK_BACKQUOTE && !(sdl_mod & (KMOD_SHIFT))) || !console_mode) // exit the console
		{
			console_mode = 0;
			free(old_buf);
			return 1;
		}
		if (sdl_key==SDLK_UP || sdl_key==SDLK_DOWN) //up / down to scroll through history
		{
			selectedCommand += sdl_key==SDLK_UP?1:-1;
			if (selectedCommand<-1)
				selectedCommand = -1;
			if (selectedCommand==-1)
			{
				strcpy(ed.str, "");
				ed.cursor = ed.cursorstart = strlen(ed.str);
			}
			else
			{
				if (last_command != NULL)
				{
					int commandLoop;
					currentcommand = last_command;
					for (commandLoop = 0; commandLoop < selectedCommand; commandLoop++) {
						if (currentcommand->prev_command==NULL)
							selectedCommand = commandLoop;
						else
							currentcommand = currentcommand->prev_command;
					}
					strncpy(ed.str, currentcommand->command.str, 1023);
					ed.cursor = ed.cursorstart = strlen(ed.str);
				}
				else
				{
					strcpy(ed.str, "tpt.load(644543)");
					ed.cursor = ed.cursorstart = strlen(ed.str);
				}
			}
		}
	}
	console_mode = 0;
	free(old_buf);
	return 1;
}

ui_edit box_R;
ui_edit box_G;
ui_edit box_B;
ui_edit box_A;

void init_colour_boxes()
{
	ui_edit_init(&box_R, 5, 264, 30, 14);
	strcpy(box_R.str, "255");
	box_R.focus = 0;

	ui_edit_init(&box_G, 40, 264, 30, 14);
	strcpy(box_G.str, "255");
	box_G.focus = 0;

	ui_edit_init(&box_B, 75, 264, 30, 14);
	strcpy(box_B.str, "255");
	box_B.focus = 0;

	ui_edit_init(&box_A, 110, 264, 30, 14);
	strcpy(box_A.str, "255");
	box_A.focus = 0;
}

void decoration_textbox_colour(ui_edit* textbox, int *colour, int *colour2)
{
	*colour = atoi((*textbox).str);
	if (*colour > 255) *colour = 255;
	if (*colour < 0) *colour = 0;
	*colour2 = *colour;
	RGB_to_HSV(currR, currG, currB, &currH, &currS, &currV);
	(*textbox).focus = 0;
}

ARGBColour decocolour = COLARGB(255, 255, 0, 0);
int currA = 255, currR = 255, currG = 0, currB = 0;
int currH = 0, currS = 255, currV = 255;
int on_left = 1, decobox_hidden = 0;

int deco_disablestuff;
void decoration_editor(pixel *vid_buf, int b, int bq, int mx, int my)
{
	pixel t;
	int i, hh, ss, vv, can_select_colour = 1;
	int cr = 255, cg = 0, cb = 0, ca = 255;
	int th = currH, ts = currS, tv = currV;
	int grid_offset_x;
	int window_offset_x;
	int onleft_button_offset_x;
	//char frametext[64];

	if (!deco_disablestuff && b) //If mouse is down, but a colour isn't already being picked
		can_select_colour = 0;
	if (!bq)
		deco_disablestuff = 0;
	currR = COLR(decocolour), currG = COLG(decocolour), currB = COLB(decocolour), currA = COLA(decocolour);

	/*for (i = 0; i <= parts_lastActiveIndex; i++)
		if (parts[i].type == PT_ANIM)
		{
			parts[i].tmp2 = framenum;
		}*/

	
	ui_edit_process(mx, my, b, bq, &box_R);
	ui_edit_process(mx, my, b, bq, &box_G);
	ui_edit_process(mx, my, b, bq, &box_B);
	ui_edit_process(mx, my, b, bq, &box_A);

	if(on_left==1)
	{
		grid_offset_x = 5;
		window_offset_x = 2;
		onleft_button_offset_x = 259;
		box_R.x = 5;
		box_G.x = 40;
		box_B.x = 75;
		box_A.x = 110;
	}
	else
	{
		grid_offset_x = XRES - 274;
		window_offset_x = XRES - 279;
		onleft_button_offset_x = 4;
		box_R.x = XRES - 254 + 5;
		box_G.x = XRES - 254 + 40;
		box_B.x = XRES - 254 + 75;
		box_A.x = XRES - 254 + 110;
	}
	
	//drawrect(vid_buf, -1, -1, XRES+1, YRES+1, 220, 220, 220, 255);
	//drawrect(vid_buf, -1, -1, XRES+2, YRES+2, 70, 70, 70, 255);
	//drawtext(vid_buf, 2, 388, "Welcome to the decoration editor v.3 (by cracker64) \n\nClicking the current colour on the window will move it to the other side. Right click is eraser. ", 255, 255, 255, 255);
	//drawtext(vid_buf, 2, 388, "Welcome to the decoration editor v.4 (by cracker64/jacob1)", 255, 255, 255, 255);
	//sprintf(frametext,"Frame %i/%i",framenum+1,maxframes);
	//drawtext(vid_buf, 2, 399, frametext, 255, 255, 255, 255);

	if(!decobox_hidden)
	{
		char hex[32] = "";
		fillrect(vid_buf, window_offset_x, 2, 2+255+4+10+5, 2+255+20, 0, 0, 0, currA);
		drawrect(vid_buf, window_offset_x, 2, 2+255+4+10+5, 2+255+20, 255, 255, 255, 255);//window around whole thing

		drawrect(vid_buf, window_offset_x + onleft_button_offset_x +1, 2 +255+6, 12, 12, 255, 255, 255, 255);
		drawrect(vid_buf, window_offset_x + 230, 2 +255+6, 26, 12, 255, 255, 255, 255);
		drawtext(vid_buf, window_offset_x + 232, 2 +255+9, "Clear", 255, 255, 255, 255);
		ui_edit_draw(vid_buf, &box_R);
		ui_edit_draw(vid_buf, &box_G);
		ui_edit_draw(vid_buf, &box_B);
		ui_edit_draw(vid_buf, &box_A);
		sprintf(hex,"0x%.8X",(currA<<24)+(currR<<16)+(currG<<8)+currB);
		drawtext(vid_buf,on_left?145:504,264,hex,currR,currG,currB,currA);

		//draw colour square
		for(ss=0; ss<=255; ss++)
		{
			int lasth = -1, currh = 0;
			for(hh=0;hh<=359;hh++)
			{
				currh = clamp_flt((float)hh, 0, 359);
				if (currh == lasth)
					continue;
				lasth = currh;
				t = vid_buf[(ss+5)*(XRES+BARSIZE)+(currh+grid_offset_x)];
				cr = 0;
				cg = 0;
				cb = 0;
				HSV_to_RGB(hh,255-ss,currV,&cr,&cg,&cb);
				cr = ((currA*cr + (255-currA)*PIXR(t))>>8);
				cg = ((currA*cg + (255-currA)*PIXG(t))>>8);
				cb = ((currA*cb + (255-currA)*PIXB(t))>>8);
				vid_buf[(ss+5)*(XRES+BARSIZE)+(currh+grid_offset_x)] = PIXRGB(cr, cg, cb);
			}
		}
		//draw brightness bar
		for(vv=0; vv<=255; vv++)
			for( i=0; i<10; i++)
			{
				t = vid_buf[(vv+5)*(XRES+BARSIZE)+(i+grid_offset_x+255+4)];
				cr = 0;
				cg = 0;
				cb = 0;
				HSV_to_RGB(currH,currS,vv,&cr,&cg,&cb);
				cr = ((currA*cr + (255-currA)*PIXR(t))>>8);
				cg = ((currA*cg + (255-currA)*PIXG(t))>>8);
				cb = ((currA*cb + (255-currA)*PIXB(t))>>8);
				vid_buf[(vv+5)*(XRES+BARSIZE)+(i+grid_offset_x+255+4)] = PIXRGB(cr, cg, cb);
			}
		addpixel(vid_buf,grid_offset_x + clamp_flt((float)currH, 0, 359),5-1,255,255,255,255);
		addpixel(vid_buf,grid_offset_x -1,5+(255-currS),255,255,255,255);

		addpixel(vid_buf,grid_offset_x + clamp_flt((float)th, 0, 359),5-1,100,100,100,255);
		addpixel(vid_buf,grid_offset_x -1,5+(255-ts),100,100,100,255);

		addpixel(vid_buf,grid_offset_x + 255 +3,5+tv,100,100,100,255);
		addpixel(vid_buf,grid_offset_x + 255 +3,5 +currV,255,255,255,255);

		fillrect(vid_buf, window_offset_x + onleft_button_offset_x +1, 2 +255+6, 12, 12, currR, currG, currB, currA);
	}

	if(!box_R.focus)//prevent text update if it is being edited
		sprintf(box_R.str,"%d",currR);
	else
	{
		deco_disablestuff = 1;
		if(sdl_key == SDLK_RETURN)
		{
			decoration_textbox_colour(&box_R, &cr, &currR);
		}
	}
	if(!box_G.focus)
		sprintf(box_G.str,"%d",currG);
	else
	{
		deco_disablestuff = 1;
		if(sdl_key == SDLK_RETURN)
		{
			decoration_textbox_colour(&box_G, &cg, &currG);
		}
	}
	if(!box_B.focus)
		sprintf(box_B.str,"%d",currB);
	else
	{
		deco_disablestuff = 1;
		if(sdl_key == SDLK_RETURN)
		{
			decoration_textbox_colour(&box_B, &cb, &currB);
		}
	}
	if(!box_A.focus)
		sprintf(box_A.str,"%d",currA);
	else
	{
		deco_disablestuff = 1;
		if(sdl_key == SDLK_RETURN)
		{
			decoration_textbox_colour(&box_A, &ca, &currA);
		}
	}

	//fillrect(vid_buf, 250, YRES+4, 40, 15, currR, currG, currB, currA);

	drawrect(vid_buf, 294, YRES+1, 15, 14, 255, 255, 255, 255);
	fillrect(vid_buf, 294, YRES+1, 15, 14, currR, currG, currB, currA);
	if (decobox_hidden)
		drawtext(vid_buf, 297, YRES+5, "\xCB", 255, 255, 255, 255);
	else
		drawtext(vid_buf, 297, YRES+2, "\xCA", 255, 255, 255, 255);

	if(can_select_colour && !decobox_hidden && mx >= window_offset_x && my >= 2 && mx <= window_offset_x+255+4+10+5 && my <= 2+255+20)//in the main window
	{
		//inside brightness bar
		if(mx >= grid_offset_x +255+4 && my >= 5 && mx <= grid_offset_x+255+4+10 && my <= 5+255)
		{
			tv =  my - 5;
			if (b)
			{
				currV = my - 5;
				HSV_to_RGB(currH,currS,tv,&currR,&currG,&currB);
				deco_disablestuff = 1;
			}
			HSV_to_RGB(currH,currS,tv,&cr,&cg,&cb);
			//clearrect(vid_buf, window_offset_x + onleft_button_offset_x +1, window_offset_y +255+6,12,12);
			fillrect(vid_buf, window_offset_x + onleft_button_offset_x +1, 2 +255+6, 12, 12, cr, cg, cb, ca);
			if(!box_R.focus)
				sprintf(box_R.str,"%d",cr);
			if(!box_G.focus)
				sprintf(box_G.str,"%d",cg);
			if(!box_B.focus)
				sprintf(box_B.str,"%d",cb);
			if(!box_A.focus)
				sprintf(box_A.str,"%d",ca);
		}
		//inside colour grid
		if(mx >= grid_offset_x && my >= 5 && mx <= grid_offset_x+255 && my <= 5+255)
		{
			th = mx - grid_offset_x;
			th = (int)( th*359/255 );
			ts = 255 - (my - 5);
			if(b)
			{
				currH = th;
				currS = ts;
				HSV_to_RGB(th,ts,currV,&currR,&currG,&currB);
				deco_disablestuff = 1;
			}
			HSV_to_RGB(th,ts,currV,&cr,&cg,&cb);
			//clearrect(vid_buf, window_offset_x + onleft_button_offset_x +1, window_offset_y +255+6,12,12);
			fillrect(vid_buf, window_offset_x + onleft_button_offset_x +1, 2 +255+6, 12, 12, cr, cg, cb, ca);
			//sprintf(box_R.def,"%d",cr);
			if(!box_R.focus)
				sprintf(box_R.str,"%d",cr);
			if(!box_G.focus)
				sprintf(box_G.str,"%d",cg);
			if(!box_B.focus)
				sprintf(box_B.str,"%d",cb);
			if(!box_A.focus)
				sprintf(box_A.str,"%d",ca);
		}
		//switch side button
		if(b && !bq && mx >= window_offset_x + onleft_button_offset_x +1 && my >= 2 +255+6 && mx <= window_offset_x + onleft_button_offset_x +13 && my <= 2 +255+5 +13)
		{
			on_left = !on_left;
			deco_disablestuff = 1;
		}
		//clear button
		if(b && !bq && mx >= window_offset_x + 230 && my >= 2 +255+6 && mx <= window_offset_x + 230 +26 && my <= 2 +255+5 +13)
			if (confirm_ui(vid_buf, "Reset Decoration Layer", "Do you really want to erase everything?", "Erase") )
			{
				int i;
				for (i=0;i<NPART;i++)
					parts[i].dcolour = COLARGB(0, 0, 0, 0);
			}
			deco_disablestuff = 1;
	}
	else if (mx > XRES || my > YRES)//mouse outside normal drawing area
	{
		//hide/show button
		if (b && !bq && mx >= 294 && mx <= 295+14 && my >= YRES+1 && my<= YRES+15)
		{
			decobox_hidden = !decobox_hidden;
			zoom_en = 0;
		}
	}
	if (sdl_key==SDLK_RIGHT)
	{
		int framenum = -1;
		bool canCopy = true;
		for (int i = 0; i <= globalSim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (framenum == -1)
				{
					framenum = parts[i].tmp2+1;
					if (framenum >= globalSim->maxFrames)
					{
						canCopy = false;
						framenum = globalSim->maxFrames-1;
					}
				}
				parts[i].tmp = 0;
				parts[i].tmp2 = framenum;
				if (framenum > parts[i].ctype)
					parts[i].ctype = framenum;

				if (sdl_mod & (KMOD_CTRL|KMOD_META) && canCopy)
					parts[i].animations[framenum] = parts[i].animations[framenum-1];
			}
	}
	else if (sdl_key==SDLK_LEFT)
	{
		int framenum = -1;
		for (int i = 0; i <= globalSim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (framenum == -1)
				{
					framenum = parts[i].tmp2-1;
					if (framenum < 0)
						framenum = 0;
				}
				parts[i].tmp = 0;
				parts[i].tmp2 = framenum;
			}
	}
	else if (sdl_key==SDLK_DELETE)
	{
		int framenum = -1;
		for (int i = 0; i <= globalSim->parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				if (framenum == -1)
				{
					framenum = parts[i].tmp2;
					if (framenum < 0)
						framenum = 0;
				}
				for (int j = framenum; j < globalSim->maxFrames-1; j++)
					parts[i].animations[j] = parts[i].animations[j+1];
				if (parts[i].ctype >= framenum && parts[i].ctype)
					parts[i].ctype--;
				if (parts[i].tmp2 > parts[i].ctype)
					parts[i].tmp2 = parts[i].ctype;
			}
	}
	else if (sdl_key == SDLK_TAB)
	{
		if (box_R.focus)
		{
			box_R.focus = box_R.cursorstart = 0;
			decoration_textbox_colour(&box_R, &cr, &currR);

			box_G.focus = 1;
			box_G.cursorstart = 0;
			box_G.cursor = strlen(box_G.str);
		}
		else if (box_G.focus)
		{
			box_G.focus = box_G.cursorstart = 0;
			decoration_textbox_colour(&box_G, &cg, &currG);

			box_B.focus = 1;
			box_B.cursorstart = 0;
			box_B.cursor = strlen(box_B.str);
		}
		else if (box_B.focus)
		{
			box_B.focus = box_B.cursorstart = 0;
			decoration_textbox_colour(&box_B, &cb, &currB);

			box_A.focus = 1;
			box_A.cursorstart = 0;
			box_A.cursor = strlen(box_A.str);
		}
		else if (box_A.focus)
		{
			box_A.focus = box_A.cursorstart = 0;
			decoration_textbox_colour(&box_A, &ca, &currA);

			box_R.focus = 1;
			box_R.cursorstart = 0;
			box_R.cursor = strlen(box_R.str);
		}
	}
	if (sdl_zoom_trig)
		decobox_hidden = 1;
	/*if(sdl_key=='b' || sdl_key==SDLK_ESCAPE)
	{
		/*for (i = 0; i <= parts_lastActiveIndex; i++)
			if (parts[i].type == PT_ANIM)
			{
				parts[i].tmp2 = 0;
			}* /
		decocolour = (currA<<24)|PIXRGB(currR,currG,currB);
	}*/
	/*for (i = 0; i <= parts_lastActiveIndex; i++)
		if (parts[i].type == PT_ANIM)
		{
			parts[i].tmp2 = 0;
		}*/
	decocolour = COLARGB(currA, currR, currG, currB);
}
struct savelist_e;
typedef struct savelist_e savelist_e;
struct savelist_e {
	char *filename;
	char *name;
	pixel *image;
	savelist_e *next;
	savelist_e *prev;
};
savelist_e *get_local_saves(char *folder, char *search, int *results_ret)
{
	int results = 0;
	savelist_e *new_savelist = NULL;
	savelist_e *current_item = NULL, *new_item = NULL;
	char *fname;
	struct dirent *derp;
	DIR *directory = opendir(folder);
	if(!directory)
	{
		printf("Unable to open directory\n");
		*results_ret = 0;
		return NULL;
	}
	while(derp = readdir(directory))
	{
		fname = derp->d_name;
		if(strlen(fname)>4)
		{
			char *ext = fname+(strlen(fname)-4);
			if((!strncmp(ext, ".cps", 4) || !strncmp(ext, ".stm", 4)) && (search==NULL || strstr(fname, search)))
			{
				new_item = (savelist_e*)malloc(sizeof(savelist_e));
				new_item->filename = (char*)malloc(strlen(folder)+strlen(fname)+1);
				sprintf(new_item->filename, "%s%s", folder, fname);
				new_item->name = mystrdup(fname);
				new_item->image = NULL;
				new_item->next = NULL;
				if(new_savelist==NULL){
					new_savelist = new_item;
					new_item->prev = NULL;
				} else {
					current_item->next = new_item;
					new_item->prev = current_item;
				}
				current_item = new_item;
				results++;
			}
		}
	}
	closedir(directory);
	*results_ret = results;
	return new_savelist;
}

void free_saveslist(savelist_e *saves)
{
	if(!saves)
		return;
	if(saves->next!=NULL)
		free_saveslist(saves->next);
	if(saves->filename!=NULL)
		free(saves->filename);
	if(saves->name!=NULL)
		free(saves->name);
	if(saves->image!=NULL)
		free(saves->image);
}

int DoLocalSave(std::string savename, void *saveData, int saveDataSize, bool force)
{
#ifdef WIN
	_mkdir(LOCAL_SAVE_DIR);
#else
	mkdir(LOCAL_SAVE_DIR, 0755);
#endif

	// add .cps extension if needed
	if (savename.length() < 4 || savename.substr(savename.length()-4) != ".cps")
		savename += ".cps";

	// Actual file goes into Saves/ folder
	std::stringstream filename;
	filename << LOCAL_SAVE_DIR << PATH_SEP << savename;

	if (!force && file_exists(filename.str().c_str()))
	{
		return -1;
	}

	FILE *f = fopen(filename.str().c_str(), "wb");
	if (f)
	{
		fwrite(saveData, saveDataSize, 1, f);
		fclose(f);

		strncpy(svf_filename, savename.c_str(), 255);
		svf_fileopen = 1;

		//Allow reloading
		if (svf_last)
			free(svf_last);
		svf_last = malloc(saveDataSize);
		memcpy(svf_last, saveData, saveDataSize);
		svf_lsize = saveDataSize;
		return 0;
	}
	else
	{
		return -2;
	}
}

int save_filename_ui(pixel *vid_buf)
{
	int xsize = 16+(XRES/3);
	int ysize = 64+(YRES/3);
	float ca = 0;
	int x0=(XRES+BARSIZE-xsize)/2,y0=(YRES+MENUSIZE-ysize)/2,b=1,bq,mx,my;
	int imgw, imgh, save_size;
	void *save_data;
	char *savefname = NULL;
	char *filename = NULL;
	pixel *old_vid=(pixel *)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	pixel *save_data_image;
	pixel *save = NULL;//calloc((XRES/3)*(YRES/3), PIXELSIZE);
	ui_edit ed;

	save_data = build_save(&save_size, 0, 0, XRES, YRES, bmap, vx, vy, pv, fvx, fvy, signs, parts);
	if (!save_data)
	{
		error_ui(vid_buf, 0, "Unable to create save file");
		free(old_vid);
		return 1;
	}

	save_data_image = prerender_save(save_data, save_size, &imgw, &imgh);
	if(save_data_image!=NULL)
	{
		save = resample_img(save_data_image, imgw, imgh, XRES/3, YRES/3);
	}

	ed.x = x0+11;
	ed.y = y0+25;
	ed.w = xsize-4-16;
	ed.nx = 1;
	ed.def = "[filename]";
	ed.focus = 1;
	ed.hide = 0;
	ed.cursor = ed.cursorstart = 0;
	ed.multiline = 0;
	ed.limit = 255;
	ed.str[0] = 0;
	
	if (svf_fileopen)
	{
		char * dotloc = NULL;
		strncpy(ed.str, svf_filename, 255);
		if(dotloc = strstr(ed.str, "."))
		{
			dotloc[0] = 0;
		}
		ed.cursor = ed.cursorstart = strlen(ed.str);
	}

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	draw_rgba_image(vid_buf, (unsigned char*)save_to_disk_image, 0, 0, 0.7f);
	
	memcpy(old_vid, vid_buf, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, xsize+3, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, "Filename:", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+20, xsize-16, 16, 255, 255, 255, 180);
		if(save!=NULL)
		{
			draw_image(vid_buf, save, x0+8, y0+40, XRES/3, YRES/3, 255);
		}
		drawrect(vid_buf, x0+8, y0+40, XRES/3, YRES/3, 192, 192, 192, 255);
		
		drawrect(vid_buf, x0, y0+ysize-16, xsize, 16, 192, 192, 192, 255);
		fillrect(vid_buf, x0, y0+ysize-16, xsize, 16, 170, 170, 192, (int)ca);
		drawtext(vid_buf, x0+8, y0+ysize-12, "Save", 255, 255, 255, 255);

		ui_edit_draw(vid_buf, &ed);
		if (strlen(ed.str) || ed.focus)
			drawtext(vid_buf, x0+12+textwidth(ed.str), y0+25, ".cps", 240, 240, 255, 180);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));

		memcpy(vid_buf, old_vid, ((XRES+BARSIZE)*(YRES+MENUSIZE))*PIXELSIZE);

		ui_edit_process(mx, my, b, bq, &ed);
		
		if ((b && !bq && mx > x0 && mx < x0+xsize && my > y0+ysize-16 && my < y0+ysize) || sdl_key == SDLK_RETURN)
		{
			clean_text(ed.str,256);
			if (strlen(ed.str))
			{
				int ret = DoLocalSave(ed.str, save_data, save_size);
				if (ret == -1)
				{
					if (confirm_ui(vid_buf, "A save with that name already exists.", ed.str, "Overwrite"))
						ret = DoLocalSave(ed.str, save_data, save_size, true);
				}
				if (ret == -2)
				{
					error_ui(vid_buf, 0, "Unable to write to save file.");
				}
				break;
			}
		}

		if (b && !bq && (mx < x0 || my < y0 || mx > x0+xsize || my > y0+ysize))
			break;
		if (sdl_key == SDLK_ESCAPE)
		{
			break;
		}
	}
	
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	free(save_data_image);
	free(save_data);
	free(old_vid);
	free(save);
	if(filename) free(filename);
	if(savefname) free(savefname);
	return 0;
}

void catalogue_ui(pixel * vid_buf)
{
	int xsize = 8+(XRES/CATALOGUE_S+8)*CATALOGUE_X;
	int ysize = 48+(YRES/CATALOGUE_S+20)*CATALOGUE_Y;
	int x0=(XRES+BARSIZE-xsize)/2,y0=(YRES+MENUSIZE-ysize)/2,b=1,bq,mx,my;
	int rescount, imageoncycle = 0, currentstart = 0, currentoffset = 0, thidden = 0, cactive = 0;
	int listy = 0, listxc;
	int listx = 0, listyc;
	pixel * vid_buf2;
	float scrollvel, offsetf = 0.0f;
	char savetext[128] = "";
	char * last = mystrdup("");
	savelist_e *saves, *cssave, *csave;
	ui_edit ed;
	
	vid_buf2 = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	if (!vid_buf2)
		return;
	
	ed.w = xsize-16-4;
	ed.x = x0+11;
	ed.y = y0+29;
	ed.multiline = 0;
	ed.limit = 255;
	ed.def = "[search]";
	ed.focus = 0;
	ed.hide = 0;
	ed.cursor = ed.cursorstart = 0;
	ed.nx = 0;
	strcpy(ed.str, "");

	saves = get_local_saves(LOCAL_SAVE_DIR PATH_SEP, NULL, &rescount);
	cssave = csave = saves;
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	
	fillrect(vid_buf, -1, -1, XRES+BARSIZE, YRES+MENUSIZE, 0, 0, 0, 192);
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);
		sprintf(savetext, "Found %d save%s", rescount, rescount==1?"":"s");
		clearrect(vid_buf, x0-1, y0-1, xsize+3, ysize+3);
		clearrect(vid_buf2, x0-1, y0-1, xsize+3, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, "Saves", 255, 216, 32, 255);
		drawtext(vid_buf, x0+xsize-8-textwidth(savetext), y0+8, savetext, 255, 216, 32, 255);
		drawrect(vid_buf, x0+8, y0+24, xsize-16, 16, 255, 255, 255, 180);
		if(strcmp(ed.str, last)){
			free(last);
			last = mystrdup(ed.str);
			currentstart = 0;
			if(saves!=NULL) free_saveslist(saves);
			saves = get_local_saves(LOCAL_SAVE_DIR PATH_SEP, last, &rescount);
			cssave = saves;
			scrollvel = 0.0f;
			offsetf = 0.0f;
			thidden = 0;
		}
		//Scrolling
		if (sdl_wheel!=0)
		{
			scrollvel -= (float)sdl_wheel;
			if(scrollvel > 5.0f) scrollvel = 5.0f;
			if(scrollvel < -5.0f) scrollvel = -5.0f;
			sdl_wheel = 0;
		}
		offsetf += scrollvel;
		scrollvel*=0.99f;
		if(offsetf >= (YRES/CATALOGUE_S+20) && rescount)
		{
			if(rescount - thidden > CATALOGUE_X*(CATALOGUE_Y+1))
			{
				int i = 0;
				for(i = 0; i < CATALOGUE_X; i++){
					if(cssave->next==NULL)
						break;
					cssave = cssave->next;
				}
				offsetf -= (YRES/CATALOGUE_S+20);
				thidden += CATALOGUE_X;
			} else {
				offsetf = (YRES/CATALOGUE_S+20);
			}
		} 
		if(offsetf > 0.0f && rescount <= CATALOGUE_X*CATALOGUE_Y && rescount)
		{
			offsetf = 0.0f;
		}
		if(offsetf < 0.0f && rescount)
		{
			if(thidden >= CATALOGUE_X)
			{
				int i = 0;
				for(i = 0; i < CATALOGUE_X; i++){
					if(cssave->prev==NULL)
						break;
					cssave = cssave->prev;
				}
				offsetf += (YRES/CATALOGUE_S+20);
				thidden -= CATALOGUE_X;
			} else {
				offsetf = 0.0f;
			}
		}
		currentoffset = (int)offsetf;
		csave = cssave;
		//Diplay
		if(csave!=NULL && rescount)
		{
			listx = 0;
			listy = 0;
			while(csave!=NULL)
			{
				listxc = x0+8+listx*(XRES/CATALOGUE_S+8);
				listyc = y0+48-currentoffset+listy*(YRES/CATALOGUE_S+20);
				if(listyc > y0+ysize) //Stop when we get to the bottom of the viewable
					break;
				cactive = 0;
				if(my > listyc && my < listyc+YRES/CATALOGUE_S+2 && mx > listxc && mx < listxc+XRES/CATALOGUE_S && my > y0+48 && my < y0+ysize)
				{
					if(b)
					{
						int status, size;
						void *data;
						data = file_load(csave->filename, &size);
						if(data){
							status = parse_save(data, size, 1, 0, 0, bmap, vx, vy, pv, fvx, fvy, signs, parts, pmap);
							if(!status)
							{
								//svf_filename[0] = 0;
								strncpy(svf_filename, csave->name, 255);
								svf_fileopen = 1;
								svf_open = 0;
								svf_publish = 0;
								svf_own = 0;
								svf_myvote = 0;
								svf_id[0] = 0;
								svf_name[0] = 0;
								svf_description[0] = 0;
								svf_author[0] = 0;
								svf_tags[0] = 0;
								if (svf_last)
									free(svf_last);
								svf_last = data;
								data = NULL;
								svf_lsize = size;
								ctrlzSnapshot();
								goto openfin;
							} else {
								error_ui(vid_buf, 0, "Save data corrupt");
								free(data);
							}
						} else {
							error_ui(vid_buf, 0, "Unable to read save file");
						}
					}
					cactive = 1;
				}
				//Generate an image
				if(csave->image==NULL && !imageoncycle){ //imageoncycle: Don't read/parse more than one image per cycle, makes it more resposive for slower computers
					int imgwidth, imgheight, size;
					pixel *tmpimage = NULL;
					void *data = NULL;
					data = file_load(csave->filename, &size);
					if(data!=NULL){
						tmpimage = prerender_save(data, size, &imgwidth, &imgheight);
						if(tmpimage!=NULL)
						{
							csave->image = resample_img(tmpimage, imgwidth, imgheight, XRES/CATALOGUE_S, YRES/CATALOGUE_S);
							free(tmpimage);
						} else {
							//Blank image, TODO: this should default to something else
							csave->image = (pixel*)calloc((XRES/CATALOGUE_S)*(YRES/CATALOGUE_S), PIXELSIZE);
						}
						free(data);
					} else {
						//Blank image, TODO: this should default to something else
						csave->image = (pixel*)calloc((XRES/CATALOGUE_S)*(YRES/CATALOGUE_S), PIXELSIZE);
					}
					imageoncycle = 1;
				}
				if(csave->image!=NULL)
					draw_image(vid_buf2, csave->image, listxc, listyc, XRES/CATALOGUE_S, YRES/CATALOGUE_S, 255);
				drawrect(vid_buf2, listxc, listyc, XRES/CATALOGUE_S, YRES/CATALOGUE_S, 255, 255, 255, 190);
				if(cactive)
					drawtext(vid_buf2, listxc+((XRES/CATALOGUE_S)/2-textwidth(csave->name)/2), listyc+YRES/CATALOGUE_S+3, csave->name, 255, 255, 255, 255);
				else
					drawtext(vid_buf2, listxc+((XRES/CATALOGUE_S)/2-textwidth(csave->name)/2), listyc+YRES/CATALOGUE_S+3, csave->name, 240, 240, 255, 180);
				if (mx>=listxc+XRES/GRID_S-4 && mx<=listxc+XRES/GRID_S+6 && my>=listyc-6 && my<=listyc+4)
				{
					if (b && !bq && confirm_ui(vid_buf, "Do you want to delete?", csave->name, "Delete"))
					{
						remove(csave->filename);
						currentstart = 0;
						if(saves!=NULL) free_saveslist(saves);
						saves = get_local_saves(LOCAL_SAVE_DIR PATH_SEP, last, &rescount);
						cssave = saves;
						scrollvel = 0.0f;
						offsetf = 0.0f;
						thidden = 0;
						if (rescount == 0)
							rmdir(LOCAL_SAVE_DIR PATH_SEP);
						break;
					}
					drawtext(vid_buf2, listxc+XRES/GRID_S-4, listyc-6, "\x86", 255, 48, 32, 255);
				}
				else
					drawtext(vid_buf2, listxc+XRES/GRID_S-4, listyc-6, "\x86", 160, 48, 32, 255);
				drawtext(vid_buf2, listxc+XRES/GRID_S-4, listyc-6, "\x85", 255, 255, 255, 255);
				csave = csave->next;
				if(++listx==CATALOGUE_X){
					listx = 0;
					listy++;
				}
			}
			imageoncycle = 0;
		} else {
			drawtext(vid_buf2, x0+(xsize/2)-(textwidth("No saves found")/2), y0+(ysize/2)+20, "No saves found", 255, 255, 255, 180);
		}
		ui_edit_draw(vid_buf, &ed);
		ui_edit_process(mx, my, b, bq, &ed);
		//Draw the scrollable area onto the main buffer
		{
			pixel *srctemp = vid_buf2, *desttemp = vid_buf;
			int j = 0;
			for (j = y0+42; j < y0+ysize; j++)
			{
				memcpy(desttemp+j*(XRES+BARSIZE)+x0+1, srctemp+j*(XRES+BARSIZE)+x0+1, (xsize-1)*PIXELSIZE);
				//desttemp+=(XRES+BARSIZE);//*PIXELSIZE;
				//srctemp+=(XRES+BARSIZE);//*PIXELSIZE;
			}
		}
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		if (sdl_key==SDLK_RETURN || sdl_key==SDLK_ESCAPE)
			break;
		if (b && !bq && (mx < x0 || mx >= x0+xsize || my < y0 || my >= y0+ysize))
			break;
	}
openfin:	
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	if(saves)
		free_saveslist(saves);
	return;
}

void drawIcon(pixel * vid_buf, int x, int y, int cmode)
{
	switch (cmode)
	{
	case 0x98:
		drawtext(vid_buf, x, y, "\x98", 128, 160, 255, 255);
		break;
	case 0x99:
		drawtext(vid_buf, x, y, "\x99", 255, 212, 32, 255);
		break;
	case 0x9A:
		drawtext(vid_buf, x, y, "\x9A", 212, 212, 212, 255);
		break;
	case 0x9B:
		drawtext(vid_buf, x+1, y, "\x9B", 255, 0, 0, 255);
		drawtext(vid_buf, x+1, y, "\x9C", 255, 255, 64, 255);
		break;
	case 0xBF:
		drawtext(vid_buf, x, y, "\xBF", 55, 255, 55, 255);
		break;
	case 0xBE:
		drawtext(vid_buf, x+2, y, "\xBE", 255, 0, 0, 255);
		drawtext(vid_buf, x+2, y, "\xBD", 255, 255, 255, 255);
		break;
	case 0xC4:
		drawtext(vid_buf, x, y, "\xC4", 100, 150, 255, 255);
		break;
	case 0xD3:
		drawtext(vid_buf, x, y, "\xD3", 255, 50, 255, 255);
		break;
	case 0xE0:
		drawtext(vid_buf, x, y, "\xE0", 255, 255, 255, 255);
		break;
	case 0xE1:
		drawtext(vid_buf, x, y, "\xE1", 255, 255, 160, 255);
		break;
	case 0xDF:
		drawtext(vid_buf, x, y, "\xDF", 200, 255, 255, 255);
		break;
	case 0xDE:
		drawtext(vid_buf, x, y, "\xDE", 255, 255, 255, 255);
		break;
	case 0xDB:
		drawtext(vid_buf, x, y, "\xDB", 255, 255, 200, 255);
		break;
	case 0xD4:
		drawtext(vid_buf, x, y, "\xD4", 255, 55, 55, 255);
		drawtext(vid_buf, x, y, "\xD5", 55, 255, 55, 255);
		break;
	}
}

void render_ui(pixel * vid_buf, int xcoord, int ycoord, int orientation)
{
	pixel * o_vid_buf;
	pixel *part_vbuf; //Extra video buffer
	pixel *part_vbuf_store;
	int i, j, count, changed, temp;
	int xsize;
	int ysize;
	int yoffset;
	int xoffset;
	int xcoffset;
	int b, bq, mx, my;
	ui_checkbox *render_cb;
	ui_checkbox *display_cb;
	ui_checkbox *colour_cb;

	int render_optioncount = 8;
	const unsigned int render_options[] = {RENDER_EFFE, RENDER_GLOW, RENDER_FIRE, RENDER_BLUR, RENDER_BASC, RENDER_BLOB, RENDER_SPRK, RENDER_NONE};
	const unsigned int render_optionicons[] = {0xE1, 0xDF, 0x9B, 0xC4, 0xDB, 0xBF, 0xDF, 0xDB};
	const char * render_desc[] = {"Effects", "Glow", "Fire", "Blur", "Basic", "Blob", "Spark", "None"};

#ifdef OGLR
	int display_optioncount = 7;
	const unsigned int display_options[] = {DISPLAY_AIRC, DISPLAY_AIRP, DISPLAY_AIRV, DISPLAY_AIRH, DISPLAY_WARP, DISPLAY_PERS, DISPLAY_EFFE};
	const unsigned int display_optionicons[] = {0xD4, 0x99, 0x98, 0xBE, 0xDE, 0x9A, 0xE1};
	const char * display_desc[] = {"Air: Cracker", "Air: Pressure", "Air: Velocity", "Air: Heat", "Warp effect", "Persistent", "Effects"};
#else
	int display_optioncount = 6;
	const unsigned int display_options[] = {DISPLAY_AIRC, DISPLAY_AIRP, DISPLAY_AIRV, DISPLAY_AIRH, DISPLAY_WARP, DISPLAY_PERS};
	const unsigned int display_optionicons[] = {0xD4, 0x99, 0x98, 0xBE, 0xDE, 0x9A};
	const char * display_desc[] = {"Air: Cracker", "Air: Pressure", "Air: Velocity", "Air: Heat", "Warp effect", "Persistent"};
#endif

	int colour_optioncount = 4;
	const unsigned int colour_options[] = {COLOUR_BASC, COLOUR_LIFE, COLOUR_HEAT, COLOUR_GRAD};
	const unsigned int colour_optionicons[] = {0xDB, 0xE0, 0xBE, 0xD3};
	const char * colour_desc[] = {"Basic", "Life", "Heat", "Heat Gradient"};

	yoffset = 16;
	xoffset = 0;
	
	xcoffset = 35;
	
	xsize = xcoffset*3;
	ysize = render_optioncount * yoffset + 6;
	
	ycoord -= ysize;
	xcoord -= xsize;
	
	colour_cb = (ui_checkbox*)calloc(colour_optioncount, sizeof(ui_checkbox));
	for(i = 0; i < colour_optioncount; i++)
	{
		colour_cb[i].x = (xcoffset * 0) + xcoord + (i * xoffset) + 5;
		colour_cb[i].y = ycoord + (i * yoffset) + 5;
		colour_cb[i].focus = 0;
		colour_cb[i].checked = 0;
		j = 0;
		if(colour_mode == colour_options[i])
		{
			colour_cb[i].checked = 1;
		}
	}
	
	render_cb = (ui_checkbox*)calloc(render_optioncount, sizeof(ui_checkbox));
	for(i = 0; i < render_optioncount; i++)
	{
		render_cb[i].x = (xcoffset * 1) + xcoord + (i * xoffset) + 5;
		render_cb[i].y = ycoord + (i * yoffset) + 5;
		render_cb[i].focus = 0;
		render_cb[i].checked = 0;
		j = 0;
		while(render_modes[j])
		{
			if(render_modes[j] == render_options[i])
			{
				render_cb[i].checked = 1;
				break;
			}
			j++;
		}
	}
	
	display_cb = (ui_checkbox*)calloc(display_optioncount, sizeof(ui_checkbox));
	for(i = 0; i < display_optioncount; i++)
	{
		display_cb[i].x = (xcoffset * 2) + xcoord + (i * xoffset) + 5;
		display_cb[i].y = ycoord + (i * yoffset) + 5;
		display_cb[i].focus = 0;
		display_cb[i].checked = 0;
		j = 0;
		while(display_modes[j])
		{
			if(display_modes[j] == display_options[i])
			{
				display_cb[i].checked = 1;
				break;
			}
			j++;
		}
	}
	
	o_vid_buf = (pixel*)calloc((YRES+MENUSIZE) * (XRES+BARSIZE), PIXELSIZE);
	//memcpy(o_vid_buf, vid_buf, ((YRES+MENUSIZE) * (XRES+BARSIZE)) * PIXELSIZE);
	
	part_vbuf = (pixel*)calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE); //Extra video buffer
	part_vbuf_store = part_vbuf;
	
	if (!o_vid_buf || !part_vbuf || !display_cb || !colour_cb || !render_cb)
	{
		if(o_vid_buf)
			free(o_vid_buf);
		if(part_vbuf)
			free(part_vbuf);
		if (display_cb)
			free(display_cb);
		if (colour_cb)
			free(colour_cb);
		if (render_cb)
			free(render_cb);
		return;
	}
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	
	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);
		
		memcpy(vid_buf, o_vid_buf, ((YRES+MENUSIZE) * (XRES+BARSIZE)) * PIXELSIZE);
#ifdef OGLR
		part_vbuf = vid_buf;
#else
		if(ngrav_enable && (display_mode & DISPLAY_WARP))
		{
			part_vbuf = part_vbuf_store;
			memset(vid_buf, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
		} else {
			part_vbuf = vid_buf;
		}
#endif
		render_before(part_vbuf);
		render_after(part_vbuf, vid_buf, Point(0,0));
		if (old_menu)
		{
			for (i=0; i<SC_TOTAL; i++)
			{
				if (i < SC_FAV)
					drawchar(vid_buf, XRES+1, /*(12*i)+2*/((YRES/GetNumMenus())*i)+((YRES/GetNumMenus())/2)+5, menuSections[i]->icon, 255, 255, 255, 255);
			}
		}
		else
		{
			quickoptions_menu(vid_buf, b, bq, mx, my);
			DrawMenus(vid_buf, active_menu, my);
		}
		draw_svf_ui(vid_buf, sdl_mod & (KMOD_CTRL|KMOD_META));
		
		clearrect(vid_buf, xcoord-1, ycoord-1, xsize+3, ysize+3);
		drawrect(vid_buf, xcoord, ycoord, xsize, ysize, 192, 192, 192, 255);
		
		changed = 0;
		for(i = 0; i < render_optioncount; i++)
		{
			temp = render_cb[i].checked;
			drawIcon(vid_buf, render_cb[i].x + 16, render_cb[i].y+2, render_optionicons[i]);
			ui_checkbox_draw(vid_buf, &(render_cb[i]));
			ui_checkbox_process(mx, my, b, bq, &(render_cb[i]));
			if(render_cb[i].focus)
				drawtext(vid_buf, xcoord - textwidth(render_desc[i]) - 10, render_cb[i].y+2, render_desc[i], 255, 255, 255, 255);
			if(temp != render_cb[i].checked)
				changed = 1;
		}
		if(changed)
		{
			//Compile render options
			count = 1;
			for(i = 0; i < render_optioncount; i++)
			{
				if(render_cb[i].checked)
					count++;
			}
			free(render_modes);
			render_mode = 0;
			render_modes = (unsigned int*)calloc(count, sizeof(unsigned int));
			count = 0;
			for(i = 0; i < render_optioncount; i++)
			{
				if(render_cb[i].checked)
				{
					render_modes[count] = render_options[i];
					render_mode |= render_options[i];
					count++;
				}
			}
		}
		
		changed = 0;
		for(i = 0; i < display_optioncount; i++)
		{
			temp = display_cb[i].checked;
			drawIcon(vid_buf, display_cb[i].x + 16, display_cb[i].y+2, display_optionicons[i]);

			if(display_options[i] & DISPLAY_AIR)
			{
				ui_radio_draw(vid_buf, &(display_cb[i]));
				ui_radio_process(mx, my, b, bq, &(display_cb[i]));
			}
			else
			{
				ui_checkbox_draw(vid_buf, &(display_cb[i]));
				ui_checkbox_process(mx, my, b, bq, &(display_cb[i]));
			}
			if(display_cb[i].checked && (display_options[i] & DISPLAY_AIR))	//One air type only
			{
				for(j = 0; j < display_optioncount; j++)
				{
					if((display_options[j] & DISPLAY_AIR) && j!=i)
					{
						display_cb[j].checked = 0;
					}
				}
			}
			if(display_cb[i].focus)
				drawtext(vid_buf, xcoord - textwidth(display_desc[i]) - 10, display_cb[i].y+2, display_desc[i], 255, 255, 255, 255);
			if(temp != display_cb[i].checked)
				changed = 1;
		}
		if(changed)
		{
			//Compile display options
			count = 1;
			for(i = 0; i < display_optioncount; i++)
			{
				if(display_cb[i].checked)
					count++;
			}
			free(display_modes);
			display_mode = 0;
			display_modes = (unsigned int*)calloc(count, sizeof(unsigned int));
			count = 0;
			for(i = 0; i < display_optioncount; i++)
			{
				if(display_cb[i].checked)
				{
					display_modes[count] = display_options[i];
					display_mode |= display_options[i];
					count++;
				}
			}
		}
		
		changed = 0;
		for(i = 0; i < colour_optioncount; i++)
		{
			temp = colour_cb[i].checked;
			drawIcon(vid_buf, colour_cb[i].x + 16, colour_cb[i].y+2, colour_optionicons[i]);
			ui_radio_draw(vid_buf, &(colour_cb[i]));
			ui_radio_process(mx, my, b, bq, &(colour_cb[i]));
			if(colour_cb[i].checked)	//One colour only
			{
				for(j = 0; j < colour_optioncount; j++)
				{
					if(j!=i)
					{
						colour_cb[j].checked = 0;
					}
				}
			}
			if(colour_cb[i].focus)
				drawtext(vid_buf, xcoord - textwidth(colour_desc[i]) - 10, colour_cb[i].y+2, colour_desc[i], 255, 255, 255, 255);
			if(temp != colour_cb[i].checked)
				changed = 1;
		}
		if(changed)
		{
			//Compile colour options
			colour_mode = 0;
			for(i = 0; i < colour_optioncount; i++)
			{
				if(colour_cb[i].checked)
				{
					colour_mode |= colour_options[i];
				}
			}
		}
		
#ifdef OGLR
		clearScreenNP(1.0f);
		draw_parts_fbo();
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		
		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
		if (b && !bq && (mx < xcoord || mx > xcoord+xsize || my < ycoord || my > ycoord+ysize))
			break;
	}
	
	free(colour_cb);
	
	free(render_cb);
	
	free(display_cb);
	
	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
	
	free(part_vbuf_store);
	
	free(o_vid_buf);
}

void simulation_ui(pixel * vid_buf)
{
	int xsize = 300;
	int ysize = 340;
	int x0=(XRES-xsize)/2,y0=(YRES-ysize)/2,b=1,bq,mx,my;
	int oldedgeMode = edgeMode;
	ui_checkbox cb, cb2, cb3, cb4, cb5, cb6, cb7;
	char * airModeList[] = {"On", "Pressure Off", "Velocity Off", "Off", "No Update"};
	int airModeListCount = 5;
	char * gravityModeList[] = {"Vertical", "Off", "Radial"};
	int gravityModeListCount = 3;
	char * edgeModeList[] = {"Void", "Solid", "Loop"};//, "Empty"};
	int edgeModeListCount = 3;
	char * updateList[] = {"No Update Check", "ip: 178.219.36.155", "website: mniip.com"};//, "Empty"};
	int updateListCount = 3;
	ui_list list;
	ui_list list2;
	ui_list list3;
	ui_list listUpdate;

	cb.x = x0+xsize-16;		//Heat simulation
	cb.y = y0+23;
	cb.focus = 0;
	cb.checked = !legacy_enable;

	cb2.x = x0+xsize-16;	//Newt. Gravity
	cb2.y = y0+79;
	cb2.focus = 0;
	cb2.checked = ngrav_enable;
	
	cb3.x = x0+xsize-16;	//Large window
	cb3.y = y0+255;
	cb3.focus = 0;
	cb3.checked = (sdl_scale==2)?1:0;
	
	cb4.x = x0+xsize-16;	//Fullscreen
	cb4.y = y0+269;
	cb4.focus = 0;
	cb4.checked = (kiosk_enable==1)?1:0;
	
	cb5.x = x0+xsize-16;	//Ambient heat
	cb5.y = y0+51;
	cb5.focus = 0;
	cb5.checked = aheat_enable;

	cb6.x = x0+xsize-16;	//Water equalisation
	cb6.y = y0+107;
	cb6.focus = 0;
	cb6.checked = water_equal_test;

	cb7.x = x0+xsize-16;	//Fast Quit
	cb7.y = y0+283;
	cb7.focus = 0;
	cb7.checked = fastquit;
	
	list.x = x0+xsize-76;	//Air Mode
	list.y = y0+135;
	list.w = 72;
	list.h = 16;
	list.def = "[air mode]";
	list.selected = airMode;
	list.items = airModeList;
	list.count = airModeListCount;
	
	list2.x = x0+xsize-76;	//Gravity Mode
	list2.y = y0+163;
	list2.w = 72;
	list2.h = 16;
	list2.def = "[gravity mode]";
	list2.selected = gravityMode;
	list2.items = gravityModeList;
	list2.count = gravityModeListCount;

	list3.x = x0+xsize-76;	//Edge Mode
	list3.y = y0+191;
	list3.w = 72;
	list3.h = 16;
	list3.def = "[gravity mode]";
	list3.selected = edgeMode;
	list3.items = edgeModeList;
	list3.count = edgeModeListCount;

	listUpdate.x = x0+xsize-100;	//Edge Mode
	listUpdate.y = y0+219;
	listUpdate.w = 96;
	listUpdate.h = 16;
	listUpdate.def = "[update server]";
	listUpdate.selected = doUpdates;
	listUpdate.items = updateList;
	listUpdate.count = updateListCount;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}

	while (!sdl_poll())
	{
		bq = b;
		b = mouse_get_state(&mx, &my);

		clearrect(vid_buf, x0-1, y0-1, xsize+3, ysize+3);
		drawrect(vid_buf, x0, y0, xsize, ysize, 192, 192, 192, 255);
		drawtext(vid_buf, x0+8, y0+8, "Simulation options", 255, 216, 32, 255);

		drawtext(vid_buf, x0+8, y0+26, "Heat simulation", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Heat simulation"), y0+26, "Introduced in version 34.", 255, 255, 255, 180);
		drawtext(vid_buf, x0+12, y0+40, "Can cause odd behaviour when disabled.", 255, 255, 255, 120);
		
		drawtext(vid_buf, x0+8, y0+54, "Ambient heat simulation", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Ambient heat simulation"), y0+54, "Introduced in version 50.", 255, 255, 255, 180);
		drawtext(vid_buf, x0+12, y0+68, "Can cause odd / broken behaviour with many saves.", 255, 255, 255, 120);

		drawtext(vid_buf, x0+8, y0+82, "Newtonian gravity", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Newtonian gravity"), y0+82, "Introduced in version 48.", 255, 255, 255, 180);
		drawtext(vid_buf, x0+12, y0+96, "May also cause poor performance on single core computers", 255, 255, 255, 120);

		drawtext(vid_buf, x0+8, y0+110, "Water Equalization Test", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Water Equalization Test"), y0+110, "Introduced in version 61.", 255, 255, 255, 180);
		drawtext(vid_buf, x0+12, y0+124, "May cause poor performance with a lot of water.", 255, 255, 255, 120);
		
		drawtext(vid_buf, x0+8, y0+138, "Air Simulation Mode \bg(y)", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+152, "airMode", 255, 255, 255, 120);
		
		drawtext(vid_buf, x0+8, y0+166, "Gravity Simulation Mode \bg(w)", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+180, "gravityMode", 255, 255, 255, 120);

		drawtext(vid_buf, x0+8, y0+194, "Edge Mode", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+208, "edgeMode", 255, 255, 255, 120);

		drawtext(vid_buf, x0+8, y0+222, "Update Check", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12, y0+236, "doUpdates", 255, 255, 255, 120);
		
		draw_line(vid_buf, x0, y0+250, x0+xsize, y0+250, 150, 150, 150, XRES+BARSIZE);
		
		drawtext(vid_buf, x0+8, y0+256, "Large window", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Large window"), y0+256, "\bg- Double window size for small screens", 255, 255, 255, 180);
		
		drawtext(vid_buf, x0+8, y0+270, "Fullscreen", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Fullscreen"), y0+270, "\bg- Fill the entire screen", 255, 255, 255, 180);

		drawtext(vid_buf, x0+8, y0+284, "Fast Quit", 255, 255, 255, 255);
		drawtext(vid_buf, x0+12+textwidth("Fast Quit"), y0+284, "\bg- Hitting 'X' will always exit out of tpt", 255, 255, 255, 180);

		drawtext(vid_buf, x0+12, y0+ysize-34, "Open Data Folder", 255, 255, 255, 255);
		drawrect(vid_buf, x0+8, y0+ysize-38, 90, 16, 192, 192, 192, 255);
		drawtext(vid_buf, x0+25+textwidth("Open Data Folder"), y0+ysize-34, "\bg- Open the data and preferences folder", 255, 255, 255, 180);

		drawtext(vid_buf, x0+5, y0+ysize-11, "OK", 255, 255, 255, 255);
		drawrect(vid_buf, x0, y0+ysize-16, xsize, 16, 192, 192, 192, 255);

		ui_checkbox_draw(vid_buf, &cb);
		ui_checkbox_draw(vid_buf, &cb2);
		ui_checkbox_draw(vid_buf, &cb3);
		ui_checkbox_draw(vid_buf, &cb4);
		ui_checkbox_draw(vid_buf, &cb5);
		ui_checkbox_draw(vid_buf, &cb6);
		ui_checkbox_draw(vid_buf, &cb7);
		ui_list_draw(vid_buf, &list);
		ui_list_draw(vid_buf, &list2);
		ui_list_draw(vid_buf, &list3);
		ui_list_draw(vid_buf, &listUpdate);
#ifdef OGLR
		clearScreen(1.0f);
#endif
		sdl_blit(0, 0, (XRES+BARSIZE), YRES+MENUSIZE, vid_buf, (XRES+BARSIZE));
		ui_checkbox_process(mx, my, b, bq, &cb);
		ui_checkbox_process(mx, my, b, bq, &cb2);
		ui_checkbox_process(mx, my, b, bq, &cb3);
		ui_checkbox_process(mx, my, b, bq, &cb4);
		ui_checkbox_process(mx, my, b, bq, &cb5);
		ui_checkbox_process(mx, my, b, bq, &cb6);
		ui_checkbox_process(mx, my, b, bq, &cb7);
		ui_list_process(vid_buf, mx, my, b, &list);
		ui_list_process(vid_buf, mx, my, b, &list2);
		ui_list_process(vid_buf, mx, my, b, &list3);
		ui_list_process(vid_buf, mx, my, b, &listUpdate);

		if (((cb3.checked)?2:1) != sdl_scale || ((cb4.checked)?1:0) != kiosk_enable)
		{
			set_scale((cb3.checked)?2:1, (cb4.checked)?1:0);
		}

		if (b && !bq)
		{
			if (mx>=x0 && mx<x0+xsize && my>=y0+ysize-16 && my<=y0+ysize)
				break;
			else if (mx>=x0+8 && mx<x0+98 && my>=y0+ysize-38 && my<=y0+ysize-22)
			{
				//one of these should always be defined
#ifdef WIN
				const char* openCommand = "explorer ";
#elif MACOSX
				const char* openCommand = "open ";
//#elif LIN
#else
				const char* openCommand = "xdg-open ";
#endif
				char* workingDirectory = new char[FILENAME_MAX+strlen(openCommand)];
				sprintf(workingDirectory, "%s\"%s\"", openCommand, getcwd(NULL, 0));
				system(workingDirectory);
				delete workingDirectory;
			}
			else if (mx < x0 || my < y0 || mx > x0+xsize || my > y0+ysize)
				break;
		}

		if (sdl_key==SDLK_RETURN)
			break;
		if (sdl_key==SDLK_ESCAPE)
			break;
	}

	water_equal_test = cb6.checked;
	legacy_enable = !cb.checked;
	aheat_enable = cb5.checked;
	if(list.selected>=0 && list.selected<=4)
		airMode = list.selected;
	if(list2.selected>=0 && list2.selected<=2)
		gravityMode = list2.selected;
	if(ngrav_enable != cb2.checked)
	{
		if(cb2.checked)
			start_grav_async();
		else
			stop_grav_async();
	}
	edgeMode = (char)list3.selected;
	if(edgeMode == 1 && oldedgeMode != 1)
		draw_bframe();
	else if(edgeMode != 1 && oldedgeMode == 1)
		erase_bframe();
	doUpdates = listUpdate.selected;
	fastquit = cb7.checked;

	while (!sdl_poll())
	{
		b = mouse_get_state(&mx, &my);
		if (!b)
			break;
	}
}

int mouse_get_state(int *x, int *y)
{
	//Wrapper around SDL_GetMouseState to divide by sdl_scale
	Uint8 sdl_b;
	int sdl_x, sdl_y;
	sdl_b = SDL_GetMouseState(&sdl_x, &sdl_y);
	*x = sdl_x/sdl_scale;
	*y = sdl_y/sdl_scale;
	return sdl_b;
}

bool mouse_coords_window_to_sim(int *mouseX, int *mouseY)
{
	//adjust coords into the simulation area
	if (*mouseX < 0)
		*mouseX = 0;
	else if (*mouseX >= XRES)
		*mouseX = XRES-1;
	if (*mouseY < 0)
		*mouseY = 0;
	else if (*mouseY >= YRES)
		*mouseY = YRES-1;

	//Change mouse coords to take zoom window into account
	if (zoom_en && *mouseX >= zoom_wx && *mouseY >= zoom_wy
		&& *mouseX < (zoom_wx+ZFACTOR*ZSIZE)
		&& *mouseY < (zoom_wy+ZFACTOR*ZSIZE))
	{
		*mouseX = ((*mouseX-zoom_wx)/ZFACTOR)+zoom_x;
		*mouseY = ((*mouseY-zoom_wy)/ZFACTOR)+zoom_y;
		return true;
	}
	return false;
}
