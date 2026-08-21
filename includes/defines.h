/**
 * Powder Toy - Main source
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
#ifndef DEFINE_H
#define DEFINE_H

#ifdef DEBUG
#include <iostream>
#endif

#ifdef WIN
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

//VersionInfoStart
#define SAVE_VERSION 100
#define MINOR_VERSION 1
#define BUILD_NUM 400
//VersionInfoEnd
#define FAKE_SAVE_VERSION 100
#define FAKE_MINOR_VER 1

// Used in user agent to define the website api this version supports
#define API_VERSION_MAJOR 97
#define API_VERSION_MINOR 0 // always 0

#define MOD_VERSION 60
#define MOD_MINOR_VERSION 0
#define MOD_SAVE_VERSION 27 //This is not the version number of my mod anymore, it's only changed when I change the saving code
#define MOD_BUILD_VERSION 174 //For update checks

#define MOBILE_MAJOR 1
#define MOBILE_MINOR 24
#define MOBILE_BUILD 132

#define IDENT_VERSION "G" //Change this if you're not Simon! It should be a single letter

#define MTOS_EXPAND(str) #str
#define MTOS(str) MTOS_EXPAND(str)

#define SCHEME "https://"
#define STATICSCHEME "https://"
#define UPDATESCHEME "https://"
#define ENFORCE_HTTPS

#ifndef SERVER
#define SERVER "powdertoy.co.uk"
//#define SERVER "tptserv.starcatcher.us"
#endif
#ifndef STATICSERVER
#define STATICSERVER "static.powdertoy.co.uk"
//#define STATICSERVER "tptserv.starcatcher.us/Static"
#endif
#ifndef UPDATESERVER
#define UPDATESERVER "starcatcher.us/TPT"
#endif

#define LOCAL_SAVE_DIR "Saves"

#define THUMB_CACHE_SIZE 256

#ifndef M_PI
#define M_PI 3.14159265f
#endif
#ifndef M_GRAV
#define M_GRAV 6.67300e-1
#endif

#define TIMEOUT 100

#ifdef RENDERER
#define MENUSIZE 0
#define BARSIZE 0
#else
#define MENUSIZE 40
#ifdef TOUCHUI
#define BARSIZE 30
#else
#define BARSIZE 17
#endif
#endif
#define XRES	612
#define YRES	384
#define VIDXRES (XRES+BARSIZE)
#define VIDYRES (YRES+MENUSIZE)
#define NPART XRES*YRES
const int menuStartPosition = XRES+BARSIZE-17;
const int menuIconWidth = 17;

#define XCNTR   XRES/2
#define YCNTR   YRES/2

#define MAX_DISTANCE sqrt(pow(XRES, 2.0f)+pow(YRES, 2.0f))

#define GRAV_DIFF

#define CELL    4
constexpr int XCELLS = XRES / CELL;
constexpr int YCELLS = YRES / CELL;
constexpr int NCELL = XCELLS * YCELLS;
#define ISTP    (CELL/2)
#define CFDS	(4.0f/CELL)
#define MAX_VELOCITY 1e4f

#define AIR_TSTEPP 0.3f
#define AIR_TSTEPV 0.4f
#define AIR_VADV 0.3f
#define AIR_VLOSS 0.999f
#define AIR_PLOSS 0.9999f

#define GRID_X 5
#define GRID_Y 4
#define GRID_P 3
#define GRID_S 6
#define GRID_Z 3

#define CATALOGUE_X 4
#define CATALOGUE_Y 3
#define CATALOGUE_S 6
#define CATALOGUE_Z 3

//#define SAVE_OPS
//#define REALISTIC

#define SURF_RANGE     10
#define NORMAL_MIN_EST 3
#define NORMAL_INTERP  20
#define NORMAL_FRAC    16

#define REFRACT        0x80000000

/* heavy flint glass, for awesome refraction/dispersion
   this way you can make roof prisms easily */
#define GLASS_IOR      1.9f
#define GLASS_DISP     0.07f

#ifdef _MSC_VER
#pragma warning(disable: 4100) // unreferenced formal parameter
#endif

#define SDEUT

#define DEBUG_PARTS       0x0001
#define DEBUG_ELEMENTPOP  0x0002
#define DEBUG_LINES       0x0004
#define DEBUG_PARTICLE    0x0008
#define DEBUG_SURFNORM    0x0010
#define DEBUG_SIMHUD      0x0020
#define DEBUG_RENHUD      0x0040

extern bool firstRun;
extern bool redirectStd;
extern bool showLargeScreenDialog;
extern int screenWidth;
extern int screenHeight;
extern float FPSB2;
extern int elapsedTime;

extern int NUM_PARTS;

extern bool legacy_enable;
extern bool aheat_enable;
extern bool decorations_enable;
extern int active_menu;
extern int last_active_menu;
extern int last_fav_menu;
extern bool hud_enable;
extern bool pretty_powder;
extern bool drawgrav_enable;
extern bool water_equal_test;
extern int finding;
extern int foundParticles;
extern int highesttemp;
extern int lowesttemp;
extern int heatmode;
extern int secret_els;
extern int tab_num;
extern int num_tabs;
extern bool show_tabs;
#ifdef __cplusplus
class Tool;
extern Tool* activeTools[3];
#endif
extern int autosave;
extern bool explUnlocked;
extern int old_menu;
extern int decobox_hidden;
extern bool loadIncompatibleSaves;

extern int drawinfo;
extern int currentTime;
extern int totaltime;
extern int totalafktime;
extern int afktime;
extern int frames;
extern double totalfps;
extern int prevafktime;
extern int timesplayed;

extern int deco_disablestuff;

extern bool doUpdates;

extern int debug_flags;
#define DEBUG_PERF_FRAMECOUNT 256
extern int debug_perf_istart;
extern int debug_perf_iend;
extern long debug_perf_frametime[DEBUG_PERF_FRAMECOUNT];
extern long debug_perf_partitime[DEBUG_PERF_FRAMECOUNT];
extern long debug_perf_time;

extern int active_menu;

extern bool sys_pause;
extern int framerender;

#include "graphics/Pixel.h"
struct stamp
{
	char name[11];
	pixel *thumb;
	int thumb_w, thumb_h, dodelete;
};
typedef struct stamp stamp;

extern bool console_mode;
extern bool graveExitsConsole;
extern bool REPLACE_MODE;
extern bool SPECIFIC_DELETE;
extern int GRID_MODE;
extern int DEBUG_MODE;

extern int ptsaveOpenID;
extern int saveURIOpen;
extern int do_open;
extern pixel *vid_buf;

extern int scrollSpeed;
extern int scrollSpeedMomentum;
extern float scrollDeceleration;

extern unsigned short last_major, last_minor, update_flag, last_build, last_modbuild;

extern char http_proxy_string[256];

//Functions in main.c
void thumb_cache_inval(char *id);
void thumb_cache_add(char *id, void *thumb, int size);
bool thumb_cache_find(char *id, void **thumb, int *size);
void clear_sim();
void NewSim();
void tab_save(int num);
int tab_load(int tabNum, bool del = false, bool showException = true);
void ctrlzSnapshot();

extern bool sendNewEvents;
extern bool openConsole;
extern bool openSign;
extern bool openProp;
#ifdef __cplusplus
class PowderToy;
extern PowderToy *the_game;
#endif
int main_loop_temp(int b, int bq, int sdl_key, int scan, int x, int y, bool shift, bool ctrl, bool alt);
void main_end_hack();
#endif
