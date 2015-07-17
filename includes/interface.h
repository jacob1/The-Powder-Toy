/**
 * Powder Toy - user interface (header)
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
#ifndef INTERFACE_H
#define INTERFACE_H
#include <vector>
#include <string>
#include "defines.h"
#include "graphics/Pixel.h"
#include "graphics/ARGBColour.h"

#define QM_TOGGLE	1

struct quick_option
{
	const char *icon;
	const char *name;
	int type;
	int *variable;
};
typedef struct quick_option quick_option;

const quick_option quickmenu[] =
{
	{"T", "Show tabs \bg(t)", QM_TOGGLE, &show_tabs},
	{"G", "Draw gravity grid \bg(ctrl+g)", QM_TOGGLE, &drawgrav_enable},
	{"D", "Show decorations \bg(ctrl+b)", QM_TOGGLE, &decorations_enable},
	{"N", "Newtonian gravity \bg(n)", QM_TOGGLE, &ngrav_enable},
	{"A", "Ambient heat \bg(u)", QM_TOGGLE, &aheat_enable},
	{"P", "Sand effect", QM_TOGGLE, &pretty_powder},
	{"C", "Show Console \bg(~)", QM_TOGGLE, &console_mode},
	{NULL}
};

extern char tabNames[10][255];
extern pixel* tabThumbnails[10];

struct menu_wall
{
	ARGBColour colour;
	const char *descs;
};
typedef struct menu_wall menu_wall;

const menu_wall colourlist[] =
{
	{COLPACK(0xFF0000), "Red"},
	{COLPACK(0x00FF00), "Green"},
	{COLPACK(0x0000FF), "Blue"},
	{COLPACK(0xFFFF00), "Yellow"},
	{COLPACK(0xFF00FF), "Pink"},
	{COLPACK(0x00FFFF), "Cyan"},
	{COLPACK(0xFFFFFF), "White"},
	{COLPACK(0x000000), "Black"},
};
#define DECO_RED 260
#define DECO_GREEN 261
#define DECO_BLUE 262
#define DECO_YELLOW 263
#define DECO_PINK 264
#define DECO_CYAN 265
#define DECO_WHITE 266
#define DECO_BLACK 267

#define DECO_PRESET_START 260
#define NUM_COLOR_PRESETS 8

struct ui_edit
{
	int x, y, w, nx, h, limit, focus, alwaysFocus, hide, overDelete;
	char str[1024],*def;
	int multiline, resizable, resizespeed;
	int cursor, cursorstart, highlightstart, highlightlength, lastClick, numClicks, clickPosition;
};
typedef struct ui_edit ui_edit;

struct ui_label
{
	int x, y, w, h, focus, multiline, maxHeight;
	char str[1024];
	int cursor, cursorstart, highlightstart, highlightlength, lastClick, numClicks, clickPosition;
};
typedef struct ui_label ui_label;

struct ui_list
{
	int x, y, w, h;
	char str[256],*def,**items;
	int selected, focus, count;
};
typedef struct ui_list ui_list;

struct ui_copytext
{
	int x, y, width, height;
	char text[256];
	int state, hover;
};
typedef struct ui_copytext ui_copytext;

#define NUM_COMMENTS 200
struct save_info
{
	char *title;
	char *name;
	char *author;
	char *date;
	char *description;
	int publish;
	int voteup;
	int votedown;
	int vote;
	int myvote;
	int downloadcount;
	int myfav;
	char *tags;
	int comment_count;
	//char *comments[NUM_COMMENTS];
	ui_label comments[NUM_COMMENTS];
	char *commentauthors[NUM_COMMENTS];
	char *commentauthorsunformatted[NUM_COMMENTS];
	char *commentauthorIDs[NUM_COMMENTS];
	char *commenttimestamps[NUM_COMMENTS];
};
typedef struct save_info save_info;

struct ui_checkbox
{
	int x, y;
	int focus, checked;
};
typedef struct ui_checkbox ui_checkbox;

struct ui_richtext
{
	int x, y;
	char str[512];
	char printstr[512];
	int regionss[6];
	int regionsf[6];
	char action[6];
	char actiondata[6][256];
	char actiontext[6][256];
};
typedef struct ui_richtext ui_richtext;

typedef struct command_history command_history;
struct command_history {
	command_history *prev_command;
	ui_label command;
};
//typedef struct command_history command_history;

extern command_history *last_command;
extern command_history *last_command_result;

extern unsigned short sdl_mod;
extern int sdl_key, sdl_rkey, sdl_wheel, sdl_ascii, sdl_zoom_trig;

extern char *shift_0;
extern char *shift_1;
extern int svf_messages;
extern int svf_login;
extern int svf_admin;
extern int svf_mod;
extern char svf_user[64];
extern char svf_pass[64];
extern char svf_user_id[64];
extern char svf_session_id[64];
extern char svf_session_key[64];


extern char svf_filename[255];
extern int svf_fileopen;
extern int svf_open;
extern int svf_own;
extern int svf_myvote;
extern int svf_publish;
extern char svf_id[16];
extern char svf_name[64];
extern char svf_tags[256];
extern char svf_description[255];
extern char svf_author[64];
extern void *svf_last;
extern int svf_lsize;

extern char *search_ids[GRID_X*GRID_Y];
extern char *search_dates[GRID_X*GRID_Y];
extern int   search_votes[GRID_X*GRID_Y];
extern int   search_publish[GRID_X*GRID_Y];
extern int	  search_scoredown[GRID_X*GRID_Y];
extern int	  search_scoreup[GRID_X*GRID_Y];
extern char *search_names[GRID_X*GRID_Y];
extern char *search_owners[GRID_X*GRID_Y];
extern void *search_thumbs[GRID_X*GRID_Y];
extern int   search_thsizes[GRID_X*GRID_Y];

extern int search_own;
extern int search_fav;
extern int search_date;
extern int search_page;
extern int p1_extra;
extern char search_expr[256];

extern char *tag_names[TAG_MAX];
extern int tag_votes[TAG_MAX];

extern int zoom_en;
extern int zoom_x, zoom_y;
extern int zoom_wx, zoom_wy;

extern int hud_menunum;
extern int dateformat;
extern int show_ids;
extern int fastquit;

extern ARGBColour decocolour;
extern ui_edit box_R;
extern ui_edit box_G;
extern ui_edit box_B;
extern ui_edit box_A;
extern int currR;
extern int currG;
extern int currB;
extern int currA;
extern int currH;
extern int currS;
extern int currV;

void quickoptions_menu(pixel *vid_buf, int b, int bq, int x, int y);

extern int propSelected;
extern char propValue[255];
void prop_edit_ui(pixel *vid_buf);

void get_sign_pos(int i, int *x0, int *y0, int *w, int *h);

void add_sign_ui(pixel *vid_buf, int mx, int my);

void ui_edit_init(ui_edit *ed, int x, int y, int w, int h);
int ui_edit_draw(pixel *vid_buf, ui_edit *ed);
void ui_edit_process(int mx, int my, int mb, int mbq, ui_edit *ed);

void ui_label_init(ui_label *ed, int x, int y, int w, int h);
int ui_label_draw(pixel *vid_buf, ui_label *ed);
void ui_label_process(int mx, int my, int mb, int mbq, ui_label *ed);

void ui_list_draw(pixel *vid_buf, ui_list *ed);
void ui_list_process(pixel * vid_buf, int mx, int my, int mb, ui_list *ed);

void ui_checkbox_draw(pixel *vid_buf, ui_checkbox *ed);
void ui_checkbox_process(int mx, int my, int mb, int mbq, ui_checkbox *ed);

void ui_copytext_draw(pixel *vid_buf, ui_copytext *ed);
void ui_copytext_process(int mx, int my, int mb, int mbq, ui_copytext *ed);

void ui_richtext_draw(pixel *vid_buf, ui_richtext *ed);
void ui_richtext_settext(char *text, ui_richtext *ed);
void ui_richtext_process(int mx, int my, int mb, int mbq, ui_richtext *ed);

void draw_svf_ui(pixel *vid_buf, int alternate);

void error_ui(pixel *vid_buf, int err, const char *txt);

void element_search_ui(pixel *vid_buf, Tool** sl, Tool** sr);

void info_ui(pixel *vid_buf, char *top, char *txt);

void copytext_ui(pixel *vid_buf, char *top, char *txt, char *copytxt);

void info_box(pixel *vid_buf, char *msg);

void info_box_overlay(pixel *vid_buf, char *msg);

char *input_ui(pixel *vid_buf, char *title, char *prompt, char *text, char *shadow);

bool confirm_ui(pixel *vid_buf, char *top, char *msg, char *btn);

void login_ui(pixel *vid_buf);

int stamp_ui(pixel *vid_buf, int *reorder);

void tag_list_ui(pixel *vid_buf);

int save_name_ui(pixel *vid_buf);

int DoLocalSave(std::string savename, void *saveData, int saveDataSize, bool force = false);
int save_filename_ui(pixel *vid_buf);

void old_menu_v2(int active_menu, int x, int y, int b, int bq);
void menu_ui_v2(pixel *vid_buf, int i);
void menu_ui_v3(pixel *vid_buf, int i, int b, int bq, int mx, int my);

class Tool;
extern std::string favMenu[18];
Tool* menu_draw(int mx, int my, int b, int bq, int i);
void menu_draw_text(Tool* over, int y);
void menu_select_element(int b, Tool* over);

union SDL_Event;
int EventProcess(SDL_Event event);
int sdl_poll(void);

void stickmen_keys();

void limit_fps();

void set_cmode(int cm);

char *download_ui(pixel *vid_buf, const char *uri, int *len);

int search_ui(pixel *vid_buf);

int open_ui(pixel *vid_buf, char *save_id, char *save_date, int instant_open);

void catalogue_ui(pixel * vid_buf);

int info_parse(char *info_data, save_info *info);

int search_results(char *str, int votes);

int execute_tagop(pixel *vid_buf, char *op, char *tag);

int execute_save(pixel *vid_buf);

int execute_delete(pixel *vid_buf, char *id);

int execute_report(pixel *vid_buf, char *id, char *reason);
int execute_bug(pixel *vid_buf, char *feedback);

bool ParseServerReturn(char *result, int status);

bool execute_submit(pixel *vid_buf, char *id, char *message);

void execute_fav(pixel *vid_buf, char *id);

void execute_unfav(pixel *vid_buf, char *id);

int execute_vote(pixel *vid_buf, char *id, char *action);

void open_link(std::string uri);

int report_ui(pixel *vid_buf, char *save_id, bool bug);

void console_limit_history(int limit, command_history *commandList);
int console_ui(pixel *vid_buf);

void init_colour_boxes();

void decoration_editor(pixel *vid_buf, int b, int bq, int mx, int my);

void render_ui(pixel *vid_buf, int xcoord, int ycoord, int orientation);

void simulation_ui(pixel *vid_buf);

int mouse_get_state(int *x, int *y);

bool mouse_coords_window_to_sim(int *mouseX, int *mouseY);

void converttotime(char *timestamp, char **timestring, int show_day, int show_year, int show_time);

#endif

