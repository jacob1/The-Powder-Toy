#ifndef TOOLNUMBERS_H
#define TOOLNUMBERS_H
#include <string>
#include "graphics/ARGBColour.h"

#define TOOL_HEAT	0
#define TOOL_COOL	1
#define TOOL_AIR	2
#define TOOL_VAC	3
#define TOOL_PGRV	4
#define TOOL_NGRV	5
#define TOOL_WIND	6
#define TOOL_PROP	7
#define TOOL_SIGN	8
#define TOOLCOUNT	9

#define OLD_PT_WIND	147
#define OLD_WL_SIGN	126
#define OLD_SPC_AIR	236
#define SPC_AIR		256

struct toolType
{
	std::string name;
	std::string identifier;
	ARGBColour colour;
	std::string descs;
};
typedef struct toolType toolType;

static toolType toolTypes[] =
{
	{"HEAT", "DEFAULT_TOOL_HEAT",	COLPACK(0xFFBB00), "Heats the targeted element."},
	{"COOL", "DEFAULT_TOOL_COOL",	COLPACK(0x00BBFF), "Cools the targeted element."},
	{"AIR",  "DEFAULT_TOOL_AIR",	COLPACK(0xFFFFFF), "Air, creates airflow and pressure."},
	{"VAC",  "DEFAULT_TOOL_VAC",	COLPACK(0x303030), "Vacuum, reduces air pressure."},
	{"PGRV", "DEFAULT_TOOL_PGRV",	COLPACK(0xCCCCFF), "Creates a short-lasting gravity well."},
	{"NGRV", "DEFAULT_TOOL_NGRV",	COLPACK(0xAACCFF), "Creates a short-lasting negative gravity well."},
	{"WIND", "DEFAULT_UI_WIND",		COLPACK(0x000000), "Creates air movement."},
	{"PROP", "DEFAULT_UI_PROPERTY",	COLPACK(0xFFAA00), "Property drawing tool."},
	{"SIGN", "DEFAULT_UI_SIGN",		COLPACK(0x808080), "Sign. Displays text. Click on a sign to edit it or anywhere else to place a new one."}
};

#define DECO_DRAW		0
#define DECO_CLEAR		1
#define DECO_ADD		2
#define DECO_SUBTRACT	3
#define DECO_MULTIPLY	4
#define DECO_DIVIDE		5
#define DECO_SMUDGE		6
#define DECO_LIGHTEN	7
#define DECO_DARKEN		8
#define DECOCOUNT		9

struct decoType
{
	std::string name;
	std::string identifier;
	ARGBColour colour;
	std::string descs;
};
typedef struct decoType decoType;

static decoType decoTypes[] =
{
	{"SET", "DEFAULT_DECOR_SET",	COLPACK(0xFF0000), "Draw decoration."},
	{"CLR", "DEFAULT_DECOR_CLR",	COLPACK(0x000000), "Erase decoration."},
	{"ADD", "DEFAULT_DECOR_ADD",	COLPACK(0x323232), "Color blending: Add."},
	{"SUB", "DEFAULT_DECOR_SUB",	COLPACK(0x323232), "Color blending: Subtract."},
	{"MUL", "DEFAULT_DECOR_MUL",	COLPACK(0x323232), "Color blending: Multiply."},
	{"DIV", "DEFAULT_DECOR_DIV",	COLPACK(0x323232), "Color blending: Divide."},
	{"SMDG", "DEFAULT_DECOR_SMDG",	COLPACK(0x00FF00), "Smudge tool, blends surrounding deco together."},
	{"LIGH", "DEFAULT_DECOR_LIGH",	COLPACK(0xDDDDDD), "Lighten deco colour."},
	{"DARK", "DEFAULT_DECOR_DARK",	COLPACK(0x111111), "Darken deco colour."}
};


//fav menu stuff since doesn't go anywhere else
#define FAV_START 300
#define FAV_MORE 300
#define FAV_BACK 301
#define FAV_FIND 302
#define FAV_INFO 303
#define FAV_ROTATE 304
#define FAV_HEAT 305
//#define FAV_SAVE 306
#define FAV_LUA 306
#define FAV_CUSTOMHUD 307
//#define FAV_AUTOSAVE 308
#define FAV_REAL 308
#define FAV_FIND2 309
#define FAV_DATE 310
#define FAV_SECR 311
#define FAV_END 312

struct fav_menu
{
	const char *name;
	ARGBColour colour;
	const char *description;
};
typedef struct fav_menu fav_menu;

const fav_menu fav[] =
{
	{"MORE", COLPACK(0xFF7F00), "Display different options"},
	{"BACK", COLPACK(0xFF7F00), "Go back to the favorites menu"},
	//{"HUD",  COLPACK(0x20D8FF), "Left click to toggle a different HUD. Now using the "},
	{"FIND", COLPACK(0xFF0000), "Highlights the currently selected element in red"},
	{"INFO", COLPACK(0x00FF00), "Displays statistics and records about The Powder Toy. Left click to toggle display"},
	{"SPIN", COLPACK(0x0010A0), "Makes moving solids rotate. Currently "},
	{"HEAT", COLPACK(0xFF00D4), "Changes heat display mode. Current mode: "},
	//{"SAVE", COLPACK(0x2B1AC9), "Makes saves/stamps compatible with: "},
	{"LUA",  COLPACK(0xFFFF00), "Add Lua code to a save"},
	{"HUD2", COLPACK(0x20D8FF), "Make a custom HUD"},
	//{"AUTO", COLPACK(0xDF1BFF), "Sets how often your work is autosaved. Currently: "},
	{"REAL", COLPACK(0xFF6800), "Turns on realistic heat mode, by savask. Now "},
	{"FND2", COLPACK(0xDF0000), "Alternate find mode, looks different but may find things better. Now "},
	{"DATE", COLPACK(0x3FBB3F), "Change date and time format. Example: "},
	{"", COLPACK(0x000000), ""}
};

#endif
