#ifndef HUD_H
#define HUD_H

#include "graphics/ARGBColour.h"

void HudDefaults();
void SetCurrentHud();

void SetRightHudText(int x, int y);
void SetLeftHudText(float FPSB2);
void DrawHud(int introTextAlpha, int qTipAlpha);
void DrawPhotonWavelengths(pixel *vid, int x, int y, int h, int wl);

void DrawRecordsInfo();

void DrawLuaLogs();

void GetTimeString(int currtime, char *string, int length);

struct hud
{
	const char *name;
	ARGBColour colour;
	int menunum;
	const char *description;
};
typedef struct hud hud;

const hud hud_menu[] =
{
	{"BACK",   COLPACK(0xFF7F00), 0, "Go Back"},
	{"UI",     COLPACK(0x20D8FF), 0, "Change the HUD on the left"},
	{"INFO",   COLPACK(0xFFFFFF), 0, "Change the top right HUD with particle properties"},
	{"CORD",   COLPACK(0xFFFFFF), 0, "Change the bottom right HUD with corrdinate info"},
	{"RSET",   COLPACK(0xFF8A08), 0, "Resets the HUDs back to their default values"},

	{"VERS", COLPACK(0x000000), 1, "The version number"},
	{"BLD",  COLPACK(0x000000), 1, "The current build number"},
	{"FPS",  COLPACK(0x000000), 1, "Show the current frames per second"},
	{"FPS#", COLPACK(0x000000), 1, "Show FPS to "},
	{"PART", COLPACK(0x000000), 1, "The number of particles on the screen"},
	{"GEN",  COLPACK(0x000000), 1, "The current life generation"},
	{"GRAV", COLPACK(0x000000), 1, "Shows the current gravity mode, changed by pressing w"},
	{"AIR",  COLPACK(0x000000), 1, "Shows the current air mode, change it with y"},
	{"XTRA", COLPACK(0x000000), 1, "Shows whether replace mode or specific delete are activated"},
	{"GRID", COLPACK(0x000000), 1, "Adds the current grid mode to the end, when it's activated."},
	
	//10
	{"NAME", COLPACK(0x000000), 2, "Why would you want to disable this?"},
	{"CTYP", COLPACK(0x000000), 2, "Puts the ctype in parentheses"},
	{"CTP2", COLPACK(0x000000), 2, "Display invalid ctypes as numbers instead of showing no ctype"},
	{"MOLT", COLPACK(0x000000), 2, "Molten [NAME] instead of name & ctype, also special FILT mode info"},
	{"PIPE", COLPACK(0x000000), 2, "PIPE ([NAME]) instead of name & not useful ctype"},
	{"CELC", COLPACK(0x000000), 2, "Show temperatures in Celsius"},
	{"FARH", COLPACK(0x000000), 2, "Show temperatures in Fahrenheit"},
	{"KELV", COLPACK(0x000000), 2, "Show temperatures in Kelvin"},
	{"TEM#", COLPACK(0x000000), 2, "Show all temperatures to "},
	{"LIFE", COLPACK(0x000000), 2, "Show a particle's life value"},
	{"TMP",  COLPACK(0x000000), 2, "Show a particle's tmp value"},
	{"TMP2", COLPACK(0x000000), 2, "Show a particle's tmp2 value"},
	{"CORD", COLPACK(0x000000), 2, "Show a particle's exact coordinates"},
	{"CRD#", COLPACK(0x000000), 2, "Show x and y coordinates to "},
	{"VEL",  COLPACK(0x000000), 2, "Show a particle's x and y velocities"},
	{"VEL#", COLPACK(0x000000), 2, "Show x and y velocities to "},
	{"PRES", COLPACK(0x000000), 2, "Show the pressure"},
	{"PRS#", COLPACK(0x000000), 2, "Show pressure to "},
	
	//28
	{"INDX", COLPACK(0x000000), 3, "The index of a particle"},
	{"MCRD", COLPACK(0x000000), 3, "Shows the mouse coordinates"},
	{"GRAV", COLPACK(0x000000), 3, "Shows the gravity at a spot if Newtonian gravity is enabled"},
	{"GRV#", COLPACK(0x000000), 3, "Show gravity to "},
	{"PRES", COLPACK(0x000000), 3, "Show the pressure"},
	{"PRS#", COLPACK(0x000000), 3, "Show pressure to "},
	{"AMB",  COLPACK(0x000000), 3, "Show the ambient heat"},
	{"AMB#", COLPACK(0x000000), 3, "Show ambient heat to "},

	//36
	{"DAY",  COLPACK(0x000000), 1, "Show the weekday, month, and day"},
	{"TIME", COLPACK(0x000000), 1, "Show the time of day"},
	{"YEAR", COLPACK(0x000000), 1, "Show the year"},
	{"TIME", COLPACK(0x000000), 1, "The time played since The Powder Toy was started"},
	{"TTME", COLPACK(0x000000), 1, "The total time played ever"},
	{"AFPS", COLPACK(0x000000), 1, "Show the average FPS"},
	{"AFS#", COLPACK(0x000000), 1, "Show the average FPS to "},
	//43
	{"VEL",	 COLPACK(0x000000), 3, "Show the velocity"},
	{"VEL#", COLPACK(0x000000), 3, "Show velocity to "},
	{"PHOT", COLPACK(0x000000), 2, "Show the colour of PHOT, FILT, and BIZR wavelengths"},
	{"DECO", COLPACK(0x000000), 2, "Show the decoration colour"},
	{"FLAG", COLPACK(0x000000), 2, "Show the flags (mostly unused)"},
	{"WALL", COLPACK(0x000000), 2, "Show wall names"},
	{"LIFE", COLPACK(0x000000), 2, "Show the name of life particles instead of LIFE([NAME])"},
	{"FILT", COLPACK(0x000000), 2, "Show FILT tmp modes in the HUD"},
	{"PAVG", COLPACK(0x000000), 2, "Show pavg[0] and pavg[1], used by VIRS and PIPE to store extra info"},
	{"EMAP", COLPACK(0x000000), 3, "Show the value of emap, used in conductive walls"},
};

#define HUD_START 400
#define HUD_REALSTART 405
#define HUD_NUM 58
#define HUD_OPTIONS 53

extern int currentHud[HUD_OPTIONS];
extern int normalHud[HUD_OPTIONS];
extern int debugHud[HUD_OPTIONS];

#endif
