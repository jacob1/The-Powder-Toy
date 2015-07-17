/**
 * Powder Toy - graphics (header)
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
#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef SDL_R_INCL
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#if defined(WIN) || defined(LIN)
#ifdef SDL_R_INCL
#include <SDL_syswm.h>
#else
#include <SDL/SDL_syswm.h>
#endif
#endif

#include <string>
#include "defines.h"
#include "graphics/Pixel.h"

extern pixel sampleColor;

extern unsigned int *render_modes;
extern unsigned int render_mode;
extern unsigned int colour_mode;
extern unsigned int *display_modes;
extern unsigned int display_mode;

#if defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
extern SDL_SysWMinfo sdl_wminfo;
extern Atom XA_CLIPBOARD, XA_TARGETS, XA_UTF8_STRING, XA_NET_FRAME_EXTENTS;
#endif
extern SDL_Surface *sdl_scrn;
extern int sdl_scale;
extern int savedWindowX;
extern int savedWindowY;

extern unsigned char fire_r[YRES/CELL][XRES/CELL];
extern unsigned char fire_g[YRES/CELL][XRES/CELL];
extern unsigned char fire_b[YRES/CELL][XRES/CELL];

extern unsigned int fire_alpha[CELL*3][CELL*3];
extern pixel *pers_bg;

extern char * flm_data;
extern int flm_data_points;
extern pixel flm_data_colours[];
extern float flm_data_pos[];

extern char * plasma_data;
extern int plasma_data_points;
extern pixel plasma_data_colours[];
extern float plasma_data_pos[];

struct gcache_item
{
	int isready;
	int pixel_mode;
	int cola, colr, colg, colb;
	int firea, firer, fireg, fireb;
};
typedef struct gcache_item gcache_item;

extern gcache_item *graphicscache;

void prepare_graphicscache();

void draw_other(pixel *vid);

void draw_rgba_image(pixel *vid, unsigned char *data, int x, int y, float a);

void render_gravlensing(pixel *src, pixel * dst);

void sdl_blit_1(int x, int y, int w, int h, pixel *src, int pitch);

void sdl_blit_2(int x, int y, int w, int h, pixel *src, int pitch);

void sdl_blit(int x, int y, int w, int h, pixel *src, int pitch);

void drawblob(pixel *vid, int x, int y, unsigned char cr, unsigned char cg, unsigned char cb);

void draw_tool_button(pixel *vid_buf, int x, int y, pixel colour, std::string name);
int draw_tool_xy(pixel *vid_buf, int x, int y, Tool* currents);

int DrawMenus(pixel *vid_buf, int hover, int mouseY);

void drawpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

int addchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a);

int drawtext(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a);

int drawhighlight(pixel *vid, int x, int y, const char *s);

int drawtext_outline(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a, int outr, int outg, int outb, int outa);

int drawtextwrap(pixel *vid, int x, int y, int w, int h, const char *s, int r, int g, int b, int a);

int drawhighlightwrap(pixel *vid, int x, int y, int w, int h, const char *s, int highlightstart, int highlightlength);

void drawrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void fillrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a);

void drawcircle(pixel *vid, int x, int y, int rx, int ry, int r, int g, int b, int a);

void fillcircle(pixel *vid, int x, int y, int rx, int ry, int r, int g, int b, int a);

void clearrect(pixel *vid, int x, int y, int w, int h);

void drawdots(pixel *vid, int x, int y, int h, int r, int g, int b, int a);

int charwidth(unsigned char c);

int textwidth(const char *s);

int drawtextmax(pixel *vid, int x, int y, int w, char *s, int r, int g, int b, int a);

int textnwidth(char *s, int n);

void textnpos(char *s, int n, int w, int *cx, int *cy);

int textwidthx(char *s, int w);

void textsize(char * s, int *width, int *height);

int textposxy(char *s, int width, int w, int h);

int textwrapheight(char *s, int width);

void blendpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void draw_icon(pixel *vid_buf, int x, int y, char ch, int flag);

void draw_air(pixel *vid);

void draw_grav_zones(pixel *vid);

void draw_grav(pixel *vid);

void draw_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int screenwidth);

void addpixel(pixel *vid, int x, int y, int r, int g, int b, int a);

void xor_pixel(int x, int y, pixel *vid);

void xor_line(int x1, int y1, int x2, int y2, pixel *vid);

void xor_rect(pixel *vid, int x, int y, int w, int h);

void blend_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int a);

struct Point;
void render_parts(pixel *vid, Point mousePos);

void render_before(pixel *part_vbuf);

void render_after(pixel *part_vbuf, pixel *vid_buf, Point mousePos);

#ifdef OGLR
void draw_parts_fbo();
#endif

void draw_parts(pixel *vid);

void draw_walls(pixel *vid);

void draw_find();

void render_signs(pixel *vid_buf);

void render_fire(pixel *dst);

void prepare_alpha(int size, float intensity);

void draw_image(pixel *vid, pixel *img, int x, int y, int w, int h, int a);

void dim_copy(pixel *dst, pixel *src);

void dim_copy_pers(pixel *dst, pixel *src);

void render_zoom(pixel *img);

int render_thumb(void *thumb, int size, int bzip2, pixel *vid_buf, int px, int py, int scl);

class Brush;
void render_cursor(pixel *vid, int x, int y, Tool* t, Brush* brush);

int LoadWindowPosition(int scale);

int SaveWindowPosition();

int sdl_open(void);

int set_scale(int scale, int kiosk);

int draw_debug_info(pixel* vid, int lm, int lx, int ly, int cx, int cy, int line_x, int line_y);

void init_display_modes();
void update_display_modes();

#ifdef OGLR
void clearScreen(float alpha);
void ogl_blit(int x, int y, int w, int h, pixel *src, int pitch, int scale);
void loadShaders();
#endif

#endif

#ifdef INCLUDE_SHADERS
extern const char * fireFragment;
extern const char * fireVertex;
extern const char * lensFragment;
extern const char * lensVertex;
extern const char * airVFragment;
extern const char * airVVertex;
extern const char * airPFragment;
extern const char * airPVertex;
extern const char * airCFragment;
extern const char * airCVertex;
#endif
