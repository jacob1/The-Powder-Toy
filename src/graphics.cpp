/**
 * Powder Toy - graphics
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

#include <math.h>
#include <sstream>
#ifdef SDL_R_INCL
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif
#include <bzlib.h>
#include <climits>
#if defined(WIN) || defined(LIN)
#ifdef SDL_R_INCL
#include <SDL_syswm.h>
#else
#include <SDL/SDL_syswm.h>
#endif
#endif

#ifdef OGLR
#ifdef MACOSX
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#elif WIN
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif

#include "defines.h"
#include "air.h"
#include "gravity.h"
#include "interface.h"
#include "powder.h"
#define INCLUDE_SHADERS
#include "graphics.h"
#include "powdergraphics.h"
#define INCLUDE_FONTDATA
#include "font.h"
#include "misc.h"
#include "hmap.h"
#include "luaconsole.h"
#include "hud.h"

#ifdef LIN
#include "images.h"
#endif

#include "game/Brush.h"
#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"
#include "simulation/ToolNumbers.h"
#include "simulation/GolNumbers.h"
#include "simulation/elements/EMP.h"
#include "simulation/elements/FIGH.h"

//unsigned cmode = CM_FIRE;
unsigned int *render_modes;
unsigned int render_mode;
unsigned int colour_mode;
unsigned int *display_modes;
unsigned int display_mode;
gcache_item *graphicscache;

#if defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
SDL_SysWMinfo sdl_wminfo;
Atom XA_CLIPBOARD, XA_TARGETS, XA_UTF8_STRING, XA_NET_FRAME_EXTENTS;
#endif
SDL_Surface *sdl_scrn;
int sdl_scale = 1;

#ifdef OGLR
GLuint zoomTex, vidBuf, airBuf, fireAlpha, glowAlpha, blurAlpha, partsFboTex, partsFbo, partsTFX, partsTFY, airPV, airVY, airVX;
GLuint fireProg, airProg_Pressure, airProg_Velocity, airProg_Cracker, lensProg;

const char * fireFragment = "#version 120\n\
uniform sampler2D fireAlpha;\
void main () {\
    vec4 texColor = texture2D(fireAlpha, gl_PointCoord);\
    gl_FragColor = vec4(gl_Color.rgb, texColor.a*gl_Color.a);\
}";
const char * fireVertex = "#version 120\n\
void main(void)\
{\
   gl_Position = ftransform();;\
   gl_FrontColor = gl_Color;\
}";
const char * lensFragment = "#version 120\n\
uniform sampler2D pTex;\
uniform sampler2D tfX;\
uniform sampler2D tfY;\
uniform float xres;\
uniform float yres;\
void main () {\
	vec4 transformX = texture2D(tfX, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t));\
	vec4 transformY = texture2D(tfY, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t));\
	transformX.r /= xres/4.0;\
	transformY.g /= yres/4.0;\
    vec4 texColor1 = vec4(\
    	texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.90, transformY.g*0.90)).r,\
    	texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.80, transformY.g*0.80)).g,\
    	texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.70, transformY.g*0.70)).b,\
    	1.0\
    );\
	vec4 texColor2 = vec4(\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.95, transformY.g*0.95)).r,\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.85, transformY.g*0.85)).g,\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.75, transformY.g*0.75)).b,\
		1.0\
	);\
	vec4 texColor3 = vec4(\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.85, transformY.g*0.85)).r,\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.75, transformY.g*0.75)).g,\
		texture2D(pTex, gl_TexCoord[0].st-vec2(transformX.r*0.65, transformY.g*0.65)).b,\
		1.0\
	);\
	vec4 texColor = texColor1*0.6 + texColor2*0.2 + texColor3*0.2;\
    gl_FragColor = texColor;\
}";
const char * lensVertex = "#version 120\n\
void main(void)\
{\
	gl_TexCoord[0]  = gl_MultiTexCoord0;\
	gl_Position = ftransform();;\
	gl_FrontColor = gl_Color;\
}";
const char * airVFragment = "#version 120\n\
uniform sampler2D airX;\
uniform sampler2D airY;\
uniform sampler2D airP;\
void main () {\
	vec4 texX = texture2D(airX, gl_TexCoord[0].st);\
	vec4 texY = texture2D(airY, gl_TexCoord[0].st);\
	vec4 texP = texture2D(airP, gl_TexCoord[0].st);\
	gl_FragColor = vec4(abs(texX.r)/2.0, texP.b/2.0, abs(texY.g)/2.0, 1.0);\
}";
const char * airVVertex = "#version 120\n\
void main(void)\
{\
	gl_TexCoord[0]  = gl_MultiTexCoord0;\
	gl_Position = ftransform();;\
	gl_FrontColor = gl_Color;\
}";
const char * airPFragment = "#version 120\n\
uniform sampler2D airX;\
uniform sampler2D airY;\
uniform sampler2D airP;\
void main () {\
	vec4 texP = texture2D(airP, gl_TexCoord[0].st);\
    gl_FragColor = vec4(max(texP.b/2.0, 0), 0, abs(min(texP.b/2.0, 0)), 1.0);\
}";
const char * airPVertex = "#version 120\n\
void main(void)\
{\
	gl_TexCoord[0]  = gl_MultiTexCoord0;\
	gl_Position = ftransform();;\
	gl_FrontColor = gl_Color;\
}";
const char * airCFragment = "#version 120\n\
uniform sampler2D airX;\
uniform sampler2D airY;\
uniform sampler2D airP;\
void main () {\
	vec4 texX = texture2D(airX, gl_TexCoord[0].st);\
	vec4 texY = texture2D(airY, gl_TexCoord[0].st);\
	vec4 texP = texture2D(airP, gl_TexCoord[0].st);\
    gl_FragColor = vec4(max(texP.b/2.0, 0), 0, abs(min(texP.b/2.0, 0)), 1.0) + vec4(abs(texX.r)/8.0, abs(texX.r)/8.0, abs(texX.r)/8.0, 1.0) + vec4(abs(texY.g)/8.0, abs(texY.g)/8.0, abs(texY.g)/8.0, 1.0);\
}";
const char * airCVertex = "#version 120\n\
void main(void)\
{\
	gl_TexCoord[0]  = gl_MultiTexCoord0;\
	gl_Position = ftransform();;\
	gl_FrontColor = gl_Color;\
}";

#endif

pixel sampleColor = 0;

unsigned char fire_r[YRES/CELL][XRES/CELL];
unsigned char fire_g[YRES/CELL][XRES/CELL];
unsigned char fire_b[YRES/CELL][XRES/CELL];

unsigned int fire_alpha[CELL*3][CELL*3];
pixel *pers_bg;

char * flm_data;
int flm_data_points = 4;
pixel flm_data_colours[] = {PIXPACK(0xAF9F0F), PIXPACK(0xDFBF6F), PIXPACK(0x60300F), PIXPACK(0x000000)};
float flm_data_pos[] = {1.0f, 0.9f, 0.5f, 0.0f};

char * plasma_data;
int plasma_data_points = 5;
pixel plasma_data_colours[] = {PIXPACK(0xAFFFFF), PIXPACK(0xAFFFFF), PIXPACK(0x301060), PIXPACK(0x301040), PIXPACK(0x000000)};
float plasma_data_pos[] = {1.0f, 0.9f, 0.5f, 0.25, 0.0f};

void init_display_modes()
{
	display_modes = (unsigned int*)calloc(1, sizeof(unsigned int));
	render_modes = (unsigned int*)calloc(5, sizeof(unsigned int));
	
	display_modes[0] = 0;
	render_modes[0] = RENDER_FIRE;
	render_modes[1] = RENDER_SPRK;
	render_modes[2] = RENDER_EFFE;
	render_modes[3] = RENDER_BASC;
	render_modes[4] = 0;
	
	update_display_modes();
}

// Combine all elements of the display_modes and render_modes arrays into single variables using bitwise or
void update_display_modes()
{
	int i;
	display_mode = 0;
	i = 0;
	while(display_modes[i])
	{
		display_mode |= display_modes[i];
		i++;
	}
	render_mode = 0;
	i = 0;
	while(render_modes[i])
	{
		render_mode |= render_modes[i];
		i++;
	}
}

#ifdef OGLR
void clearScreen(float alpha)
{
	if(alpha > 0.999f)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
    else
    {
		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		glColor4f(1.0f, 1.0f, 1.0f, alpha);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(XRES, 0);
		glVertex2f(XRES, YRES);
		glVertex2f(0, YRES);
		glEnd();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlendEquation(GL_FUNC_ADD);
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clearScreenNP(float alpha)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ogl_blit(int x, int y, int w, int h, pixel *src, int pitch, int scale)
{

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDrawPixels(w,h,GL_BGRA,GL_UNSIGNED_BYTE,src); //Why does this still think it's ABGR?
    glEnable( GL_TEXTURE_2D );
    glBindTexture(GL_TEXTURE_2D, vidBuf);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES+BARSIZE, YRES+MENUSIZE, GL_BGRA, GL_UNSIGNED_BYTE, src);
	glBegin(GL_QUADS);
    glTexCoord2d(1, 0);
    glVertex3f((XRES+BARSIZE)*sdl_scale, (YRES+MENUSIZE)*sdl_scale, 1.0);
    glTexCoord2d(0, 0);
    glVertex3f(0, (YRES+MENUSIZE)*sdl_scale, 1.0);
    glTexCoord2d(0, 1);
    glVertex3f(0, 0, 1.0);
    glTexCoord2d(1, 1);
    glVertex3f((XRES+BARSIZE)*sdl_scale, 0, 1.0);
    glEnd();

    glDisable( GL_TEXTURE_2D );
	glBlendFunc(GL_ONE, GL_ONE);
    glFlush();
    SDL_GL_SwapBuffers ();
}
#endif

void sdl_blit_1(int x, int y, int w, int h, pixel *src, int pitch)
{
	pixel *dst;
	int j;
	if (SDL_MUSTLOCK(sdl_scrn))
		if (SDL_LockSurface(sdl_scrn)<0)
			return;
	dst=(pixel *)sdl_scrn->pixels+y*sdl_scrn->pitch/PIXELSIZE+x;
	if (SDL_MapRGB(sdl_scrn->format,0x33,0x55,0x77)!=PIXPACK(0x335577))
	{
		//pixel format conversion
		int i;
		pixel px;
		SDL_PixelFormat *fmt = sdl_scrn->format;
		for (j=0; j<h; j++)
		{
			for (i=0; i<w; i++)
			{
				px = src[i];
				dst[i] = ((PIXR(px)>>fmt->Rloss)<<fmt->Rshift)|
						((PIXG(px)>>fmt->Gloss)<<fmt->Gshift)|
						((PIXB(px)>>fmt->Bloss)<<fmt->Bshift);
			}
			dst+=sdl_scrn->pitch/PIXELSIZE;
			src+=pitch;
		}
	}
	else
	{
		for (j=0; j<h; j++)
		{
			memcpy(dst, src, w*PIXELSIZE);
			dst+=sdl_scrn->pitch/PIXELSIZE;
			src+=pitch;
		}
	}
	if (SDL_MUSTLOCK(sdl_scrn))
		SDL_UnlockSurface(sdl_scrn);
	SDL_UpdateRect(sdl_scrn,0,0,0,0);
}

void sdl_blit_2(int x, int y, int w, int h, pixel *src, int pitch)
{
	pixel *dst;
	int j;
	int i,k;
	if (SDL_MUSTLOCK(sdl_scrn))
		if (SDL_LockSurface(sdl_scrn)<0)
			return;
	dst=(pixel *)sdl_scrn->pixels+y*sdl_scrn->pitch/PIXELSIZE+x;
	if (SDL_MapRGB(sdl_scrn->format,0x33,0x55,0x77)!=PIXPACK(0x335577))
	{
		//pixel format conversion
		pixel px;
		SDL_PixelFormat *fmt = sdl_scrn->format;
		for (j=0; j<h; j++)
		{
			for (k=0; k<sdl_scale; k++)
			{
				for (i=0; i<w; i++)
				{
					px = src[i];
					px = ((PIXR(px)>>fmt->Rloss)<<fmt->Rshift)|
						((PIXG(px)>>fmt->Gloss)<<fmt->Gshift)|
						((PIXB(px)>>fmt->Bloss)<<fmt->Bshift);
					dst[i*2]=px;
					dst[i*2+1]=px;
				}
				dst+=sdl_scrn->pitch/PIXELSIZE;
			}
			src+=pitch;
		}
	}
	else
	{
		for (j=0; j<h; j++)
		{
			for (k=0; k<sdl_scale; k++)
			{
				for (i=0; i<w; i++)
				{
					dst[i*2]=src[i];
					dst[i*2+1]=src[i];
				}
				dst+=sdl_scrn->pitch/PIXELSIZE;
			}
			src+=pitch;
		}
	}
	if (SDL_MUSTLOCK(sdl_scrn))
		SDL_UnlockSurface(sdl_scrn);
	SDL_UpdateRect(sdl_scrn,0,0,0,0);
}

void sdl_blit(int x, int y, int w, int h, pixel *src, int pitch)
{
#if defined(OGLR)
        ogl_blit(x, y, w, h, src, pitch, sdl_scale);
#else
	if (sdl_scale == 2)
		sdl_blit_2(x, y, w, h, src, pitch);
	else
		sdl_blit_1(x, y, w, h, src, pitch);
#endif
}

//an easy way to draw a blob
void drawblob(pixel *vid, int x, int y, unsigned char cr, unsigned char cg, unsigned char cb)
{
	blendpixel(vid, x+1, y, cr, cg, cb, 112);
	blendpixel(vid, x-1, y, cr, cg, cb, 112);
	blendpixel(vid, x, y+1, cr, cg, cb, 112);
	blendpixel(vid, x, y-1, cr, cg, cb, 112);

	blendpixel(vid, x+1, y-1, cr, cg, cb, 64);
	blendpixel(vid, x-1, y-1, cr, cg, cb, 64);
	blendpixel(vid, x+1, y+1, cr, cg, cb, 64);
	blendpixel(vid, x-1, y+1, cr, cg, cb, 64);
}

//draws the background and correctly coloured text for each button
void draw_tool_button(pixel *vid_buf, int x, int y, pixel colour, std::string name)
{
#ifdef OpenGL
	fillrect(vid_buf, x, y, 28, 16, PIXR(colour), PIXG(colour), PIXB(colour), 255);
#else
	for (int j = 1; j < 15; j++)
	{
		for (int i = 1; i < 27; i++)
		{
			vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
		}
	}
#endif

	int textColor = 0;
	if (PIXB(colour) + 3*PIXG(colour) + 2*PIXR(colour) < 544)
		textColor = 255;
	drawtext(vid_buf, x+14-textwidth(name.c_str())/2, y+4, name.c_str(), textColor, textColor, textColor, 255);
}

//draws walls and elements for menu
int draw_tool_xy(pixel *vid_buf, int x, int y, Tool* current)
{
	int i, j;
	if (x > XRES-26 || x < 0)
		return 26;
	if (current->GetType() == ELEMENT_TOOL)
	{
		draw_tool_button(vid_buf, x, y, PIXPACK(globalSim->elements[current->GetID()].Colour), globalSim->elements[current->GetID()].Name);

		//special case for erase tool
		if (!current->GetID())
		{
			for (j=4; j<12; j++)
			{
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+6)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+7)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+21)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+22)] = PIXPACK(0xFF0000);
			}
		}
	}
	else if (current->GetType() == WALL_TOOL)
	{
		int ds = wallTypes[current->GetID()].drawstyle;
		pixel colour = PIXPACK(wallTypes[current->GetID()].colour);
		pixel glowColor = PIXPACK(wallTypes[current->GetID()].eglow);
		
		if (ds==1)
		{
			for (j=1; j<15; j+=2)
				for (i=1+(1&(j>>1)); i<27; i+=2)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
		}
		else if (ds==2)
		{
			for (j=1; j<15; j+=2)
				for (i=1; i<27; i+=2)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
		}
		else if (ds==3)
		{
			for (j=1; j<15; j++)
				for (i=1; i<27; i++)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
		}
		else if (ds==4)
		{
			for (j=1; j<15; j++)
				for (i=1; i<27; i++)
					if(i%CELL == j%CELL)
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
					else if  (i%CELL == (j%CELL)+1 || (i%CELL == 0 && j%CELL == CELL-1))
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = glowColor;
					else 
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXPACK(0x202020);
		}
		else
		switch (current->GetID())
		{
		case WL_WALLELEC:
			for (j=1; j<15; j++)
			{
				for (i=1; i<27; i++)
				{
					if (!(i%2) && !(j%2))
					{
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
					}
					else
					{
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXPACK(0x808080);
					}
				}
			}
			break;
		case WL_EWALL:
			for (j=1; j<15; j++)
			{
				for (i=1; i<6+j; i++)
				{
					if (!(i&j&1))
					{
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
					}
				}
				for (; i<27; i++)
				{
					if (i&j&1)
					{
						vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
					}
				}
			}
			break;
		case WL_STREAM:
			for (j=1; j<15; j++)
			{
				for (i=1; i<27; i++)
				{
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = i==1||i==26||j==1||j==14 ? PIXPACK(0xA0A0A0) : PIXPACK(0x000000);
					drawtext(vid_buf, x+4, y+3, "\x8D", 255, 255, 255, 255);
				}
			}
			for (i=9; i<27; i++)
			{
				drawpixel(vid_buf, x+i, y+8+(int)(3.9f*cos(i*0.3f)), 255, 255, 255, 255);
			}
			break;
		case WL_ERASE:
			for (j=1; j<15; j+=2)
			{
				for (i=1+(1&(j>>1)); i<13; i+=2)
				{
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
				}
			}
			for (j=1; j<15; j++)
			{
				for (i=14; i<27; i++)
				{
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
				}
			}

			//X in middle
			for (j=4; j<12; j++)
			{
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+6)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+7)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+21)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+22)] = PIXPACK(0xFF0000);
			}
			break;
		case WL_ERASEALL:
			for (j=1; j<15; j++)
			{
				int r = 100, g = 150, b = 50;
				int rd = 1, gd = -1, bd = -1;
				for (i=1; i<27; i++)
				{
					int rc, gc, bc;
					r+=15*rd; g+=15*gd; b+=15*bd;
					if (r > 200) rd = -1;
					if (g > 200) gd = -1;
					if (b > 200) bd = -1;
					if (r < 15) rd = 1;
					if (g < 15) gd = 1;
					if (b < 15) bd = 1;
					rc = std::max(0,r); gc = std::max(0,g); bc = std::max(0,b);
					rc = std::min(150,rc); gc = std::min(200,gc); bc = std::min(200,bc);
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXRGB(rc, gc, bc);
				}
			}

			//double X in middle
			for (j=4; j<12; j++)
			{
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+0)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+1)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+15)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+16)] = PIXPACK(0xFF0000);

				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+11)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+12)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+26)] = PIXPACK(0xFF0000);
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+27)] = PIXPACK(0xFF0000);
			}
			break;
		default:
			draw_tool_button(vid_buf, x, y, colour, "");
		}
	}
	else if (current->GetType() == TOOL_TOOL)
	{
		if (current->GetID() == TOOL_SIGN)
		{
			for (j=1; j<15; j++)
			{
				for (i=1; i<27; i++)
				{
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = i==1||i==26||j==1||j==14 ? PIXPACK(0xA0A0A0) : PIXPACK(0x000000);
				}
			}
			drawtext(vid_buf, x+9, y+3, "\xA1", 32, 64, 128, 255);
			drawtext(vid_buf, x+9, y+3, "\xA0", 255, 255, 255, 255);
		}
		else
			draw_tool_button(vid_buf, x, y, PIXPACK(toolTypes[current->GetID()].colour), toolTypes[current->GetID()].name.c_str());
	}
	else if (current->GetType() == DECO_TOOL)
	{
		pixel colour = PIXPACK(decoTypes[current->GetID()].colour);
		for (j=1; j<15; j++)
		{
			for (i=1; i<27; i++)
			{
				if (current->GetID() == DECO_LIGHTEN)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXRGB(PIXR(colour)-10*j, PIXG(colour)-10*j, PIXB(colour)-10*j);
				else if (current->GetID() == DECO_DARKEN)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXRGB(PIXR(colour)+10*j, PIXG(colour)+10*j, PIXB(colour)+10*j);
				else if (current->GetID() == DECO_SMUDGE)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXRGB(PIXR(colour), PIXG(colour)-5*i, PIXB(colour)+5*i);
				else if (current->GetID() == DECO_DRAW || current->GetID() == DECO_CLEAR)
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = PIXPACK(decocolour);
				else
					vid_buf[(XRES+BARSIZE)*(y+j)+(x+i)] = colour;
			}
		}

		if (current->GetID() == DECO_CLEAR)
		{
			colour = PIXRGB((COLR(decocolour)+127)%256, (COLG(decocolour)+127)%256, (COLB(decocolour)+127)%256);
			for (j=4; j<12; j++)
			{
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+6)] = colour;
				vid_buf[(XRES+BARSIZE)*(y+j)+(x+j+7)] = colour;
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+21)] = colour;
				vid_buf[(XRES+BARSIZE)*(y+j)+(x-j+22)] = colour;
			}
		}
		else if (current->GetID() == DECO_ADD)
			drawtext(vid_buf, x+12, y+5, "+", COLR(decocolour), COLG(decocolour), COLB(decocolour), 255);
		else if (current->GetID() == DECO_SUBTRACT)
			drawtext(vid_buf, x+12, y+5, "-", COLR(decocolour), COLG(decocolour), COLB(decocolour), 255);
		else if (current->GetID() == DECO_MULTIPLY)
			drawtext(vid_buf, x+12, y+4, "x", COLR(decocolour), COLG(decocolour), COLB(decocolour), 255);
		else if (current->GetID() == DECO_DIVIDE)
			drawtext(vid_buf, x+12, y+5, "/", COLR(decocolour), COLG(decocolour), COLB(decocolour), 255);
	}
	else if (current->GetType() == GOL_TOOL)
	{
		draw_tool_button(vid_buf, x, y, PIXPACK(golTypes[current->GetID()].colour), golTypes[current->GetID()].name.c_str());
	}
	else if (current->GetType() == INVALID_TOOL)
	{
		if (current->GetID() >= FAV_START && current->GetID() < FAV_END)
			draw_tool_button(vid_buf, x, y, PIXPACK(fav[current->GetID()-FAV_START].colour), fav[current->GetID()-FAV_START].name);
		else if (current->GetID() >= HUD_START && current->GetID() < HUD_START+HUD_NUM)
		{
			if (hud_menu[current->GetID()-HUD_START].colour != COLPACK(0x000000))
				draw_tool_button(vid_buf, x, y, PIXPACK(hud_menu[current->GetID()-HUD_START].colour), hud_menu[current->GetID()-HUD_START].name);
			else
				draw_tool_button(vid_buf, x, y, PIXPACK(globalSim->elements[((current->GetID()-HUD_START)*53)%(PT_NUM-1)+1].Colour), hud_menu[current->GetID()-HUD_START].name);
		}
	}
	return 26;
}

int DrawMenus(pixel *vid_buf, int hover, int mouseY)
{
	int y = YRES+MENUSIZE-32, ret = -1;
	for (int i = SC_TOTAL-1; i >= 0; i--)
	{
		if (menuSections[i]->enabled)
		{
			drawrect(vid_buf, (XRES+BARSIZE)-16, y, 14, 14, 255, 255, 255, 255);

			if (hover == i || (i == SC_FAV && (hover == SC_FAV2 || hover == SC_HUD)))
			{
				fillrect(vid_buf, (XRES+BARSIZE)-16, y, 14, 14, 255, 255, 255, 255);
				drawchar(vid_buf, (XRES+BARSIZE)-13, y+2, menuSections[i]->icon, 0, 0, 0, 255);
			}
			else
			{
				drawchar(vid_buf, (XRES+BARSIZE)-13, y+2, menuSections[i]->icon, 255, 255, 255, 255);
			}

			if (mouseY >= y && mouseY < y+16)
				ret = i;
			y -= 16;
		}
	}
	return ret;
}

//draws a pixel, identical to blendpixel(), except blendpixel has OpenGL support
TPT_INLINE void drawpixel(pixel *vid, int x, int y, int r, int g, int b, int a)
{
#ifdef PIXALPHA
	pixel t;
	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return;
	if (a!=255)
	{
		t = vid[y*(XRES+BARSIZE)+x];
		r = (a*r + (255-a)*PIXR(t)) >> 8;
		g = (a*g + (255-a)*PIXG(t)) >> 8;
		b = (a*b + (255-a)*PIXB(t)) >> 8;
		a = a > PIXA(t) ? a : PIXA(t);
	}
	vid[y*(XRES+BARSIZE)+x] = PIXRGBA(r,g,b,a);
#else
	pixel t;
	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE || a == 0)
		return;
	if (a!=255)
	{
		t = vid[y*(XRES+BARSIZE)+x];
		r = (a*r + (255-a)*PIXR(t)) >> 8;
		g = (a*g + (255-a)*PIXG(t)) >> 8;
		b = (a*b + (255-a)*PIXB(t)) >> 8;
	}
	vid[y*(XRES+BARSIZE)+x] = PIXRGB(r,g,b);
#endif
}

TPT_INLINE int drawchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a)
{
	int i, j, w, bn = 0, ba = 0;
	char *rp = (char*)font_data + font_ptrs[c];
	w = *(rp++);
	for (j=0; j<FONT_H; j++)
		for (i=0; i<w; i++)
		{
			if (!bn)
			{
				ba = *(rp++);
				bn = 8;
			}
			drawpixel(vid, x+i, y+j, r, g, b, ((ba&3)*a)/3);
			ba >>= 2;
			bn -= 2;
		}
	return x + w;
}

int addchar(pixel *vid, int x, int y, int c, int r, int g, int b, int a)
{
	int i, j, w, bn = 0, ba = 0;
	char *rp = (char*)font_data + font_ptrs[c];
	w = *(rp++);
	for (j=0; j<FONT_H; j++)
		for (i=0; i<w; i++)
		{
			if (!bn)
			{
				ba = *(rp++);
				bn = 8;
			}
			{
			addpixel(vid, x+i, y+j, r, g, b, ((ba&3)*a)/3);
			}
			ba >>= 2;
			bn -= 2;
		}
	return x + w;
}

int drawtext(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a)
{
	int sx = x;
	bool highlight = false;
	int oR = r, oG = g, oB = b;
	for (; *s; s++)
	{
		if (*s == '\n' || *s == '\r')
		{
			x = sx;
			y += FONT_H+2;
			if (highlight && (s[1] == '\n' || s[1] == '\r' || (s[1] == '\x01' && (s[2] == '\n' || s[2] == '\r'))))
			{
				fillrect(vid, x-1, y-3, font_data[font_ptrs[' ']]+1, FONT_H+3, 0, 0, 255, 127);
			}
		}
		else if (*s == '\x0F')
		{
			if(!s[1] || !s[2] || !s[3]) break;
			oR = r;
			oG = g;
			oB = b;
			r = (unsigned char)s[1];
			g = (unsigned char)s[2];
			b = (unsigned char)s[3];
			s += 3;
		}
		else if (*s == '\x0E')
		{
			r = oR;
			g = oG;
			b = oB;
		}
		else if (*s == '\x01')
		{
			highlight = !highlight;
		}
		else if (*s == '\b')
		{
			switch (s[1])
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
			s++;
		}
		else
		{
			if (highlight)
			{
				fillrect(vid, x-1, y-3, font_data[font_ptrs[(int)(*(unsigned char *)s)]]+1, FONT_H+3, 0, 0, 255, 127);
			}
			x = drawchar(vid, x, y, *(unsigned char *)s, r, g, b, a);
		}
	}
	return x;
}

int drawhighlight(pixel *vid, int x, int y, const char *s)
{
	int sx = x;
	for (; *s; s++)
	{
		if (*s == '\n')
		{
			x = sx;
			y += FONT_H+2;
		}
		else if (*s == '\x0F')
		{
			s += 3;
		}
		else if (*s == '\x0E')
		{
			
		}
		else if (*s == '\x01')
		{
			
		}
		else if (*s == '\b')
		{
			s++;
		}
		else
		{
			int width = font_data[font_ptrs[(int)(*(unsigned char *)s)]];
			fillrect(vid, x-1, y-3, width+1, FONT_H+3, 0, 0, 255, 127);
			x += width;
		}
	}
	return x;
}

//Draw text with an outline
int drawtext_outline(pixel *vid, int x, int y, const char *s, int r, int g, int b, int a, int outr, int outg, int outb, int outa)
{
	drawtext(vid, x-1, y-1, s, outr, outg, outb, outa);
	drawtext(vid, x+1, y+1, s, outr, outg, outb, outa);
	
	drawtext(vid, x-1, y+1, s, outr, outg, outb, outa);
	drawtext(vid, x+1, y-1, s, outr, outg, outb, outa);
	
	return drawtext(vid, x, y, s, r, g, b, a);
}
int drawtextwrap(pixel *vid, int x, int y, int w, int h, const char *s, int r, int g, int b, int a)
{
	int sx = x;
	int rh = 12;
	int rw = 0;
	int cw = x;
	int wordlen;
	int charspace;
	int invert = 0;
	int oR = r, oG = g, oB = b;
	while (*s)
	{
		wordlen = strcspn(s," .,!?\n");
		charspace = textwidthx((char *)s, w-(x-cw));
		if (charspace<wordlen && wordlen && w-(x-cw)<w/3)
		{
			x = sx;
			rw = 0;
			y += FONT_H+2;
			rh += FONT_H+2;
		}
		if ((h > 0 && rh > h) || (h < 0 && rh > YRES+MENUSIZE-110)) // the second part is hacky, since this will only be used for comments anyway
			break;
		for (; *s && --wordlen>=-1; s++)
		{
			if (*s == '\n')
			{
				x = sx;
				rw = 0;
				y += FONT_H+2;
				rh += FONT_H+2;
			}
			else if (*s == '\x0F')
			{
				if(!s[1] || !s[2] || !s[3])
					goto textwrapend;
				oR = r;
				oG = g;
				oB = b;
				r = (unsigned char)s[1];
				g = (unsigned char)s[2];
				b = (unsigned char)s[3];
				s += 3;
			}
			else if (*s == '\x0E')
			{
				r = oR;
				g = oG;
				b = oB;
			}
			else if (*s == '\x01')
			{
				invert = !invert;
				r = 255-r;
				g = 255-g;
				b = 255-b;
			}
			else if (*s == '\b')
			{
				switch (s[1])
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
				}
				if(invert)
				{
					r = 255-r;
					g = 255-g;
					b = 255-b;
				}
				s++;
			}
			else
			{

				if (x-cw>=w)
				{
					x = sx;
					rw = 0;
					y+=FONT_H+2;
					rh+=FONT_H+2;
					if (*s==' ')
						continue;
				}
				if ((h > 0 && rh > h) || (h < 0 && rh > YRES+MENUSIZE-110)) // the second part is hacky, since this will only be used for comments anyway
					goto textwrapend;
				if (rh + h < 0)
					x = drawchar(vid, x, y, *(unsigned char *)s, 0, 0, 0, 0);
				else
					x = drawchar(vid, x, y, *(unsigned char *)s, r, g, b, a);
			}
		}
	}

textwrapend:
	return rh;
}

int drawhighlightwrap(pixel *vid, int x, int y, int w, int h, const char *s, int highlightstart, int highlightlength)
{
	int sx = x;
	int rh = 12;
	int rw = 0;
	int cw = x;
	int wordlen;
	int charspace;
	int num = 0;
	while (*s)
	{
		wordlen = strcspn(s," .,!?\n");
		charspace = textwidthx((char *)s, w-(x-cw));
		if (charspace<wordlen && wordlen && w-(x-cw)<w/3)
		{
			x = sx;
			rw = 0;
			y += FONT_H+2;
			rh += FONT_H+2;
		}
		if ((h > 0 && rh > h) || (h < 0 && rh > YRES+MENUSIZE-110)) // the second part is hacky, since this will only be used for comments anyway
			break;
		for (; *s && --wordlen>=-1; s++)
		{
			if (*s == '\n')
			{
				x = sx;
				rw = 0;
				y += FONT_H+2;
			}
			else if (*s == '\x0F')
			{
				s += 3;
				num += 4;
			}
			else if (*s == '\x0E')
			{
				num++;
			}
			else if (*s == '\x01')
			{
				num++;
			}
			else if (*s == '\b')
			{
				s++;
				num += 2;
			}
			else
			{
				int width = font_data[font_ptrs[(int)(*(unsigned char *)s)]];
				if (x-cw>=w)
				{
					x = sx;
					rw = 0;
					y+=FONT_H+2;
					rh+=FONT_H+2;
					if (*s==' ')
					{
						num++;
						continue;
					}
				}
				if ((h > 0 && rh > h) || (h < 0 && rh > YRES+MENUSIZE-110)) // the second part is hacky, since this will only be used for comments anyway
					goto highlightwrapend;
				if (num >= highlightstart && num < highlightstart + highlightlength && rh + h >= 0)
					fillrect(vid, x-1, y-3, width+1, FONT_H+3, 0, 0, 255, 127);
				x += width;
				num++;
			}
		}
	}

highlightwrapend:
	return rh;
}

//draws a rectange, (x,y) are the top left coords.
void drawrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a)
{
	int i;
	for (i=0; i<=w; i++)
	{
		drawpixel(vid, x+i, y, r, g, b, a);
		drawpixel(vid, x+i, y+h, r, g, b, a);
	}
	for (i=1; i<h; i++)
	{
		drawpixel(vid, x, y+i, r, g, b, a);
		drawpixel(vid, x+w, y+i, r, g, b, a);
	}
}

//draws a rectangle and fills it in as well.
void fillrect(pixel *vid, int x, int y, int w, int h, int r, int g, int b, int a)
{
	int i,j;
	for (j=1; j<h; j++)
		for (i=1; i<w; i++)
			drawpixel(vid, x+i, y+j, r, g, b, a);
}

void drawcircle(pixel* vid, int x, int y, int rx, int ry, int r, int g, int b, int a)
{
	int tempy = y, i, j, oldy;
	if (!rx)
	{
		for (j = -ry; j <= ry; j++)
			drawpixel(vid, x, y+j, r, g, b, a);
		return;
	}
	for (i = x - rx; i <= x; i++)
	{
		oldy = tempy;
		while (pow(i-x, 2.0)*pow(ry, 2.0)+pow(tempy-y, 2.0)*pow(rx, 2.0) <= pow(rx, 2.0)*pow(ry, 2.0))
			tempy = tempy - 1;
		tempy = tempy + 1;
		if (oldy != tempy)
			oldy--;
		for (j = tempy; j <= oldy; j++)
		{
			int i2 = 2*x-i, j2 = 2*y-j;
			drawpixel(vid, i, j, r, g, b, a);
			if (i2 != i)
				drawpixel(vid, i2, j, r, g, b, a);
			if (j2 != j)
				drawpixel(vid, i, j2, r, g, b, a);
			if (i2 != i && j2 != j)
				drawpixel(vid, i2, j2, r, g, b, a);
		}
	}
}

void fillcircle(pixel* vid, int x, int y, int rx, int ry, int r, int g, int b, int a)
{
	int tempy = y, i, j, oldy, jmax;
	if (!rx)
	{
		for (j = -ry; j <= ry; j++)
			drawpixel(vid, x, y+j, r, g, b, a);
		return;
	}
	for (i = x - rx; i <= x; i++)
	{
		oldy = tempy;
		while (pow(i-x, 2.0)*pow(ry, 2.0)+pow(tempy-y, 2.0)*pow(rx, 2.0) <= pow(rx, 2.0)*pow(ry, 2.0))
			tempy = tempy - 1;
		tempy = tempy + 1;
		jmax = 2*y - tempy;
		for (j = tempy; j <= jmax; j++)
		{
			drawpixel(vid, i, j, r, g, b, a);
			if (i != x)
				drawpixel(vid, 2*x-i, j, r, g, b, a);
		}
	}
}

/*void draw_egg(pixel* vid, int x, int y, int egg_num) //Old Easter Egg hunt from version 14.1/2
{
	int i, egg_clicked = 0, r = 0, g = 0, b = 0, a = 255;
	char text[32], text2[32];
	char* heavy = "This egg is heavy";
	char* inside = "You find something inside";
	if (!finding_eggs || found_eggs[egg_num-1])
		return;
	if (egg_num%6 == 0)
		r = 255;
	else if (egg_num%6 == 1)
		g = 255;
	else if (egg_num%6 == 2)
		b = 255;
	else if (egg_num%6 == 3) {
		r = 255; g = 255;    }
	else if (egg_num%6 == 4) {
		g = 255; b = 255;    }
	else if (egg_num%6 == 5) {
		r = 255; b = 255;    }
	for (i = 0; i < 5; i++)
	{
		fillcircle(vid, x, y-i*2, 8-i, 8-i, r, g, b, a);
		if (mouse_b && !mouse_bq && (pow((double)mouse_x-x,2)*pow((double)8-i,2)+pow((double)mouse_y-(y-i*2),2)*pow((double)8-i,2)<=pow((double)8-i,2)*pow((double)8-i,2)))
			egg_clicked = 1;
	}
	if (egg_clicked)
	{
		sprintf(text,"You found egg %i",egg_num);
		if (19-eggs_found > 1)
			sprintf(text2,"%i eggs left to find",20-eggs_found-1);
		else if (19-eggs_found == 1)
			sprintf(text2,"1 egg left to find");
		info_ui(vid, text, text2);
		found_eggs[egg_num-1] = 1;
		eggs_found++;
		if (egg_num == 20)
		{
			unlockedstuff |= 0x01;
			SC_TOTAL = SC_CRACKER+1;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's cracker64", "He gives you permanent access to this menu");
			heavy = "This egg is still heavy";
			inside = "You find something else inside";
		}
		if (eggs_found == 5)
		{
			unlockedstuff |= 0x20;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's the old menu", "Press 'o' to activate");
		}
		if (eggs_found == 9)
		{
			unlockedstuff |= 0x02;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's tpt.indestructible(\"\")", "Use to make any element indestructible");
		}
		if (eggs_found == 14)
		{
			unlockedstuff |= 0x04;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's fast open", "Right click an online save to use");
		}
		if (eggs_found == 17)
		{
			unlockedstuff |= 0x08;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's vote show", "Votes are shown on all saves, not just your's");
		}
		if (eggs_found == 20)
		{
			unlockedstuff |= 0x10;
			ptypes[PT_EXPL].menu = 1;
			ptypes[PT_EXPL].enabled = 1;
			info_ui(vid_buf, heavy, inside);
			info_ui(vid_buf, "It's EXPL", "This explosive is the most destructive so far");
			info_ui(vid_buf, "Congradulations!", "You finished the Easter Egg hunt");
		}
	}
}*/

void clearrect(pixel *vid, int x, int y, int w, int h)
{
#ifdef OGLR
	fillrect(vid, x, y, w, h, 0, 0, 0, 255);
#else
	int i;

	if (x+w > XRES+BARSIZE) w = XRES+BARSIZE-x;
	if (y+h > YRES+MENUSIZE) h = YRES+MENUSIZE-y;
	if (x<0)
	{
		w += x;
		x = 0;
	}
	if (y<0)
	{
		h += y;
		y = 0;
	}
	if (w<0 || h<0)
		return;

	for (i=0; i<h; i++)
		memset(vid+(x+(XRES+BARSIZE)*(y+i)), 0, PIXELSIZE*w);
#endif
}
//draws a line of dots, where h is the height. (why is this even here)
void drawdots(pixel *vid, int x, int y, int h, int r, int g, int b, int a)
{
	int i;
	for (i=0; i<=h; i+=2)
		drawpixel(vid, x, y+i, r, g, b, a);
}

int charwidth(unsigned char c)
{
	return font_data[font_ptrs[(int)c]];
}

int textwidth(const char *s)
{
	int x = 0, maxX = 0;
	for (; *s; s++)
		if (*s == '\n')
		{
			x = 0;
		}
		else if (*s == '\x0F')
		{
			s += 3;
		}
		else if (*s == '\x0E')
		{
		}
		else if (*s == '\x01')
		{
		}
		else if (*s == '\b')
		{
			s++;
		}
		else
		{
			x += font_data[font_ptrs[(int)(*(unsigned char *)s)]];
			if (x > maxX)
				maxX = x;
		}
	return maxX-1;
}

int drawtextmax(pixel *vid, int x, int y, int w, char *s, int r, int g, int b, int a)
{
	int i;
	w += x-5;
	for (; *s; s++)
	{
		if (x+font_data[font_ptrs[(int)(*(unsigned char *)s)]]>=w && x+textwidth(s)>=w+5)
			break;
		x = drawchar(vid, x, y, *(unsigned char *)s, r, g, b, a);
	}
	if (*s)
		for (i=0; i<3; i++)
			x = drawchar(vid, x, y, '.', r, g, b, a);
	return x;
}

int textnwidth(char *s, int n)
{
	int x = 0;
	for (; *s; s++)
	{
		if (!n)
			break;
		if (*s == '\x0F')
		{
			s += 3;
			n = std::min(1,n-3);
		}
		else if (*s == '\x0E')
		{
		}
		else if (*s == '\x01')
		{
		}
		else if (*s == '\b')
		{
			s++;
			if (n > 1)
				n--;
		}
		else
			x += font_data[font_ptrs[(int)(*(unsigned char *)s)]];
		n--;
	}
	return x-1;
}
void textnpos(char *s, int n, int w, int *cx, int *cy)
{
	int x = 0;
	int y = 0;
	int wordlen, charspace;
	while (*s&&n)
	{
		wordlen = strcspn(s," .,!?\n");
		charspace = textwidthx(s, w-x);
		if (charspace<wordlen && wordlen && w-x<w/3)
		{
			x = 0;
			y += FONT_H+2;
		}
		for (; *s && --wordlen>=-1; s++)
		{
			if (!n) {
				break;
			}
			if (*s == '\n')
			{
				x = 0;
				y += FONT_H+2;
				continue;
			}
			else if (*s == '\x0F')
			{
				s += 3;
				n = std::min(1,n-3);
			}
			else if (*s == '\x0E')
			{
			}
			else if (*s == '\x01')
			{
			}
			else if (*s == '\b')
			{
				s++;
				if (n > 1)
					n--;
			}
			else
				x += font_data[font_ptrs[(int)(*(unsigned char *)s)]];
			if (x>=w)
			{
				x = 0;
				y += FONT_H+2;
				if (*(s+1)==' ')
					x -= font_data[font_ptrs[(int)(' ')]];
			}
			n--;
		}
	}
	*cx = x-1;
	*cy = y;
}

int textwidthx(char *s, int w)
{
	int x=0,n=0,cw;
	for (; *s; s++)
	{
		if (*s == '\x0F')
		{
			s += 4;
			n += 4;
			if (!*s)
				break;
		}
		else if (*s == '\x0E')
		{
		}
		else if (*s == '\x01')
		{
		}
		else if (*s == '\b')
		{
			s+=2;
			n+=2;
			if (!*s)
				break;
		}
		cw = font_data[font_ptrs[(int)(*(unsigned char *)s)]];
		if (x+(cw/2) >= w)
			break;
		x += cw;
		n++;
	}
	return n;
}
void textsize(char * s, int *width, int *height)
{
	int cHeight = FONT_H, cWidth = 0, lWidth = 0;
	if(!strlen(s))
	{
		*width = 0;
		*height = FONT_H;
		return;
	}

	for (; *s; s++)
	{
		if (*s == '\n')
		{
			cWidth = 0;
			cHeight += FONT_H+2;
		}
		else if (*s == '\x0F')
		{
			if(!s[1] || !s[2] || !s[1]) break;
			s+=3;
		}
		else if (*s == '\b')
		{
			if(!s[1]) break;
			s++;
		}
		else
		{
			cWidth += font_data[font_ptrs[(int)(*(unsigned char *)s)]];
			if(cWidth>lWidth)
				lWidth = cWidth;
		}
	}
	*width = lWidth;
	*height = cHeight;
}
int textposxy(char *s, int width, int w, int h)
{
	int x=0,y=0,n=0,cw, wordlen, charspace;
	while (*s)
	{
		wordlen = strcspn(s," .,!?\n");
		charspace = textwidthx(s, width-x);
		if (charspace<wordlen && wordlen && width-x<width/3)
		{
			x = 0;
			y += FONT_H+2;
		}
		for (; *s && --wordlen>=-1; s++)
		{
			if (*s == '\n')
			{
				x = 0;
				y += FONT_H+2;
				continue;
			}
			else if (*s == '\x0F')
			{
				s += 4;
				n += 4;
				if (!*s)
					return n;
			}
			else if (*s == '\x0E')
			{
			}
			else if (*s == '\x01')
			{
			}
			else if (*s == '\b')
			{
				s+=2;
				n+=2;
				if (!*s)
					break;
			}
			cw = font_data[font_ptrs[(int)(*(unsigned char *)s)]];
			if ((x+(cw/2) >= w && y+6 >= h)||(y+6 >= h+FONT_H+2))
				return n++;
			x += cw;
			if (x>=width) {
				x = 0;
				y += FONT_H+2;
				if (*(s+1)==' ')
					x -= font_data[font_ptrs[(int)(' ')]];
			}
			n++;
		}
	}
	return n;
}
int textwrapheight(char *s, int width)
{
	int x=0, height=FONT_H+2, cw = 0;
	int wordlen;
	int charspace;
	while (*s)
	{
		wordlen = strcspn(s," .,!?\n");
		charspace = textwidthx(s, width-x);
		if (charspace<wordlen && wordlen && width-x<width/3)
		{
			x = 0;
			height += FONT_H+2;
		}
		for (; *s && --wordlen>=-1; s++)
		{
			if (*s == '\n')
			{
				x = 0;
				height += FONT_H+2;
			}
			else if (*s == '\x0F')
			{
				s += 3;
			}
			else if (*s == '\x0E')
			{
			}
			else if (*s == '\x01')
			{
			}
			else if (*s == '\b')
			{
				s++;
			}
			else
			{
				if (x-cw>=width)
				{
					x = 0;
					height += FONT_H+2;
					if (*s==' ')
						continue;
				}
				cw = font_data[font_ptrs[(int)(*(unsigned char *)s)]];
				x += cw;
			}
		}
	}
	return height;
}

//the most used function for drawing a pixel, because it has OpenGL support, which is not fully implemented.
TPT_INLINE void blendpixel(pixel *vid, int x, int y, int r, int g, int b, int a)
{
#ifdef PIXALPHA
	pixel t;
	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return;
	if (a!=255)
	{
		t = vid[y*(XRES+BARSIZE)+x];
		r = (a*r + (255-a)*PIXR(t)) >> 8;
		g = (a*g + (255-a)*PIXG(t)) >> 8;
		b = (a*b + (255-a)*PIXB(t)) >> 8;
		a = a > PIXA(t) ? a : PIXA(t);
	}
	vid[y*(XRES+BARSIZE)+x] = PIXRGBA(r,g,b,a);
#else
	pixel t;
	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return;
	if (a!=255)
	{
		t = vid[y*(XRES+BARSIZE)+x];
		r = (a*r + (255-a)*PIXR(t)) >> 8;
		g = (a*g + (255-a)*PIXG(t)) >> 8;
		b = (a*b + (255-a)*PIXB(t)) >> 8;
	}
	vid[y*(XRES+BARSIZE)+x] = PIXRGB(r,g,b);
#endif
}

void draw_icon(pixel *vid_buf, int x, int y, char ch, int flag)
{
	char t[2];
	t[0] = ch;
	t[1] = 0;
	if (flag)
	{
		fillrect(vid_buf, x-1, y-1, 17, 17, 255, 255, 255, 255);
		drawtext(vid_buf, x+3, y+2, t, 0, 0, 0, 255);
	}
	else
	{
		drawrect(vid_buf, x, y, 15, 15, 255, 255, 255, 255);
		drawtext(vid_buf, x+3, y+2, t, 255, 255, 255, 255);
	}
}
void draw_air(pixel *vid)
{
#ifndef OGLR
	int x, y, i, j;
	pixel c;
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
		{
			if (display_mode & DISPLAY_AIRP)
			{
				if (pv[y][x] > 0.0f)
					c  = PIXRGB(clamp_flt(pv[y][x], 0.0f, 8.0f), 0, 0);//positive pressure is red!
				else
					c  = PIXRGB(0, 0, clamp_flt(-pv[y][x], 0.0f, 8.0f));//negative pressure is blue!
			}
			else if (display_mode & DISPLAY_AIRV)
			{
				c  = PIXRGB(clamp_flt(fabsf(vx[y][x]), 0.0f, 8.0f),//vx adds red
				clamp_flt(pv[y][x], 0.0f, 8.0f),//pressure adds green
				clamp_flt(fabsf(vy[y][x]), 0.0f, 8.0f));//vy adds blue
			}
			else if (display_mode & DISPLAY_AIRH)
			{
				if (!aheat_enable)
					c = 0;
				else
				{
					float ttemp = hv[y][x]+(-MIN_TEMP);
					int caddress = (int)restrict_flt((int)( restrict_flt(ttemp, 0.0f, (float)MAX_TEMP+(-MIN_TEMP)) / ((MAX_TEMP+(-MIN_TEMP))/1024) ) *3.0f, 0.0f, (1024.0f*3)-3);
					c = PIXRGB((int)((unsigned char)colour_data[caddress]*0.7f), (int)((unsigned char)colour_data[caddress+1]*0.7f), (int)((unsigned char)colour_data[caddress+2]*0.7f));
				}
			}
			else if (display_mode & DISPLAY_AIRC)
			{
				int r;
				int g;
				int b;
				// velocity adds grey
				r = clamp_flt(fabsf(vx[y][x]), 0.0f, 24.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 20.0f);
				g = clamp_flt(fabsf(vx[y][x]), 0.0f, 20.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 24.0f);
				b = clamp_flt(fabsf(vx[y][x]), 0.0f, 24.0f) + clamp_flt(fabsf(vy[y][x]), 0.0f, 20.0f);
				if (pv[y][x] > 0.0f)
				{
					r += clamp_flt(pv[y][x], 0.0f, 16.0f);//pressure adds red!
					if (r>255)
						r=255;
					if (g>255)
						g=255;
					if (b>255)
						b=255;
					c  = PIXRGB(r, g, b);
				}
				else
				{
					b += clamp_flt(-pv[y][x], 0.0f, 16.0f);//pressure adds blue!
					if (r>255)
						r=255;
					if (g>255)
						g=255;
					if (b>255)
						b=255;
					c  = PIXRGB(r, g, b);
				}
			}
			if (finding && !(finding & 0x8))
			{
				c = PIXRGB(PIXR(c)/10,PIXG(c)/10,PIXB(c)/10);
			}
			for (j=0; j<CELL; j++)//draws the colours
				for (i=0; i<CELL; i++)
					vid[(x*CELL+i) + (y*CELL+j)*(XRES+BARSIZE)] = c;
		}
#else
	GLuint airProg;
	if(display_mode & DISPLAY_AIRC)
	{
		airProg = airProg_Cracker;
	}
	else if(display_mode & DISPLAY_AIRV)
	{
		airProg = airProg_Velocity;
	}
	else if(display_mode & DISPLAY_AIRP)
	{
		airProg = airProg_Pressure;
	}
	else
	{
		return;
	}

    glEnable( GL_TEXTURE_2D );
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
    
	glUseProgram(airProg);
	
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, airVX);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_RED, GL_FLOAT, vx);
    glUniform1i(glGetUniformLocation(airProg, "airX"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, airVY);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_GREEN, GL_FLOAT, vy);
    glUniform1i(glGetUniformLocation(airProg, "airY"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, airPV);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_BLUE, GL_FLOAT, pv);
    glUniform1i(glGetUniformLocation(airProg, "airP"), 2);
    glActiveTexture(GL_TEXTURE0);
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
    glTexCoord2d(1, 1);
    glVertex3f(XRES*sdl_scale, YRES*sdl_scale, 1.0);
    glTexCoord2d(0, 1);
    glVertex3f(0, YRES*sdl_scale, 1.0);
    glTexCoord2d(0, 0);
    glVertex3f(0, 0, 1.0);
    glTexCoord2d(1, 0);
    glVertex3f(XRES*sdl_scale, 0, 1.0);
    glEnd();
    
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable( GL_TEXTURE_2D );
#endif
}

void draw_grav_zones(pixel * vid)
{
	int x, y, i, j;
	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			if(gravmask[y*(XRES/CELL)+x])
			{
				for (j=0; j<CELL; j++)//draws the colours
					for (i=0; i<CELL; i++)
						if(i == j)
							drawpixel(vid, x*CELL+i, y*CELL+j, 255, 200, 0, 120);
						else 
							drawpixel(vid, x*CELL+i, y*CELL+j, 32, 32, 32, 120);
			}
		}
	}
}

void draw_grav(pixel *vid)
{
	int x, y, i, ca;
	float nx, ny, dist;

	for (y=0; y<YRES/CELL; y++)
	{
		for (x=0; x<XRES/CELL; x++)
		{
			ca = y*(XRES/CELL)+x;
			if(fabsf(gravx[ca]) <= 0.001f && fabsf(gravy[ca]) <= 0.001f)
				continue;
			nx = (float)x*CELL;
			ny = (float)y*CELL;
			dist = fabsf(gravy[ca])+fabsf(gravx[ca]);
			for(i = 0; i < 4; i++)
			{
				nx -= gravx[ca]*0.5f;
				ny -= gravy[ca]*0.5f;
				addpixel(vid, (int)(nx+0.5f), (int)(ny+0.5f), 255, 255, 255, (int)(dist*20.0f));
			}
		}
	}
}

void draw_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int screenwidth)  //Draws a line
{
	int dx, dy, i, sx, sy, check, e, x, y;

	dx = abs(x1-x2);
	dy = abs(y1-y2);
	sx = isign((float)x2-x1);
	sy = isign((float)y2-y1);
	x = x1;
	y = y1;
	check = 0;

	if (dy > dx)
	{
		dx = dx + dy;
		dy = dx - dy;
		dx = dx - dy;
		check = 1;
	}

	e = (dy<<2) - dx;
	for (i = 0; i <= dx; i++)
	{
		if (x>=0 && y>=0 && x<screenwidth && y<YRES+MENUSIZE)
			vid[x + y*screenwidth] = PIXRGB(r, g, b);
		if (e >= 0)
		{
			if (check == 1)
				x = x + sx;
			else
				y = y + sy;
			e = e - (dx<<2);
		}
		if (check == 1)
			y = y + sy;
		else
			x = x + sx;
		e = e + (dy<<2);
	}
}

//adds colour to a pixel, does not overwrite.
void addpixel(pixel *vid, int x, int y, int r, int g, int b, int a)
{
	pixel t;
	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return;
	t = vid[y*(XRES+BARSIZE)+x];
	r = (a*r + 255*PIXR(t)) >> 8;
	g = (a*g + 255*PIXG(t)) >> 8;
	b = (a*b + 255*PIXB(t)) >> 8;
	if (r>255)
		r = 255;
	if (g>255)
		g = 255;
	if (b>255)
		b = 255;
	vid[y*(XRES+BARSIZE)+x] = PIXRGB(r,g,b);
}

//draws one of two colours, so that it is always clearly visible
void xor_pixel(int x, int y, pixel *vid)
{
	int c;
	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return;
	c = vid[y*(XRES+BARSIZE)+x];
	c = PIXB(c) + 3*PIXG(c) + 2*PIXR(c);
	if (c<512)
		vid[y*(XRES+BARSIZE)+x] = PIXPACK(0xC0C0C0);
	else
		vid[y*(XRES+BARSIZE)+x] = PIXPACK(0x404040);
}

//same as xor_pixel, but draws a line of it
void xor_line(int x1, int y1, int x2, int y2, pixel *vid)
{
	int cp=abs(y2-y1)>abs(x2-x1), x, y, dx, dy, sy;
	float e, de;
	if (cp)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (cp)
			xor_pixel(y, x, vid);
		else
			xor_pixel(x, y, vid);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			e -= 1.0f;
		}
	}
}

//same as blend_pixel, but draws a line of it
void blend_line(pixel *vid, int x1, int y1, int x2, int y2, int r, int g, int b, int a)
{
	int cp=abs(y2-y1)>abs(x2-x1), x, y, dx, dy, sy;
	float e, de;
	if (cp)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	e = 0.0f;
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (cp)
			blendpixel(vid, y, x, r, g, b, a);
		else
			blendpixel(vid, x, y, r, g, b, a);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			e -= 1.0f;
		}
	}
}

//same as xor_pixel, but draws a rectangle
void xor_rect(pixel *vid, int x, int y, int w, int h)
{
	int i;
	for (i=0; i<w; i+=2)
	{
		xor_pixel(x+i, y, vid);
	}
	if (h != 1)
	{
		if (h%2 == 1) i = 2;
		else i = 1;
		for (; i<w; i+=2)
		{
			xor_pixel(x+i, y+h-1, vid);
		}
	}

	for (i=2; i<h; i+=2)
	{
		xor_pixel(x, y+i, vid);
	}
	if (w != 1)
	{
		if (w%2 == 1) i = 2;
		else i = 1;
		for (; i<h-1; i+=2)
		{
			xor_pixel(x+w-1, y+i, vid);
		}
	}
}

void draw_other(pixel *vid) // EMP effect
{
	if (!sys_pause || framerender)
		((EMP_ElementDataContainer*)globalSim->elementData[PT_EMP])->Deactivate();
	if (!(render_mode & EFFECT)) // not in nothing mode
		return;

	int emp_decor = ((EMP_ElementDataContainer*)globalSim->elementData[PT_EMP])->emp_decor;
	if (emp_decor)
	{
#ifdef OGLR
		float femp_decor = ((float)emp_decor)/255.0f;
		/*int r=emp_decor*2.5, g=100+emp_decor*1.5, b=255;
		int a=(1.0*emp_decor/110)*255;
		if (r>255) r=255;
		if (g>255) g=255;
		if (b>255) g=255;
		if (a>255) a=255;*/
		glBegin(GL_QUADS);
		glColor4f(femp_decor*2.5f, 0.4f+femp_decor*1.5f, 1.0f+femp_decor*1.5f, femp_decor/0.44f);
		glVertex2f(0, MENUSIZE);
		glVertex2f(XRES, MENUSIZE);
		glVertex2f(XRES, YRES+MENUSIZE);
		glVertex2f(0, YRES+MENUSIZE);
		glEnd();
#else
		int r=(int)(emp_decor*2.5), g=(int)(100+emp_decor*1.5), b=255;
		int a=(int)((1.0*emp_decor/110)*255);
		if (r>255) r=255;
		if (g>255) g=255;
		if (b>255) g=255;
		if (a>255) a=255;
		for (int j = 0; j < YRES; j++)
			for (int i = 0; i < XRES; i++)
			{
				drawpixel(vid, i, j, r, g, b, a);
			}
#endif
	}
}

void prepare_graphicscache()
{
	graphicscache = (gcache_item*)malloc(sizeof(gcache_item)*PT_NUM);
	memset(graphicscache, 0, sizeof(gcache_item)*PT_NUM);
}
//New function for drawing particles
#ifdef OGLR
GLuint fireV[(YRES*XRES)*2];
GLfloat fireC[(YRES*XRES)*4];
GLuint smokeV[(YRES*XRES)*2];
GLfloat smokeC[(YRES*XRES)*4];
GLuint blobV[(YRES*XRES)*2];
GLfloat blobC[(YRES*XRES)*4];
GLuint blurV[(YRES*XRES)*2];
GLfloat blurC[(YRES*XRES)*4];
GLuint glowV[(YRES*XRES)*2];
GLfloat glowC[(YRES*XRES)*4];
GLuint flatV[(YRES*XRES)*2];
GLfloat flatC[(YRES*XRES)*4];
GLuint addV[(YRES*XRES)*2];
GLfloat addC[(YRES*XRES)*4];
GLfloat lineV[(((YRES*XRES)*2)*6)];
GLfloat lineC[(((YRES*XRES)*2)*6)];
GLfloat blurLineV[(((YRES*XRES)*2))];
GLfloat blurLineC[(((YRES*XRES)*2)*4)];
GLfloat ablurLineV[(((YRES*XRES)*2))];
GLfloat ablurLineC[(((YRES*XRES)*2)*4)];
#endif
void render_parts(pixel *vid, Point mousePos)
{
	Simulation *sim = globalSim;
	int deca, decr, decg, decb, cola, colr, colg, colb, firea, firer = 0, fireg = 0, fireb = 0, pixel_mode, q, i, t, nx, ny, x, y, caddress;
	int orbd[4] = {0, 0, 0, 0}, orbl[4] = {0, 0, 0, 0};
	float gradv, flicker, fnx, fny;
#ifdef OGLR
	int cfireV = 0, cfireC = 0, cfire = 0;
	int csmokeV = 0, csmokeC = 0, csmoke = 0;
	int cblobV = 0, cblobC = 0, cblob = 0;
	int cblurV = 0, cblurC = 0, cblur = 0;
	int cglowV = 0, cglowC = 0, cglow = 0;
	int cflatV = 0, cflatC = 0, cflat = 0;
	int caddV = 0, caddC = 0, cadd = 0;
	int clineV = 0, clineC = 0, cline = 0;
	int cblurLineV = 0, cblurLineC = 0, cblurLine = 0;
	int cablurLineV = 0, cablurLineC = 0, cablurLine = 0;
	GLuint origBlendSrc, origBlendDst;
	
	glGetIntegerv(GL_BLEND_SRC, &origBlendSrc);
	glGetIntegerv(GL_BLEND_DST, &origBlendDst);
	//Render to the particle FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
#else
	if (GRID_MODE)//draws the grid
	{
		for (ny=0; ny<YRES; ny++)
			for (nx=0; nx<XRES; nx++)
			{
				if (ny%(4*GRID_MODE) == 0)
					blendpixel(vid, nx, ny, 100, 100, 100, 80);
				if (nx%(4*GRID_MODE) == 0 && ny%(4*GRID_MODE) != 0)
					blendpixel(vid, nx, ny, 100, 100, 100, 80);
			}
	}
#endif
	for(i = 0; i <= globalSim->parts_lastActiveIndex; i++) {
		if (parts[i].type) {
			t = parts[i].type;
			if (t == PT_PINV && parts[i].tmp2 && (parts[i].tmp2>>8)<i)
				continue;

			nx = (int)(parts[i].x+0.5f);
			ny = (int)(parts[i].y+0.5f);
			if (nx < 0 || nx > XRES || ny < 0 || ny > YRES)
				continue;
			fnx = parts[i].x;
			fny = parts[i].y;
			if ((pmap[ny][nx]&0xFF) == PT_PINV)
				parts[pmap[ny][nx]>>8].tmp2 = t|(i<<8);
#ifdef OGLR
			float flx = parts[i].lastX;
			float fly = parts[i].lastY;
#endif

			if(photons[ny][nx]&0xFF && !(ptypes[t].properties & TYPE_ENERGY) && t!=PT_STKM && t!=PT_STKM2 && t!=PT_FIGH)
				continue;
				
			//Defaults
			pixel_mode = 0 | PMODE_FLAT;
			cola = COLA(globalSim->elements[t].Colour); // always 255
			colr = COLR(globalSim->elements[t].Colour);
			colg = COLG(globalSim->elements[t].Colour);
			colb = COLB(globalSim->elements[t].Colour);
			firea = 0;
			
			deca = COLA(parts[i].dcolour);
			decr = COLR(parts[i].dcolour);
			decg = COLG(parts[i].dcolour);
			decb = COLB(parts[i].dcolour);
				
			/*if(display_mode == RENDER_NONE)
			{
				if(decorations_enable)
				{
					colr = (deca*decr + (255-deca)*colr) >> 8;
					colg = (deca*decg + (255-deca)*colg) >> 8;
					colb = (deca*decb + (255-deca)*colb) >> 8;
				}
#ifdef OGLR
		        flatV[cflatV++] = nx;
		        flatV[cflatV++] = ny;
		        flatC[cflatC++] = ((float)colr)/255.0f;
		        flatC[cflatC++] = ((float)colg)/255.0f;
		        flatC[cflatC++] = ((float)colb)/255.0f;
		        flatC[cflatC++] = 1.0f;
		        cflat++;
#else
		        vid[ny*(XRES+BARSIZE)+nx] = PIXRGB(colr,colg,colb);
#endif
			}
			else*/
			{	
				if (graphicscache[t].isready)
				{
					pixel_mode = graphicscache[t].pixel_mode;
					cola = graphicscache[t].cola;
					colr = graphicscache[t].colr;
					colg = graphicscache[t].colg;
					colb = graphicscache[t].colb;
					firea = graphicscache[t].firea;
					firer = graphicscache[t].firer;
					fireg = graphicscache[t].fireg;
					fireb = graphicscache[t].fireb;
				}
				else if(!(colour_mode & COLOUR_BASC))	//Don't get special effects for BASIC colour mode
				{
#ifdef LUACONSOLE
					if (lua_gr_func[t])
					{
						if (luacon_graphics_update(t,i, &pixel_mode, &cola, &colr, &colg, &colb, &firea, &firer, &fireg, &fireb))
						{
							graphicscache[t].isready = 1;
							graphicscache[t].pixel_mode = pixel_mode;
							graphicscache[t].cola = cola;
							graphicscache[t].colr = colr;
							graphicscache[t].colg = colg;
							graphicscache[t].colb = colb;
							graphicscache[t].firea = firea;
							graphicscache[t].firer = firer;
							graphicscache[t].fireg = fireg;
							graphicscache[t].fireb = fireb;
						}
					}
					else if (ptypes[t].graphics_func)
					{
#else
					if (ptypes[t].graphics_func)
					{
#endif
						if ((*(ptypes[t].graphics_func))(sim, &(parts[i]), nx, ny, &pixel_mode, &cola, &colr, &colg, &colb, &firea, &firer, &fireg, &fireb)) //That's a lot of args, a struct might be better
						{
							graphicscache[t].isready = 1;
							graphicscache[t].pixel_mode = pixel_mode;
							graphicscache[t].cola = cola;
							graphicscache[t].colr = colr;
							graphicscache[t].colg = colg;
							graphicscache[t].colb = colb;
							graphicscache[t].firea = firea;
							graphicscache[t].firer = firer;
							graphicscache[t].fireg = fireg;
							graphicscache[t].fireb = fireb;
						}
#ifdef LUACONSOLE
					}
#else
					}
#endif
					else
					{
						if(graphics_DEFAULT(sim, &(parts[i]), nx, ny, &pixel_mode, &cola, &colr, &colg, &colb, &firea, &firer, &fireg, &fireb))
						{
							graphicscache[t].isready = 1;
							graphicscache[t].pixel_mode = pixel_mode;
							graphicscache[t].cola = cola;
							graphicscache[t].colr = colr;
							graphicscache[t].colg = colg;
							graphicscache[t].colb = colb;
							graphicscache[t].firea = firea;
							graphicscache[t].firer = firer;
							graphicscache[t].fireg = fireg;
							graphicscache[t].fireb = fireb;
						}
					}
				}
				if(ptypes[t].properties & PROP_HOT_GLOW && parts[i].temp>(ptransitions[t].thv-800.0f))
				{
					gradv = M_PI/(2*ptransitions[t].thv-(ptransitions[t].thv-800.0f));
					caddress = (int)((parts[i].temp>ptransitions[t].thv)?ptransitions[t].thv-(ptransitions[t].thv-800.0f):parts[i].temp-(ptransitions[t].thv-800.0f));
					colr += (int)(sin(gradv*caddress) * 226);
					colg += (int)(sin(gradv*caddress*4.55 +3.14) * 34);
					colb += (int)(sin(gradv*caddress*2.22 +3.14) * 64);
				}
				
				if(pixel_mode & FIRE_ADD && !(render_mode & FIRE_ADD))
					pixel_mode |= PMODE_GLOW;
				if(pixel_mode & FIRE_BLEND && !(render_mode & FIRE_BLEND))
					pixel_mode |= PMODE_BLUR;
				if(pixel_mode & PMODE_BLUR && !(render_mode & PMODE_BLUR))
					pixel_mode |= PMODE_FLAT;
				if(pixel_mode & PMODE_GLOW && !(render_mode & PMODE_GLOW))
					pixel_mode |= PMODE_BLEND;
				if (render_mode & PMODE_BLOB)
					pixel_mode |= PMODE_BLOB;
					
				pixel_mode &= render_mode;
				
				//Alter colour based on display mode
				if(colour_mode & COLOUR_HEAT)
				{
					if (heatmode == 0)
						caddress = (int)restrict_flt((int)( restrict_flt((float)(parts[i].temp+(-MIN_TEMP)), 0.0f, MAX_TEMP+(-MIN_TEMP)) / ((MAX_TEMP+(-MIN_TEMP))/1024) ) *3.0f, 0.0f, (1024.0f*3)-3); //Not having that second (float) might be a bug, and is definetely needed if min&max temps are less than 1024 apart
					else
						caddress = (int)restrict_flt((int)( restrict_flt((float)(parts[i].temp+(-lowesttemp)), 0.0f, (float)highesttemp+(-lowesttemp)) / ((float)(highesttemp+(-lowesttemp))/1024) ) *3.0f, 0.0f, (1024.0f*3)-3);
					firea = 255;
					firer = colr = (unsigned char)colour_data[caddress];
					fireg = colg = (unsigned char)colour_data[caddress+1];
					fireb = colb = (unsigned char)colour_data[caddress+2];
					cola = 255;
					if(pixel_mode & (FIREMODE | PMODE_GLOW))
						pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
					else if (!pixel_mode)
						pixel_mode |= PMODE_FLAT;
				}
				else if(colour_mode & COLOUR_LIFE)
				{
					gradv = 0.4f;
					if (!(parts[i].life<5))
						q = (int)sqrtf((float)parts[i].life);
					else
						q = parts[i].life;
					colr = colg = colb = (int)(sin(gradv*q) * 100 + 128);
					cola = 255;
					if(pixel_mode & (FIREMODE | PMODE_GLOW))
						pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
					else if (!pixel_mode)
						pixel_mode |= PMODE_FLAT;
				}
				else if (colour_mode & COLOUR_BASC)
				{
					colr = COLR(globalSim->elements[t].Colour);
					colg = COLG(globalSim->elements[t].Colour);
					colb = COLB(globalSim->elements[t].Colour);
					pixel_mode = PMODE_FLAT;
				}

				if(!(colour_mode & ~COLOUR_GRAD))
				{
					if(!(pixel_mode & NO_DECO) && decorations_enable)
					{
						colr = (deca*decr + (255-deca)*colr) >> 8;
						colg = (deca*decg + (255-deca)*colg) >> 8;
						colb = (deca*decb + (255-deca)*colb) >> 8;
					}
					
					if(pixel_mode & DECO_FIRE && decorations_enable)
					{
						firer = (deca*decr + (255-deca)*firer) >> 8;
						fireg = (deca*decg + (255-deca)*fireg) >> 8;
						fireb = (deca*decb + (255-deca)*fireb) >> 8;
					}
				}

				if (finding && !(finding & 0x8))
				{
					if ((finding & 0x1) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[0])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[0])->GetID() == parts[i].ctype)))
					{
						colr = firer = 255;
						colg = colb = fireg = fireb = 0;
						cola = firea = 255;
					}
					else if ((finding & 0x2) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[1])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[1])->GetID() == parts[i].ctype)))
					{
						colb = fireb = 255;
						colr = colg = firer = fireg = 0;
						cola = firea = 255;
					}
					else if ((finding & 0x4) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[2])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[2])->GetID() == parts[i].ctype)))
					{
						colg = fireg = 255;
						colr = colb = firer = fireb = 0;
						cola = firea = 255;
					}
					else
					{
						colr /= 10;
						colg /= 10;
						colb /= 10;
						firer /= 5;
						fireg /= 5;
						fireb /= 5;
						if (colr + colg + colg < 10)
							colr = colg = colb = 20;
						if (firer + fireg + fireg < 35)
							firer = fireg = fireb = 65;
						}
				}

				if (colour_mode & COLOUR_GRAD)
				{
					float frequency = 0.05f;
					int q = (int)parts[i].temp-40;
					colr = (int)(sin(frequency*q) * 16 + colr);
					colg = (int)(sin(frequency*q) * 16 + colg);
					colb = (int)(sin(frequency*q) * 16 + colb);
					if(pixel_mode & (FIREMODE | PMODE_GLOW)) pixel_mode = (pixel_mode & ~(FIREMODE|PMODE_GLOW)) | PMODE_BLUR;
				}

	#ifndef OGLR
				//All colours are now set, check ranges
				if(colr>255) colr = 255;
				else if(colr<0) colr = 0;
				if(colg>255) colg = 255;
				else if(colg<0) colg = 0;
				if(colb>255) colb = 255;
				else if(colb<0) colb = 0;
				if(cola>255) cola = 255;
				else if(cola<0) cola = 0;

				if(firer>255) firer = 255;
				else if(firer<0) firer = 0;
				if(fireg>255) fireg = 255;
				else if(fireg<0) fireg = 0;
				if(fireb>255) fireb = 255;
				else if(fireb<0) fireb = 0;
				if(firea>255) firea = 255;
				else if(firea<0) firea = 0;
	#endif

				//Pixel rendering
				if (t==PT_SOAP) //pixel_mode & EFFECT_LINES, pointless to check if only soap has it ...
				{
					if ((parts[i].ctype&3) == 3 && parts[i].tmp >= 0 && parts[i].tmp < NPART)
						draw_line(vid, nx, ny, (int)(parts[parts[i].tmp].x+0.5f), (int)(parts[parts[i].tmp].y+0.5f), colr, colg, colb, XRES+BARSIZE);
				}
				if(pixel_mode & PSPEC_STICKMAN)
				{
					char buff[20];  //Buffer for HP
					int s;
					int legr, legg, legb;
					playerst *cplayer;
					if(t==PT_STKM)
						cplayer = &player;
					else if(t==PT_STKM2)
						cplayer = &player2;
					else if(t==PT_FIGH && parts[i].tmp >= 0 && parts[i].tmp < ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->MaxFighters())
						cplayer = ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->Get(parts[i].tmp);
					else
						continue;

					if (mousePos.X>nx-3 && mousePos.X<nx+3 && mousePos.Y<ny+3 && mousePos.Y>ny-3) //If mouse is in the head
					{
						sprintf(buff, "%3d", parts[i].life);  //Show HP
						drawtext(vid, mousePos.X-8-2*(parts[i].life<100)-2*(parts[i].life<10), mousePos.Y-12, buff, 255, 255, 255, 255);
					}

					if (colour_mode!=COLOUR_HEAT && !(finding & ~0x8))
					{
						if (cplayer->elem<PT_NUM)
						{
							colr = COLR(globalSim->elements[cplayer->elem].Colour);
							colg = COLG(globalSim->elements[cplayer->elem].Colour);
							colb = COLB(globalSim->elements[cplayer->elem].Colour);
						}
						else
						{
							colr = 0x80;
							colg = 0x80;
							colb = 0xFF;
						}
					}
#ifdef OGLR
					glColor4f(((float)colr)/255.0f, ((float)colg)/255.0f, ((float)colb)/255.0f, 1.0f);
					glBegin(GL_LINE_STRIP);
					if(t==PT_FIGH)
					{
						glVertex2f(fnx, fny+2);
						glVertex2f(fnx+2, fny);
						glVertex2f(fnx, fny-2);
						glVertex2f(fnx-2, fny);
						glVertex2f(fnx, fny+2);
					}
					else
					{
						glVertex2f(fnx-2, fny-2);
						glVertex2f(fnx+2, fny-2);
						glVertex2f(fnx+2, fny+2);
						glVertex2f(fnx-2, fny+2);
						glVertex2f(fnx-2, fny-2);
					}
					glEnd();
					glBegin(GL_LINES);

					if (colour_mode!=COLOUR_HEAT)
					{
						if (t==PT_STKM2)
							glColor4f(100.0f/255.0f, 100.0f/255.0f, 1.0f, 1.0f);
						else
							glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}

					glVertex2f(nx, ny+3);
					glVertex2f(cplayer->legs[0], cplayer->legs[1]);
					
					glVertex2f(cplayer->legs[0], cplayer->legs[1]);
					glVertex2f(cplayer->legs[4], cplayer->legs[5]);
					
					glVertex2f(nx, ny+3);
					glVertex2f(cplayer->legs[8], cplayer->legs[9]);
					
					glVertex2f(cplayer->legs[8], cplayer->legs[9]);
					glVertex2f(cplayer->legs[12], cplayer->legs[13]);
					glEnd();
#else
					s = XRES+BARSIZE;

					if (t==PT_STKM2)
					{
						legr = 100;
						legg = 100;
						legb = 255;
					}
					else
					{
						legr = 255;
						legg = 255;
						legb = 255;
					}

					if (colour_mode==COLOUR_HEAT || (finding & ~0x8))
					{
						legr = colr;
						legg = colg;
						legb = colb;
					}

					//head
					if(t==PT_FIGH)
					{
						draw_line(vid , nx, ny+2, nx+2, ny, colr, colg, colb, s);
						draw_line(vid , nx+2, ny, nx, ny-2, colr, colg, colb, s);
						draw_line(vid , nx, ny-2, nx-2, ny, colr, colg, colb, s);
						draw_line(vid , nx-2, ny, nx, ny+2, colr, colg, colb, s);
					}
					else
					{
						draw_line(vid , nx-2, ny+2, nx+2, ny+2, colr, colg, colb, s);
						draw_line(vid , nx-2, ny-2, nx+2, ny-2, colr, colg, colb, s);
						draw_line(vid , nx-2, ny-2, nx-2, ny+2, colr, colg, colb, s);
						draw_line(vid , nx+2, ny-2, nx+2, ny+2, colr, colg, colb, s);
					}
					//legs
					draw_line(vid , nx, ny+3, (int)cplayer->legs[0], (int)cplayer->legs[1], legr, legg, legb, s);
					draw_line(vid , (int)cplayer->legs[0], (int)cplayer->legs[1], (int)cplayer->legs[4], (int)cplayer->legs[5], legr, legg, legb, s);
					draw_line(vid , nx, ny+3, (int)cplayer->legs[8], (int)cplayer->legs[9], legr, legg, legb, s);
					draw_line(vid , (int)cplayer->legs[8], (int)cplayer->legs[9], (int)cplayer->legs[12], (int)cplayer->legs[13], legr, legg, legb, s);
					if (cplayer->rocketBoots)
					{
						int leg;
						for (leg=0; leg<2; leg++)
						{
							int nx = (int)cplayer->legs[leg*8+4], ny = (int)cplayer->legs[leg*8+5];
							int colr = 255, colg = 0, colb = 255;
							if (((int)(cplayer->comm)&0x04) == 0x04 || (((int)(cplayer->comm)&0x01) == 0x01 && leg==0) || (((int)(cplayer->comm)&0x02) == 0x02 && leg==1))
								blendpixel(vid, nx, ny, 0, 255, 0, 255);
							else
								blendpixel(vid, nx, ny, 255, 0, 0, 255);
							blendpixel(vid, nx+1, ny, colr, colg, colb, 223);
							blendpixel(vid, nx-1, ny, colr, colg, colb, 223);
							blendpixel(vid, nx, ny+1, colr, colg, colb, 223);
							blendpixel(vid, nx, ny-1, colr, colg, colb, 223);

							blendpixel(vid, nx+1, ny-1, colr, colg, colb, 112);
							blendpixel(vid, nx-1, ny-1, colr, colg, colb, 112);
							blendpixel(vid, nx+1, ny+1, colr, colg, colb, 112);
							blendpixel(vid, nx-1, ny+1, colr, colg, colb, 112);
						}
					}
#endif
				}
#ifdef OGLR
				if((display_mode & DISPLAY_EFFE) && (fabs(fnx-flx)>1.5f || fabs(fny-fly)>1.5f))
				{
					if(pixel_mode & PMODE_FLAT)
					{
						blurLineV[cblurLineV++] = nx;
						blurLineV[cblurLineV++] = ny;
						blurLineC[cblurLineC++] = ((float)colr)/255.0f;
						blurLineC[cblurLineC++] = ((float)colg)/255.0f;
						blurLineC[cblurLineC++] = ((float)colb)/255.0f;
						blurLineC[cblurLineC++] = 1.0f;
						cblurLine++;
						
						blurLineV[cblurLineV++] = flx;
						blurLineV[cblurLineV++] = fly;
						blurLineC[cblurLineC++] = ((float)colr)/255.0f;
						blurLineC[cblurLineC++] = ((float)colg)/255.0f;
						blurLineC[cblurLineC++] = ((float)colb)/255.0f;
						blurLineC[cblurLineC++] = 0.0f;
						cblurLine++;
					}
					else if(pixel_mode & PMODE_BLEND)
					{
						blurLineV[cblurLineV++] = nx;
						blurLineV[cblurLineV++] = ny;
						blurLineC[cblurLineC++] = ((float)colr)/255.0f;
						blurLineC[cblurLineC++] = ((float)colg)/255.0f;
						blurLineC[cblurLineC++] = ((float)colb)/255.0f;
						blurLineC[cblurLineC++] = ((float)cola)/255.0f;
						cblurLine++;
						
						blurLineV[cblurLineV++] = flx;
						blurLineV[cblurLineV++] = fly;
						blurLineC[cblurLineC++] = ((float)colr)/255.0f;
						blurLineC[cblurLineC++] = ((float)colg)/255.0f;
						blurLineC[cblurLineC++] = ((float)colb)/255.0f;
						blurLineC[cblurLineC++] = 0.0f;
						cblurLine++;
					}
					else if(pixel_mode & PMODE_ADD)
					{
						ablurLineV[cablurLineV++] = nx;
						ablurLineV[cablurLineV++] = ny;
						ablurLineC[cablurLineC++] = ((float)colr)/255.0f;
						ablurLineC[cablurLineC++] = ((float)colg)/255.0f;
						ablurLineC[cablurLineC++] = ((float)colb)/255.0f;
						ablurLineC[cablurLineC++] = ((float)cola)/255.0f;
						cablurLine++;
						
						ablurLineV[cablurLineV++] = flx;
						ablurLineV[cablurLineV++] = fly;
						ablurLineC[cablurLineC++] = ((float)colr)/255.0f;
						ablurLineC[cablurLineC++] = ((float)colg)/255.0f;
						ablurLineC[cablurLineC++] = ((float)colb)/255.0f;
						ablurLineC[cablurLineC++] = 0.0f;
						cablurLine++;
					}
				}
#endif
				if(pixel_mode & PMODE_FLAT)
				{
#ifdef OGLR
                    flatV[cflatV++] = nx;
                    flatV[cflatV++] = ny;
                    flatC[cflatC++] = ((float)colr)/255.0f;
                    flatC[cflatC++] = ((float)colg)/255.0f;
                    flatC[cflatC++] = ((float)colb)/255.0f;
                    flatC[cflatC++] = 1.0f;
                    cflat++;
#else
					vid[ny*(XRES+BARSIZE)+nx] = PIXRGB(colr,colg,colb);
#endif
				}
				if(pixel_mode & PMODE_BLEND)
				{
#ifdef OGLR
                    flatV[cflatV++] = nx;
                    flatV[cflatV++] = ny;
                    flatC[cflatC++] = ((float)colr)/255.0f;
                    flatC[cflatC++] = ((float)colg)/255.0f;
                    flatC[cflatC++] = ((float)colb)/255.0f;
                    flatC[cflatC++] = ((float)cola)/255.0f;
                    cflat++;
#else
					blendpixel(vid, nx, ny, colr, colg, colb, cola);
#endif
				}
				if(pixel_mode & PMODE_ADD)
				{
#ifdef OGLR
                    addV[caddV++] = nx;
                    addV[caddV++] = ny;
                    addC[caddC++] = ((float)colr)/255.0f;
                    addC[caddC++] = ((float)colg)/255.0f;
                    addC[caddC++] = ((float)colb)/255.0f;
                    addC[caddC++] = ((float)cola)/255.0f;
                    cadd++;
#else
					addpixel(vid, nx, ny, colr, colg, colb, cola);
#endif
				}
				if(pixel_mode & PMODE_BLOB)
				{
#ifdef OGLR
                    blobV[cblobV++] = nx;
                    blobV[cblobV++] = ny;
                    blobC[cblobC++] = ((float)colr)/255.0f;
                    blobC[cblobC++] = ((float)colg)/255.0f;
                    blobC[cblobC++] = ((float)colb)/255.0f;
                    blobC[cblobC++] = 1.0f;
                    cblob++;
#else
					vid[ny*(XRES+BARSIZE)+nx] = PIXRGB(colr,colg,colb);

					blendpixel(vid, nx+1, ny, colr, colg, colb, 223);
					blendpixel(vid, nx-1, ny, colr, colg, colb, 223);
					blendpixel(vid, nx, ny+1, colr, colg, colb, 223);
					blendpixel(vid, nx, ny-1, colr, colg, colb, 223);

					blendpixel(vid, nx+1, ny-1, colr, colg, colb, 112);
					blendpixel(vid, nx-1, ny-1, colr, colg, colb, 112);
					blendpixel(vid, nx+1, ny+1, colr, colg, colb, 112);
					blendpixel(vid, nx-1, ny+1, colr, colg, colb, 112);
#endif
				}
				if(pixel_mode & PMODE_GLOW)
				{
					int cola1 = (5*cola)/255;
#ifdef OGLR
                    glowV[cglowV++] = nx;
                    glowV[cglowV++] = ny;
                    glowC[cglowC++] = ((float)colr)/255.0f;
                    glowC[cglowC++] = ((float)colg)/255.0f;
                    glowC[cglowC++] = ((float)colb)/255.0f;
                    glowC[cglowC++] = 1.0f;
                    cglow++;
#else
					addpixel(vid, nx, ny, colr, colg, colb, (192*cola)/255);
					addpixel(vid, nx+1, ny, colr, colg, colb, (96*cola)/255);
					addpixel(vid, nx-1, ny, colr, colg, colb, (96*cola)/255);
					addpixel(vid, nx, ny+1, colr, colg, colb, (96*cola)/255);
					addpixel(vid, nx, ny-1, colr, colg, colb, (96*cola)/255);
					
					for (x = 1; x < 6; x++) {
						addpixel(vid, nx, ny-x, colr, colg, colb, cola1);
						addpixel(vid, nx, ny+x, colr, colg, colb, cola1);
						addpixel(vid, nx-x, ny, colr, colg, colb, cola1);
						addpixel(vid, nx+x, ny, colr, colg, colb, cola1);
						for (y = 1; y < 6; y++) {
							if(x + y > 7)
								continue;
							addpixel(vid, nx+x, ny-y, colr, colg, colb, cola1);
							addpixel(vid, nx-x, ny+y, colr, colg, colb, cola1);
							addpixel(vid, nx+x, ny+y, colr, colg, colb, cola1);
							addpixel(vid, nx-x, ny-y, colr, colg, colb, cola1);
						}
					}
#endif
				}
				if(pixel_mode & PMODE_BLUR)
				{
#ifdef OGLR
                    blurV[cblurV++] = nx;
                    blurV[cblurV++] = ny;
                    blurC[cblurC++] = ((float)colr)/255.0f;
                    blurC[cblurC++] = ((float)colg)/255.0f;
                    blurC[cblurC++] = ((float)colb)/255.0f;
                    blurC[cblurC++] = 1.0f;
                    cblur++;
#else
					for (x=-3; x<4; x++)
					{
						for (y=-3; y<4; y++)
						{
							if (abs(x)+abs(y) <2 && !(abs(x)==2||abs(y)==2))
								blendpixel(vid, x+nx, y+ny, colr, colg, colb, 30);
							if (abs(x)+abs(y) <=3 && abs(x)+abs(y))
								blendpixel(vid, x+nx, y+ny, colr, colg, colb, 20);
							if (abs(x)+abs(y) == 2)
								blendpixel(vid, x+nx, y+ny, colr, colg, colb, 10);
						}
					}
#endif
				}
				if(pixel_mode & PMODE_SPARK)
				{
					flicker = (float)(rand()%20);
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-5;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+5;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-5;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+5;
				    cline++;
#else
					gradv = 4*parts[i].life + flicker;
					for (x = 0; gradv>0.5; x++) {
						addpixel(vid, nx+x, ny, colr, colg, colb, (int)gradv);
						addpixel(vid, nx-x, ny, colr, colg, colb, (int)gradv);

						addpixel(vid, nx, ny+x, colr, colg, colb, (int)gradv);
						addpixel(vid, nx, ny-x, colr, colg, colb, (int)gradv);
						gradv = gradv/1.5f;
					}
#endif
				}
				if(pixel_mode & PMODE_FLARE)
				{
					flicker = (float)(rand()%20);
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-10;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/40;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+10;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-10;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+10;
				    cline++;
#else
					gradv = flicker + fabs(parts[i].vx)*17 + fabs(parts[i].vy)*17;
					blendpixel(vid, nx, ny, colr, colg, colb, (int)((gradv*4)>255?255:(gradv*4)));
					blendpixel(vid, nx+1, ny, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx-1, ny, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx, ny+1, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx, ny-1, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					if (gradv>255) gradv=255;
					blendpixel(vid, nx+1, ny-1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx-1, ny-1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx+1, ny+1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx-1, ny+1, colr, colg, colb, (int)gradv);
					for (x = 1; gradv>0.5; x++) {
						addpixel(vid, nx+x, ny, colr, colg, colb, (int)gradv);
						addpixel(vid, nx-x, ny, colr, colg, colb, (int)gradv);
						addpixel(vid, nx, ny+x, colr, colg, colb, (int)gradv);
						addpixel(vid, nx, ny-x, colr, colg, colb, (int)gradv);
						gradv = gradv/1.2f;
					}
#endif
				}
				if(pixel_mode & PMODE_LFLARE)
				{
					flicker = (float)(rand()%20);
#ifdef OGLR
					//Oh god, this is awful
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx-70;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/30;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx+70;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny-70;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 1.0f - ((float)flicker)/50;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny;
				    cline++;
				    
				    lineC[clineC++] = ((float)colr)/255.0f;
				    lineC[clineC++] = ((float)colg)/255.0f;
				    lineC[clineC++] = ((float)colb)/255.0f;
				    lineC[clineC++] = 0.0f;
				    lineV[clineV++] = fnx;
				    lineV[clineV++] = fny+70;
				    cline++;
#else
					gradv = flicker + fabs(parts[i].vx)*17 + fabs(parts[i].vy)*17;
					blendpixel(vid, nx, ny, colr, colg, colb, (int)((gradv*4)>255?255:(gradv*4)));
					blendpixel(vid, nx+1, ny, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx-1, ny, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx, ny+1, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					blendpixel(vid, nx, ny-1, colr, colg, colb, (int)((gradv*2)>255?255:(gradv*2)));
					if (gradv>255) gradv=255;
					blendpixel(vid, nx+1, ny-1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx-1, ny-1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx+1, ny+1, colr, colg, colb, (int)gradv);
					blendpixel(vid, nx-1, ny+1, colr, colg, colb, (int)gradv);
					for (x = 1; gradv>0.5; x++) {
						addpixel(vid, nx+x, ny, colr, colg, colb, (int)gradv);
						addpixel(vid, nx-x, ny, colr, colg, colb, (int)gradv);
						addpixel(vid, nx, ny+x, colr, colg, colb, (int)gradv);
						addpixel(vid, nx, ny-x, colr, colg, colb, (int)gradv);
						gradv = gradv/1.01f;
					}
#endif
				}
				if (pixel_mode & EFFECT_GRAVIN)
				{
					int nxo = 0;
					int nyo = 0;
					int r;
					float drad = 0.0f;
					float ddist = 0.0f;
					orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
					for (r = 0; r < 4; r++) {
						ddist = ((float)orbd[r])/16.0f;
						drad = (M_PI * ((float)orbl[r]) / 180.0f)*1.41f;
						nxo = (int)(ddist*cos(drad));
						nyo = (int)(ddist*sin(drad));
						if (ny+nyo>0 && ny+nyo<YRES && nx+nxo>0 && nx+nxo<XRES && (pmap[ny+nyo][nx+nxo]&0xFF) != PT_PRTI && (pmap[ny+nyo][nx+nxo]&0xFF) != PT_PPTI)
							addpixel(vid, nx+nxo, ny+nyo, colr, colg, colb, 255-orbd[r]);
					}
				}
				if (pixel_mode & EFFECT_GRAVOUT)
				{
					int nxo = 0;
					int nyo = 0;
					int r;
					float drad = 0.0f;
					float ddist = 0.0f;
					orbitalparts_get(parts[i].life, parts[i].ctype, orbd, orbl);
					for (r = 0; r < 4; r++) {
						ddist = ((float)orbd[r])/16.0f;
						drad = (M_PI * ((float)orbl[r]) / 180.0f)*1.41f;
						nxo = (int)(ddist*cos(drad));
						nyo = (int)(ddist*sin(drad));
						if (ny+nyo>0 && ny+nyo<YRES && nx+nxo>0 && nx+nxo<XRES && (pmap[ny+nyo][nx+nxo]&0xFF) != PT_PRTO && (pmap[ny+nyo][nx+nxo]&0xFF) != PT_PPTO)
							addpixel(vid, nx+nxo, ny+nyo, colr, colg, colb, 255-orbd[r]);
					}
				}
				if ((pixel_mode & EFFECT_DBGLINES) && DEBUG_MODE && !(display_mode&DISPLAY_PERS))
				{
					// draw lines connecting wifi/portal channels
					if (mousePos.X == nx && mousePos.Y == ny && (i == pmap[ny][nx]>>8))
					{
						int type = parts[i].type, tmp = (int)((parts[i].temp-73.15f)/100+1), othertmp;
						int type2 = parts[i].type;
						if (type == PT_PRTI || type == PT_PPTI)
							type = PT_PRTO;
						else if (type == PT_PRTO || type == PT_PPTO)
							type = PT_PRTI;
						if (type == PT_PRTI)
							type2 = PT_PPTI;
						else if (type == PT_PRTO)
							type2 = PT_PPTO;
						for (int z = 0; z < sim->parts_lastActiveIndex; z++)
						{
							if (parts[z].type==type || parts[z].type==type2)
							{
								othertmp = (int)((parts[z].temp-73.15f)/100+1); 
								if (tmp == othertmp)
									xor_line(nx,ny,(int)(parts[z].x+0.5f),(int)(parts[z].y+0.5f),vid);
							}
						}
					}
				}
				//Fire effects
				if(firea && (pixel_mode & FIRE_BLEND))
				{
#ifdef OGLR
                    smokeV[csmokeV++] = nx;
                    smokeV[csmokeV++] = ny;
                    smokeC[csmokeC++] = ((float)firer)/255.0f;
                    smokeC[csmokeC++] = ((float)fireg)/255.0f;
                    smokeC[csmokeC++] = ((float)fireb)/255.0f;
                    smokeC[csmokeC++] = ((float)firea)/255.0f;
                    csmoke++;
#else
					firea /= 2;
					fire_r[ny/CELL][nx/CELL] = (firea*firer + (255-firea)*fire_r[ny/CELL][nx/CELL]) >> 8;
					fire_g[ny/CELL][nx/CELL] = (firea*fireg + (255-firea)*fire_g[ny/CELL][nx/CELL]) >> 8;
					fire_b[ny/CELL][nx/CELL] = (firea*fireb + (255-firea)*fire_b[ny/CELL][nx/CELL]) >> 8;
#endif
				}
				if(firea && (pixel_mode & FIRE_ADD))
				{
#ifdef OGLR
                    fireV[cfireV++] = nx;
                    fireV[cfireV++] = ny;
                    fireC[cfireC++] = ((float)firer)/255.0f;
                    fireC[cfireC++] = ((float)fireg)/255.0f;
                    fireC[cfireC++] = ((float)fireb)/255.0f;
                    fireC[cfireC++] = ((float)firea)/255.0f;
                    cfire++;
#else
					firea /= 8;
					firer = ((firea*firer) >> 8) + fire_r[ny/CELL][nx/CELL];
					fireg = ((firea*fireg) >> 8) + fire_g[ny/CELL][nx/CELL];
					fireb = ((firea*fireb) >> 8) + fire_b[ny/CELL][nx/CELL];
				
					if(firer>255)
						firer = 255;
					if(fireg>255)
						fireg = 255;
					if(fireb>255)
						fireb = 255;
					
					fire_r[ny/CELL][nx/CELL] = firer;
					fire_g[ny/CELL][nx/CELL] = fireg;
					fire_b[ny/CELL][nx/CELL] = fireb;
#endif
				}
				if(firea && (pixel_mode & FIRE_SPARK))
				{
#ifdef OGLR
					smokeV[csmokeV++] = nx;
					smokeV[csmokeV++] = ny;
					smokeC[csmokeC++] = ((float)firer)/255.0f;
					smokeC[csmokeC++] = ((float)fireg)/255.0f;
					smokeC[csmokeC++] = ((float)fireb)/255.0f;
					smokeC[csmokeC++] = ((float)firea)/255.0f;
					csmoke++;
#else
					firea /= 4;
					fire_r[ny/CELL][nx/CELL] = (firea*firer + (255-firea)*fire_r[ny/CELL][nx/CELL]) >> 8;
					fire_g[ny/CELL][nx/CELL] = (firea*fireg + (255-firea)*fire_g[ny/CELL][nx/CELL]) >> 8;
					fire_b[ny/CELL][nx/CELL] = (firea*fireb + (255-firea)*fire_b[ny/CELL][nx/CELL]) >> 8;
#endif
				}
			}
		}
	}
#ifdef OGLR		
        
        //Go into array mode
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      

		if(cablurLine)
		{
			// -- BEGIN LINES -- //
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable( GL_LINE_SMOOTH );
			glColorPointer(4, GL_FLOAT, 0, &ablurLineC[0]);
			glVertexPointer(2, GL_FLOAT, 0, &ablurLineV[0]);
			
			glDrawArrays(GL_LINES, 0, cablurLine);
			
			//Clear some stuff we set
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_LINE_SMOOTH);
			// -- END LINES -- //
		}
		if(cblurLine)
		{
			// -- BEGIN LINES -- //
			glEnable( GL_LINE_SMOOTH );
			glColorPointer(4, GL_FLOAT, 0, &blurLineC[0]);
			glVertexPointer(2, GL_FLOAT, 0, &blurLineV[0]);
			
			glDrawArrays(GL_LINES, 0, cblurLine);
			
			//Clear some stuff we set
			glDisable(GL_LINE_SMOOTH);
			// -- END LINES -- //
		}
	
 		if(cflat)
		{
			// -- BEGIN FLAT -- //
			//Set point size (size of fire texture)
			glPointSize(1.0f);

		    glColorPointer(4, GL_FLOAT, 0, &flatC[0]);
		    glVertexPointer(2, GL_INT, 0, &flatV[0]);

		    glDrawArrays(GL_POINTS, 0, cflat);
		    
		    //Clear some stuff we set
		    // -- END FLAT -- //
        }
        
        if(cblob)
		{
		    // -- BEGIN BLOB -- //
			glEnable( GL_POINT_SMOOTH ); //Blobs!
			glPointSize(2.5f);

		    glColorPointer(4, GL_FLOAT, 0, &blobC[0]);
		    glVertexPointer(2, GL_INT, 0, &blobV[0]);

		    glDrawArrays(GL_POINTS, 0, cblob);
		    
		    //Clear some stuff we set
		    glDisable( GL_POINT_SMOOTH );
		    // -- END BLOB -- //
		}
        
        if(cglow || cblur)
        {
        	// -- BEGIN GLOW -- //
			//Start and prepare fire program
			glEnable(GL_TEXTURE_2D);
			glUseProgram(fireProg);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, glowAlpha);
			glUniform1i(glGetUniformLocation(fireProg, "fireAlpha"), 0);
			
			glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
		
			//Make sure we can use texture coords on points
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		
			//Set point size (size of fire texture)
			glPointSize(11.0f);
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if(cglow)
			{
				glColorPointer(4, GL_FLOAT, 0, &glowC[0]);
				glVertexPointer(2, GL_INT, 0, &glowV[0]);
	
				glDrawArrays(GL_POINTS, 0, cglow);
			}
			
			glPointSize(7.0f);
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			if(cblur)
			{
				glBindTexture(GL_TEXTURE_2D, blurAlpha);
			
				glColorPointer(4, GL_FLOAT, 0, &blurC[0]);
				glVertexPointer(2, GL_INT, 0, &blurV[0]);

				glDrawArrays(GL_POINTS, 0, cblur);
			}
		    
		    //Clear some stuff we set
		    glDisable(GL_POINT_SPRITE);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		    // -- END GLOW -- //
        }
        
 		if(cadd)
		{
			// -- BEGIN ADD -- //
			//Set point size (size of fire texture)
			glPointSize(1.0f);

		    glColorPointer(4, GL_FLOAT, 0, &addC[0]);
		    glVertexPointer(2, GL_INT, 0, &addV[0]);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		    glDrawArrays(GL_POINTS, 0, cadd);
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		    //Clear some stuff we set
		    // -- END ADD -- //
        }
        
        if(cline)
		{
			// -- BEGIN LINES -- //
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable( GL_LINE_SMOOTH );
		    glColorPointer(4, GL_FLOAT, 0, &lineC[0]);
		    glVertexPointer(2, GL_FLOAT, 0, &lineV[0]);

		    glDrawArrays(GL_LINE_STRIP, 0, cline);
		    
		    //Clear some stuff we set
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		    glDisable(GL_LINE_SMOOTH);
		    // -- END LINES -- //
        }
		
		if(cfire || csmoke)
		{
			// -- BEGIN FIRE -- //
			//Start and prepare fire program
			glEnable(GL_TEXTURE_2D);
			glUseProgram(fireProg);
			//glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fireAlpha);
			glUniform1i(glGetUniformLocation(fireProg, "fireAlpha"), 0);
		
			//Make sure we can use texture coords on points
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
		
			//Set point size (size of fire texture)
			glPointSize(CELL*3);
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			if(cfire)
			{
				glColorPointer(4, GL_FLOAT, 0, &fireC[0]);
				glVertexPointer(2, GL_INT, 0, &fireV[0]);

				glDrawArrays(GL_POINTS, 0, cfire);
			}
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			if(csmoke)
			{
				glColorPointer(4, GL_FLOAT, 0, &smokeC[0]);
				glVertexPointer(2, GL_INT, 0, &smokeV[0]);

				glDrawArrays(GL_POINTS, 0, csmoke);
			}
		    
		    //Clear some stuff we set
		    glDisable(GL_POINT_SPRITE);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		    // -- END FIRE -- //
        }

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);     
        
        //Reset FBO
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        
        //Drawing the FBO onto the screen sounds like a cool idea now
        
        glBlendFunc(origBlendSrc, origBlendDst);
#endif
}

#ifdef OGLR
void draw_parts_fbo()
{
	glEnable( GL_TEXTURE_2D );
	if(display_mode & DISPLAY_WARP)
	{
		float xres = XRES, yres = YRES;
		glUseProgram(lensProg);
		glActiveTexture(GL_TEXTURE0);			
		glBindTexture(GL_TEXTURE_2D, partsFboTex);
		glUniform1i(glGetUniformLocation(lensProg, "pTex"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, partsTFX);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_RED, GL_FLOAT, gravx);
		glUniform1i(glGetUniformLocation(lensProg, "tfX"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, partsTFY);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, XRES/CELL, YRES/CELL, GL_GREEN, GL_FLOAT, gravy);
		glUniform1i(glGetUniformLocation(lensProg, "tfY"), 2);
		glActiveTexture(GL_TEXTURE0);
		glUniform1fv(glGetUniformLocation(lensProg, "xres"), 1, &xres);
		glUniform1fv(glGetUniformLocation(lensProg, "yres"), 1, &yres);
	}
	else
	{	
		glBindTexture(GL_TEXTURE_2D, partsFboTex);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2d(1, 0);
	glVertex3f(XRES*sdl_scale, (YRES+MENUSIZE)*sdl_scale, 1.0);
	glTexCoord2d(0, 0);
	glVertex3f(0, (YRES+MENUSIZE)*sdl_scale, 1.0);
	glTexCoord2d(0, 1);
	glVertex3f(0, MENUSIZE*sdl_scale, 1.0);
	glTexCoord2d(1, 1);
	glVertex3f(XRES*sdl_scale, MENUSIZE*sdl_scale, 1.0);
	glEnd();
	
	if(display_mode & DISPLAY_WARP)
	{
		glUseProgram(0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glDisable( GL_TEXTURE_2D );
}
#endif

// draw the graphics that appear before update_particles is called
void render_before(pixel *part_vbuf)
{
#ifdef OGLR
		if (display_mode & DISPLAY_PERS)//save background for persistent, then clear
		{
			clearScreen(0.01f);
			memset(part_vbuf, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
		}
		else //clear screen every frame
		{
			clearScreen(1.0f);
			memset(part_vbuf, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
			if (display_mode & DISPLAY_AIR)//air only gets drawn in these modes
			{
				draw_air(part_vbuf);
			}
		}
#else
		if (display_mode & DISPLAY_AIR)//air only gets drawn in these modes
		{
			draw_air(part_vbuf);
		}
		else if (display_mode & DISPLAY_PERS)//save background for persistent, then clear
		{
			memcpy(part_vbuf, pers_bg, (XRES+BARSIZE)*YRES*PIXELSIZE);
			memset(part_vbuf+((XRES+BARSIZE)*YRES), 0, ((XRES+BARSIZE)*YRES*PIXELSIZE)-((XRES+BARSIZE)*YRES*PIXELSIZE));
		}
		else //clear screen every frame
		{
			memset(part_vbuf, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
		}
#endif
		if(ngrav_enable && drawgrav_enable)
			draw_grav(part_vbuf);
		draw_walls(part_vbuf);
}

int persist_counter = 0;
// draw the graphics that appear after update_particles is called
void render_after(pixel *part_vbuf, pixel *vid_buf, Point mousePos)
{
	render_parts(part_vbuf, mousePos); //draw particles
	draw_other(part_vbuf);
#ifndef RENDERER
	if (((WallTool*)activeTools[activeToolID])->GetID() == WL_GRAV)
		draw_grav_zones(part_vbuf);
#endif
	if (vid_buf && (display_mode & DISPLAY_PERS))
	{
		if (!persist_counter)
		{
			dim_copy_pers(pers_bg, vid_buf);
		}
		else
		{
			memcpy(pers_bg, vid_buf, (XRES+BARSIZE)*YRES*PIXELSIZE);
		}
		persist_counter = (persist_counter+1) % 3;
	}
#ifndef OGLR
	if (render_mode & FIREMODE)
		render_fire(part_vbuf);
#endif

	render_signs(part_vbuf);

#ifndef OGLR
	if(vid_buf && ngrav_enable && (display_mode & DISPLAY_WARP))
		render_gravlensing(part_vbuf, vid_buf);
#endif
	if (finding & 0x8)
		draw_find();
	sampleColor = vid_buf[(mousePos.Y)*(XRES + BARSIZE) + (mousePos.X)];
}

void draw_find() //Find just like how my lua script did it, it will find everything and show it's exact spot, and not miss things under stacked particles
{
	int i, x, y;
	if (finding == 8)
		return;
	fillrect(vid_buf, -1, -1, XRES+1, YRES+1, 0, 0, 0, 230); //Dim everything
	for (i = 0; i <= globalSim->parts_lastActiveIndex; i++) //Color particles
	{
		if ((finding & 0x1) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[0])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[0])->GetID() == parts[i].ctype)))
			drawpixel(vid_buf, (int)(parts[i].x+.5f), (int)(parts[i].y+.5f), 255, 0, 0, 255);
		else if ((finding & 0x2) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[1])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[1])->GetID() == parts[i].ctype)))
			drawpixel(vid_buf, (int)(parts[i].x+.5f), (int)(parts[i].y+.5f), 0, 0, 255, 255);
		else if ((finding & 0x4) && ((parts[i].type != PT_LIFE && ((ElementTool*)activeTools[2])->GetID() == parts[i].type) || (parts[i].type == PT_LIFE && ((GolTool*)activeTools[2])->GetID() == parts[i].ctype)))
			drawpixel(vid_buf, (int)(parts[i].x+.5f), (int)(parts[i].y+.5f), 0, 255, 0, 255);
	}
	for (y=0; y<YRES/CELL; y++) //Color walls
	{
		for (x=0; x<XRES/CELL; x++)
		{
			if ((finding & 0x1) && bmap[y][x] == ((WallTool*)activeTools[0])->GetID())
				fillrect(vid_buf, x*CELL-1, y*CELL-1, CELL+1, CELL+1, 255, 0, 0, 255);
			else if ((finding & 0x2) && bmap[y][x] == ((WallTool*)activeTools[1])->GetID())
				fillrect(vid_buf, x*CELL-1, y*CELL-1, CELL+1, CELL+1, 0, 0, 255, 255);
			else if ((finding & 0x4) && bmap[y][x] == ((WallTool*)activeTools[2])->GetID())
				fillrect(vid_buf, x*CELL-1, y*CELL-1, CELL+1, CELL+1, 0, 255, 0, 255);
		}
	}
}

void draw_walls(pixel *vid)
{
	for (int y = 0; y < YRES/CELL; y++)
		for (int x =0; x < XRES/CELL; x++)
			if (bmap[y][x])
			{
				unsigned char wt = bmap[y][x];
				if (wt >= WALLCOUNT)
					continue;
				unsigned char powered = emap[y][x];
				pixel pc = PIXPACK(wallTypes[wt].colour);
				pixel gc = PIXPACK(wallTypes[wt].eglow);

				if (finding)
				{
					if ((finding & 0x1) && wt == ((WallTool*)activeTools[0])->GetID())
					{
						pc = PIXRGB(255,0,0);
						gc = PIXRGB(255,0,0);
					}
					else if ((finding & 0x2) && wt == ((WallTool*)activeTools[1])->GetID())
					{
						pc = PIXRGB(0,0,255);
						gc = PIXRGB(0,0,255);
					}
					else if ((finding & 0x4) && wt == ((WallTool*)activeTools[2])->GetID())
					{
						pc = PIXRGB(0,255,0);
						gc = PIXRGB(0,255,0);
					}
					else if (!(finding &0x8))
					{
						pc = PIXRGB(PIXR(pc)/10,PIXG(pc)/10,PIXB(pc)/10);
						gc = PIXRGB(PIXR(gc)/10,PIXG(gc)/10,PIXB(gc)/10);
					}
				}

				switch (wallTypes[wt].drawstyle)
				{
				case 0:
					if (wt == WL_EWALL)
					{
						if (powered)
						{
							for (int j = 0; j < CELL; j++)
								for (int i =0; i < CELL; i++)
									if (i&j&1)
										vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
						}
						else
						{
							for (int j = 0; j < CELL; j++)
								for (int i = 0; i < CELL; i++)
									if (!(i&j&1))
										vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
						}
					}
					else if (wt == WL_WALLELEC)
					{
						for (int j = 0; j < CELL; j++)
							for (int i = 0; i < CELL; i++)
							{
								if (!((y*CELL+j)%2) && !((x*CELL+i)%2))
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
								else
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x808080);
							}
					}
					else if (wt == WL_EHOLE)
					{
						if (powered)
						{
							for (int j = 0; j < CELL; j++)
								for (int i = 0; i < CELL; i++)
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x242424);
							for (int j = 0; j < CELL; j += 2)
								for (int i = 0; i < CELL; i += 2)
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x000000);
						}
						else
						{
							for (int j = 0; j < CELL; j += 2)
								for (int i =0; i < CELL; i += 2)
									vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x242424);
						}
					}
					else if (wt == WL_STREAM)
					{
						float xf = x*CELL + CELL*0.5f;
						float yf = y*CELL + CELL*0.5f;
						int oldX = (int)(xf+0.5f), oldY = (int)(yf+0.5f);
						int newX, newY;
						float xVel = vx[y][x]*0.125f, yVel = vy[y][x]*0.125f;
						// there is no velocity here, draw a streamline and continue
						if (!xVel && !yVel)
						{
							drawtext(vid, x*CELL, y*CELL-2, "\x8D", 255, 255, 255, 128);
							drawpixel(vid, oldX, oldY, 255, 255, 255, 255);
							continue;
						}
						bool changed = false;
						for (int t = 0; t < 1024; t++)
						{
							newX = (int)(xf+0.5f);
							newY = (int)(yf+0.5f);
							if (newX != oldX || newY != oldY)
							{
								changed = true;
								oldX = newX;
								oldY = newY;
							}
							if (changed && (newX<0 || newX>=XRES || newY<0 || newY>=YRES))
								break;
							addpixel(vid, newX, newY, 255, 255, 255, 64);
							// cache velocity and other checks so we aren't running them constantly
							if (changed)
							{
								int wallX = newX/CELL;
								int wallY = newY/CELL;
								xVel = vx[wallY][wallX]*0.125f;
								yVel = vy[wallY][wallX]*0.125f;
								if (wallX != x && wallY != y && bmap[wallY][wallX] == WL_STREAM)
									break;
							}
							xf += xVel;
							yf += yVel;
						}
						drawtext(vid, x*CELL, y*CELL-2, "\x8D", 255, 255, 255, 128);
					}
					break;
				case 1:
					for (int j = 0; j < CELL; j += 2)
						for (int i = (j>>1)&1; i < CELL; i += 2)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
					break;
				case 2:
					for (int j = 0; j < CELL; j += 2)
						for (int i = 0; i < CELL; i += 2)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
					break;
				case 3:
					for (int j = 0; j < CELL; j++)
						for (int i = 0; i < CELL; i++)
							vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
					break;
				case 4:
					for (int j = 0; j < CELL; j++)
						for (int i = 0; i < CELL; i++)
							if (i == j)
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = pc;
							else if (i == j+1 || (i == 0 && j == CELL-1))
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = gc;
							else
								vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x202020);
					break;
				}

				// when in blob view, draw some blobs...
				if (render_mode & PMODE_BLOB)
				{
					switch (wallTypes[wt].drawstyle)
					{
					case 0:
						if (wt == WL_EWALL)
						{
							if (powered)
							{
								for (int j = 0; j < CELL; j++)
									for (int i =0; i < CELL; i++)
										if (i&j&1)
											drawblob(vid, (x*CELL+i), (y*CELL+j), 0x80, 0x80, 0x80);
							}
							else
							{
								for (int j = 0; j < CELL; j++)
									for (int i = 0; i < CELL; i++)
										if (!(i&j&1))
											drawblob(vid, (x*CELL+i), (y*CELL+j), 0x80, 0x80, 0x80);
							}
						}
						else if (wt == WL_WALLELEC)
						{
							for (int j = 0; j < CELL; j++)
								for (int i =0; i < CELL; i++)
								{
									if (!((y*CELL+j)%2) && !((x*CELL+i)%2))
										drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(pc), PIXG(pc), PIXB(pc));
									else
										drawblob(vid, (x*CELL+i), (y*CELL+j), 0x80, 0x80, 0x80);
								}
						}
						else if (wt == WL_EHOLE)
						{
							if (powered)
							{
								for (int j = 0; j < CELL; j++)
									for (int i = 0; i < CELL; i++)
										drawblob(vid, (x*CELL+i), (y*CELL+j), 0x24, 0x24, 0x24);
								for (int j = 0; j < CELL; j += 2)
									for (int i = 0; i < CELL; i += 2)
										// looks bad if drawing black blobs
										vid[(y*CELL+j)*(XRES+BARSIZE)+(x*CELL+i)] = PIXPACK(0x000000);
							}
							else
							{
								for (int j = 0; j < CELL; j += 2)
									for (int i = 0; i < CELL; i += 2)
										drawblob(vid, (x*CELL+i), (y*CELL+j), 0x24, 0x24, 0x24);
							}
						}
						break;
					 case 1:
						for (int j = 0; j < CELL; j += 2)
							for (int i = (j>>1)&1; i < CELL; i += 2)
								drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(pc), PIXG(pc), PIXB(pc));
						break;
					case 2:
						for (int j = 0; j < CELL; j += 2)
							for (int i = 0; i < CELL; i+=2)
								drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(pc), PIXG(pc), PIXB(pc));
						break;
					case 3:
						for (int j = 0; j < CELL; j++)
							for (int i = 0; i < CELL; i++)
								drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(pc), PIXG(pc), PIXB(pc));
						break;
					case 4:
						for (int j = 0; j < CELL; j++)
							for (int i = 0; i < CELL; i++)
								if (i == j)
									drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(pc), PIXG(pc), PIXB(pc));
								else if (i == j+1 || (i == 0 && j == CELL-1))
									drawblob(vid, (x*CELL+i), (y*CELL+j), PIXR(gc), PIXG(gc), PIXB(gc));
								else
									drawblob(vid, (x*CELL+i), (y*CELL+j), 0x20, 0x20, 0x20);
						break;
					}
				}

				if (wallTypes[wt].eglow && powered)
				{
					// glow if electrified
					ARGBColour glow = wallTypes[wt].eglow;
					int alpha = 255;
					int cr = (alpha*COLR(glow) + (255-alpha)*fire_r[y/CELL][x/CELL]) >> 8;
					int cg = (alpha*COLG(glow) + (255-alpha)*fire_g[y/CELL][x/CELL]) >> 8;
					int cb = (alpha*COLB(glow) + (255-alpha)*fire_b[y/CELL][x/CELL]) >> 8;

					if (cr > 255)
						cr = 255;
					if (cg > 255)
						cg = 255;
					if (cb > 255)
						cb = 255;
					fire_r[y][x] = cr;
					fire_g[y][x] = cg;
					fire_b[y][x] = cb;
				}
			}
}

void render_signs(pixel *vid_buf)
{
	int i, j, x, y, w, h, dx, dy,mx,my,b=1,bq;
	for (i=0; i<MAXSIGNS; i++)
		if (signs[i].text[0])
		{
			char buff[256];  //Buffer
			get_sign_pos(i, &x, &y, &w, &h);
			clearrect(vid_buf, x+1, y+1, w-1, h-1);
			drawrect(vid_buf, x, y, w, h, 192, 192, 192, 255);

			//Displaying special information
			if (strcmp(signs[i].text, "{p}")==0)
			{
				float pressure = 0.0f;
				if (signs[i].x>=0 && signs[i].x<XRES && signs[i].y>=0 && signs[i].y<YRES)
					pressure = pv[signs[i].y/CELL][signs[i].x/CELL];
				sprintf(buff, "Pressure: %3.2f", pressure);  //...pressure
				drawtext(vid_buf, x+3, y+3, buff, 255, 255, 255, 255);
			}
			if (strcmp(signs[i].text, "{t}")==0)
			{
				if (signs[i].x>=0 && signs[i].x<XRES && signs[i].y>=0 && signs[i].y<YRES && pmap[signs[i].y][signs[i].x])
					sprintf(buff, "Temp: %4.2f", parts[pmap[signs[i].y][signs[i].x]>>8].temp-273.15);  //...temperature
				else
					sprintf(buff, "Temp: 0.00");  //...temperature
				drawtext(vid_buf, x+3, y+3, buff, 255, 255, 255, 255);
			}

			if (!sregexp(signs[i].text, "^{[ct]:[0-9]*|.*}$") || !sregexp(signs[i].text, "^{s:.*|.*}$") || !sregexp(signs[i].text, "^{b|.*}$"))
			{
				int sldr, startm;
				memset(buff, 0, sizeof(buff));
				for (sldr=2; signs[i].text[sldr-1] != '|'; sldr++)
					startm = sldr + 1;
				sldr = startm;
				while (signs[i].text[sldr] != '}')
				{
					buff[sldr - startm] = signs[i].text[sldr];
					sldr++;
				}
				if (signs[i].text[1] == 'b')
					drawtext(vid_buf, x+3, y+3, buff, 211, 211, 40, 255);
				else
					drawtext(vid_buf, x+3, y+3, buff, 0, 191, 255, 255);
			}

			//Usual text
			if (strcmp(signs[i].text, "{p}") && strcmp(signs[i].text, "{t}") && sregexp(signs[i].text, "^{[ct]:[0-9]*|.*}$") && sregexp(signs[i].text, "^{s:.*|.*}$") && sregexp(signs[i].text, "^{b|.*}$"))
				drawtext(vid_buf, x+3, y+3, signs[i].text, 255, 255, 255, 255);

			x = signs[i].x;
			y = signs[i].y;
			dx = 1 - signs[i].ju;
			dy = (signs[i].y > 18) ? -1 : 1;
			for (j=0; j<4; j++)
			{
				drawpixel(vid_buf, x, y, 192, 192, 192, 255);
				x+=dx;
				y+=dy;
			}
			if (MSIGN==i)
			{
				bq = b;
				mouse_get_state(&mx, &my);
				mouse_coords_window_to_sim(&mx, &my);
				signs[i].x = mx;
				signs[i].y = my;
			}
		}
}

void render_gravlensing(pixel *src, pixel * dst)
{
	int nx, ny, rx, ry, gx, gy, bx, by, co;
	int r, g, b;
	pixel t;
	for(nx = 0; nx < XRES; nx++)
	{
		for(ny = 0; ny < YRES; ny++)
		{
			co = (ny/CELL)*(XRES/CELL)+(nx/CELL);
			rx = (int)(nx-gravx[co]*0.75f+0.5f);
			ry = (int)(ny-gravy[co]*0.75f+0.5f);
			gx = (int)(nx-gravx[co]*0.875f+0.5f);
			gy = (int)(ny-gravy[co]*0.875f+0.5f);
			bx = (int)(nx-gravx[co]+0.5f);
			by = (int)(ny-gravy[co]+0.5f);
			if(rx > 0 && rx < XRES && ry > 0 && ry < YRES && gx > 0 && gx < XRES && gy > 0 && gy < YRES && bx > 0 && bx < XRES && by > 0 && by < YRES)
			{
				t = dst[ny*(XRES+BARSIZE)+nx];
				r = PIXR(src[ry*(XRES+BARSIZE)+rx]) + PIXR(t);
				g = PIXG(src[gy*(XRES+BARSIZE)+gx]) + PIXG(t);
				b = PIXB(src[by*(XRES+BARSIZE)+bx]) + PIXB(t);
				if (r>255)
					r = 255;
				if (g>255)
					g = 255;
				if (b>255)
					b = 255;
				dst[ny*(XRES+BARSIZE)+nx] = PIXRGB(r,g,b);
				//	addpixel(dst, nx, ny, PIXR(src[ry*(XRES+BARSIZE)+rx]), PIXG(src[gy*(XRES+BARSIZE)+gx]), PIXB(src[by*(XRES+BARSIZE)+bx]), 255);
			}

			/*rx = nx+(gravxf[(ny*XRES)+nx]*0.5f);
			ry = ny+(gravyf[(ny*XRES)+nx]*0.5f);
			gx = nx+(gravxf[(ny*XRES)+nx]*0.75f);
			gy = ny+(gravyf[(ny*XRES)+nx]*0.75f);
			bx = nx+(gravxf[(ny*XRES)+nx]);
			by = ny+(gravyf[(ny*XRES)+nx]);
			if(rx > 0 && rx < XRES && ry > 0 && ry < YRES && gravp[ny/CELL][nx/CELL]*0.5f > -8.0f)
				addpixel(dst, rx, ry, PIXR(src[ry*(XRES+BARSIZE)+rx]), 0, 0, 255);
			if(gx > 0 && gx < XRES && gy > 0 && gy < YRES && gravp[ny/CELL][nx/CELL]*0.75f > -8.0f)
				addpixel(dst, gx, gy, 0, PIXG(src[ry*(XRES+BARSIZE)+rx]), 0, 255);
			if(bx > 0 && bx < XRES && by > 0 && by < YRES && gravp[ny/CELL][nx/CELL] > -8.0f)
				addpixel(dst, bx, by, 0, 0, PIXB(src[ry*(XRES+BARSIZE)+rx]), 255);*/
		}
	}
}

void render_fire(pixel *vid)
{
	int i,j,x,y,r,g,b,a;
	for (j=0; j<YRES/CELL; j++)
		for (i=0; i<XRES/CELL; i++)
		{
			r = fire_r[j][i];
			g = fire_g[j][i];
			b = fire_b[j][i];
			if (r || g || b)
				for (y=-CELL; y<2*CELL; y++)
					for (x=-CELL; x<2*CELL; x++)
					{
						a = fire_alpha[y+CELL][x+CELL];
						if (finding && !(finding & 0x8))
							a /= 2;
						addpixel(vid, i*CELL+x, j*CELL+y, r, g, b, a);
					}
			r *= 8;
			g *= 8;
			b *= 8;
			for (y=-1; y<2; y++)
				for (x=-1; x<2; x++)
					if ((x || y) && i+x>=0 && j+y>=0 && i+x<XRES/CELL && j+y<YRES/CELL)
					{
						r += fire_r[j+y][i+x];
						g += fire_g[j+y][i+x];
						b += fire_b[j+y][i+x];
					}
			r /= 16;
			g /= 16;
			b /= 16;
			fire_r[j][i] = r>4 ? r-4 : 0;
			fire_g[j][i] = g>4 ? g-4 : 0;
			fire_b[j][i] = b>4 ? b-4 : 0;
		}
}

void prepare_alpha(int size, float intensity)
{
	//TODO: implement size
	int x,y,i,j;
	float multiplier = 255.0f*intensity;
	float temp[CELL*3][CELL*3];
	memset(temp, 0, sizeof(temp));
	for (x=0; x<CELL; x++)
		for (y=0; y<CELL; y++)
			for (i=-CELL; i<CELL; i++)
				for (j=-CELL; j<CELL; j++)
					temp[y+CELL+j][x+CELL+i] += expf(-0.1f*(i*i+j*j));
	for (x=0; x<CELL*3; x++)
		for (y=0; y<CELL*3; y++)
			fire_alpha[y][x] = (int)(multiplier*temp[y][x]/(CELL*CELL));
			
#ifdef OGLR
	float fire_alphaf[CELL * 3][CELL * 3];
	float glow_alphaf[11][11];
	float blur_alphaf[7][7];
	for (x=0; x<CELL*3; x++)
		for (y=0; y<CELL*3; y++)
		{
			fire_alphaf[y][x] = (intensity*temp[y][x]/((float)(CELL*CELL)))/2.0f;
		}
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, fireAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CELL*3, CELL*3, GL_ALPHA, GL_FLOAT, fire_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	memset(glow_alphaf, 0, sizeof(glow_alphaf));
	
	int c = 5;
	
	glow_alphaf[c][c] = 0.8f;
	glow_alphaf[c][c-1] = 0.4f;
	glow_alphaf[c][c+1] = 0.4f;
	glow_alphaf[c-1][c] = 0.4f;
	glow_alphaf[c+1][c] = 0.4f;
	for (x = 1; x < 6; x++) {
		glow_alphaf[c][c-x] += 0.02f;
		glow_alphaf[c][c+x] += 0.02f;
		glow_alphaf[c-x][c] += 0.02f;
		glow_alphaf[c+x][c] += 0.02f;
		for (y = 1; y < 6; y++) {
			if(x + y > 7)
				continue;
			glow_alphaf[c+x][c-y] += 0.02f;
			glow_alphaf[c-x][c+y] += 0.02f;
			glow_alphaf[c+x][c+y] += 0.02f;
			glow_alphaf[c-x][c-y] += 0.02f;
		}
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, glowAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 11, 11, GL_ALPHA, GL_FLOAT, glow_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	
	c = 3;
	
	for (x=-3; x<4; x++)
	{
		for (y=-3; y<4; y++)
		{
			if (abs(x)+abs(y) <2 && !(abs(x)==2||abs(y)==2))
				blur_alphaf[c+x][c-y] = 0.11f;
			if (abs(x)+abs(y) <=3 && abs(x)+abs(y))
				blur_alphaf[c+x][c-y] = 0.08f;
			if (abs(x)+abs(y) == 2)
				blur_alphaf[c+x][c-y] = 0.04f;
		}
	}
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, blurAlpha);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 7, 7, GL_ALPHA, GL_FLOAT, blur_alphaf);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
#endif
}

pixel *render_packed_rgb(void *image, int width, int height, int cmp_size)
{
	unsigned char *tmp;
	pixel *res;
	int i;

	tmp = (unsigned char*)malloc(width*height*3);
	if (!tmp)
		return NULL;
	res = (pixel*)malloc(width*height*PIXELSIZE);
	if (!res)
	{
		free(tmp);
		return NULL;
	}

	i = width*height*3;
	if (BZ2_bzBuffToBuffDecompress((char *)tmp, (unsigned *)&i, (char *)image, cmp_size, 0, 0))
	{
		free(res);
		free(tmp);
		return NULL;
	}

	for (i=0; i<width*height; i++)
		res[i] = PIXRGB(tmp[3*i], tmp[3*i+1], tmp[3*i+2]);

	free(tmp);
	return res;
}

void draw_rgba_image(pixel *vid, unsigned char *data, int x, int y, float alpha)
{
	unsigned char w, h;
	int i, j;
	unsigned char r, g, b, a;
	if (!data) return;
	w = *(data++)&0xFF;
	h = *(data++)&0xFF;
	for (j=0; j<h; j++)
	{
		for (i=0; i<w; i++)
		{
			r = *(data++)&0xFF;
			g = *(data++)&0xFF;
			b = *(data++)&0xFF;
			a = *(data++)&0xFF;
			drawpixel(vid, x+i, y+j, r, g, b, (int)(a*alpha));
		}
	}
}

void draw_image(pixel *vid, pixel *img, int x, int y, int w, int h, int a)
{
	int i, j, r, g, b;
	if (!img) return;
	for (j=0; j<h; j++)
		for (i=0; i<w; i++)
		{
			r = PIXR(*img);
			g = PIXG(*img);
			b = PIXB(*img);
			drawpixel(vid, x+i, y+j, r, g, b, a);
			img++;
		}
}

void dim_copy(pixel *dst, pixel *src) //old persistent, unused
{
	int i,r,g,b;
	for (i=0; i<XRES*YRES; i++)
	{
		r = PIXR(src[i]);
		g = PIXG(src[i]);
		b = PIXB(src[i]);
		if (r>0)
			r--;
		if (g>0)
			g--;
		if (b>0)
			b--;
		dst[i] = PIXRGB(r,g,b);
	}
}

void dim_copy_pers(pixel *dst, pixel *src) //for persistent view, reduces rgb slowly
{
	int i,r,g,b;
	for (i=0; i<(XRES+BARSIZE)*YRES; i++)
	{
		r = PIXR(src[i]);
		g = PIXG(src[i]);
		b = PIXB(src[i]);
		if (r>0)
			r--;
		if (g>0)
			g--;
		if (b>0)
			b--;
		dst[i] = PIXRGB(r,g,b);
	}
}

void render_zoom(pixel *img) //draws the zoom box
{
#ifdef OGLR
	int origBlendSrc, origBlendDst;
	float zcx1, zcx0, zcy1, zcy0, yfactor, xfactor, i; //X-Factor is shit, btw
	xfactor = 1.0f/(float)XRES;
	yfactor = 1.0f/(float)YRES;

	zcx0 = (zoom_x)*xfactor;
	zcx1 = (zoom_x+ZSIZE)*xfactor;
	zcy0 = (zoom_y)*yfactor;
	zcy1 = ((zoom_y+ZSIZE))*yfactor;

	glGetIntegerv(GL_BLEND_SRC, &origBlendSrc);
	glGetIntegerv(GL_BLEND_DST, &origBlendDst);
	glBlendFunc(GL_ONE, GL_ZERO);

	glEnable( GL_TEXTURE_2D );
	//glReadBuffer(GL_AUX0);
	glBindTexture(GL_TEXTURE_2D, partsFboTex);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
	glTexCoord2d(zcx1, zcy1);
	glVertex3f((zoom_wx+ZSIZE*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR))*sdl_scale, 1.0);
	glTexCoord2d(zcx0, zcy1);
	glVertex3f(zoom_wx*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR))*sdl_scale, 1.0);
	glTexCoord2d(zcx0, zcy0);
	glVertex3f(zoom_wx*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale, 1.0);
	glTexCoord2d(zcx1, zcy0);
	glVertex3f((zoom_wx+ZSIZE*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale, 1.0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable( GL_TEXTURE_2D );
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glLineWidth(sdl_scale);
	glEnable(GL_LINE_SMOOTH);
	glBegin(GL_LINES);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	for(i = 0; i < ZSIZE; i++)
	{
		glVertex2f((zoom_wx+ZSIZE*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR)+i*ZFACTOR)*sdl_scale);
		glVertex2f(zoom_wx*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR)+i*ZFACTOR)*sdl_scale);
		glVertex2f((zoom_wx+i*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR))*sdl_scale);
		glVertex2f((zoom_wx+i*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale);
	}
	glEnd();
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3i((zoom_wx-1)*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale, 0);
	glVertex3i((zoom_wx-1)*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR))*sdl_scale, 0);
	glVertex3i((zoom_wx+ZSIZE*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-(zoom_wy+ZSIZE*ZFACTOR))*sdl_scale, 0);
	glVertex3i((zoom_wx+ZSIZE*ZFACTOR)*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale, 0);
	glVertex3i((zoom_wx-1)*sdl_scale, (YRES+MENUSIZE-zoom_wy)*sdl_scale, 0);
	glEnd();
	glDisable(GL_LINE_SMOOTH);
	
	glDisable(GL_LINE_SMOOTH);

	if(zoom_en)
	{	
		glEnable(GL_COLOR_LOGIC_OP);
		//glEnable(GL_LINE_SMOOTH);
		glLogicOp(GL_XOR);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBegin(GL_LINE_STRIP);
		glVertex3i((zoom_x-1)*sdl_scale, (YRES+MENUSIZE-(zoom_y-1))*sdl_scale, 0);
		glVertex3i((zoom_x-1)*sdl_scale, (YRES+MENUSIZE-(zoom_y+ZSIZE))*sdl_scale, 0);
		glVertex3i((zoom_x+ZSIZE)*sdl_scale, (YRES+MENUSIZE-(zoom_y+ZSIZE))*sdl_scale, 0);
		glVertex3i((zoom_x+ZSIZE)*sdl_scale, (YRES+MENUSIZE-(zoom_y-1))*sdl_scale, 0);
		glVertex3i((zoom_x-1)*sdl_scale, (YRES+MENUSIZE-(zoom_y-1))*sdl_scale, 0);
		glEnd();
		glDisable(GL_COLOR_LOGIC_OP);
	}
	glLineWidth(1);
	glBlendFunc(origBlendSrc, origBlendDst);
#else
	int x, y, i, j;
	pixel pix;
	drawrect(img, zoom_wx-2, zoom_wy-2, ZSIZE*ZFACTOR+2, ZSIZE*ZFACTOR+2, 192, 192, 192, 255);
	drawrect(img, zoom_wx-1, zoom_wy-1, ZSIZE*ZFACTOR, ZSIZE*ZFACTOR, 0, 0, 0, 255);
	clearrect(img, zoom_wx, zoom_wy, ZSIZE*ZFACTOR, ZSIZE*ZFACTOR);
	for (j=0; j<ZSIZE; j++)
		for (i=0; i<ZSIZE; i++)
		{
			pix = img[(j+zoom_y)*(XRES+BARSIZE)+(i+zoom_x)];
			for (y=0; y<ZFACTOR-1; y++)
				for (x=0; x<ZFACTOR-1; x++)
					img[(j*ZFACTOR+y+zoom_wy)*(XRES+BARSIZE)+(i*ZFACTOR+x+zoom_wx)] = pix;
		}
	if (zoom_en)
	{
		for (j=-1; j<=ZSIZE; j++)
		{
			xor_pixel(zoom_x+j, zoom_y-1, img);
			xor_pixel(zoom_x+j, zoom_y+ZSIZE, img);
		}
		for (j=0; j<ZSIZE; j++)
		{
			xor_pixel(zoom_x-1, zoom_y+j, img);
			xor_pixel(zoom_x+ZSIZE, zoom_y+j, img);
		}
	}
#endif
}

int render_thumb(void *thumb, int size, int bzip2, pixel *vid_buf, int px, int py, int scl)
{
	unsigned char *d,*c=(unsigned char*)thumb;
	int i,j,x,y,a,t,r,g,b,sx,sy;

	if (bzip2)
	{
		if (size<16)
			return 1;
		if (c[3]!=0x74 || c[2]!=0x49 || c[1]!=0x68 || c[0]!=0x53)
			return 1;
		if (c[4]>PT_NUM)
			return 2;
		if (c[5]!=CELL || c[6]!=XRES/CELL || c[7]!=YRES/CELL)
			return 3;
		i = XRES*YRES;
		d = (unsigned char*)malloc(i);
		if (!d)
			return 1;

		if (BZ2_bzBuffToBuffDecompress((char *)d, (unsigned *)&i, (char *)(c+8), size-8, 0, 0))
			return 1;
		size = i;
	}
	else
		d = c;

	if (size < XRES*YRES)
	{
		if (bzip2)
			free(d);
		return 1;
	}

	sy = 0;
	for (y=0; y+scl<=YRES; y+=scl)
	{
		sx = 0;
		for (x=0; x+scl<=XRES; x+=scl)
		{
			a = 0;
			r = g = b = 0;
			for (j=0; j<scl; j++)
				for (i=0; i<scl; i++)
				{
					t = d[(y+j)*XRES+(x+i)];
					if (t==0xFF)
					{
						r += 256;
						g += 256;
						b += 256;
						a += 2;
					}
					else if (t)
					{
						if (t>=PT_NUM)
							goto corrupt;
						r += COLR(globalSim->elements[t].Colour);
						g += COLG(globalSim->elements[t].Colour);
						b += COLB(globalSim->elements[t].Colour);
						a ++;
					}
				}
			if (a)
			{
				a = 256/a;
				r = (r*a)>>8;
				g = (g*a)>>8;
				b = (b*a)>>8;
			}

			drawpixel(vid_buf, px+sx, py+sy, r, g, b, 255);
			sx++;
		}
		sy++;
	}

	if (bzip2)
		free(d);
	return 0;

corrupt:
	if (bzip2)
		free(d);
	return 1;
}

//draws the cursor
void render_cursor(pixel *vid, int x, int y, Tool* tool, Brush* brush)
{
	int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
#ifdef OGLR
	int i;
	if (tool->GetType() == ELEMENT_TOOL || tool->GetType() == GOL_TOOL || tool->GetType() == TOOL_TOOL || tool->GetType() == DECO_TOOL)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp(GL_XOR);
		glBegin(GL_LINE_LOOP);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		y *= sdl_scale;
		x *= sdl_scale;
		ry *= sdl_scale;
		rx *= sdl_scale;
		if (brush->GetShape() == SQUARE_BRUSH)
		{
			glVertex2f(x-rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)-ry+1);
			glVertex2f(x+rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)-ry+1);
			glVertex2f(x+rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)+ry+1);
			glVertex2f(x-rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)+ry+1);
			glVertex2f(x-rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)-ry+1);
		}
		else if (brush->GetShape() == CIRCLE_BRUSH)
		{
			for (i = 0; i < 360; i++)
			{
			  float degInRad = i*(M_PI/180.0f);
			  glVertex2f((cos(degInRad)*rx)+x, (sin(degInRad)*ry)+/*(YRES+MENUSIZE)*sdl_scale-*/y);
			}
		}
		else if (brush->GetShape() == TRI_BRUSH)
		{
			glVertex2f(x+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)-ry-1);
			glVertex2f(x+rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)+ry-1);
			glVertex2f(x-rx+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)+ry-1);
			glVertex2f(x+1, (/*(YRES+MENUSIZE)*sdl_scale-*/y)-ry-1);
		}
		glEnd();
		glDisable(GL_COLOR_LOGIC_OP);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
#else
	int i,j;
	if ((sdl_mod & (KMOD_CTRL|KMOD_META)) && (sdl_mod & KMOD_SHIFT) && (tool->GetType() != TOOL_TOOL || ((ToolTool*)tool)->GetID() == TOOL_PROP))
	{
		for (i = -5; i < 6; i++)
			if (i != 0)
				xor_pixel(x+i, y, vid);
		for (j = -5; j < 6; j++)
			if (j != 0)
				xor_pixel(x, y+j, vid);
	}
	else if (tool->GetType() == WALL_TOOL)
	{
		int wallx = (x/CELL)*CELL, wally = (y/CELL)*CELL;
		int wallrx = (rx/CELL)*CELL, wallry = (ry/CELL)*CELL;
		xor_line(wallx-wallrx, wally-wallry, wallx+wallrx+CELL-1, wally-wallry, vid);
		xor_line(wallx-wallrx, wally+wallry+CELL-1, wallx+wallrx+CELL-1, wally+wallry+CELL-1, vid);
		xor_line(wallx-wallrx, wally-wallry+1, wallx-wallrx, wally+wallry+CELL-2, vid);
		xor_line(wallx+wallrx+CELL-1, wally-wallry+1, wallx+wallrx+CELL-1, wally+wallry+CELL-2, vid);
	}
	else
	{
		if (rx<=0)
			for (j = y - ry; j <= y + ry; j++)
				xor_pixel(x, j, vid);
		else
		{
			int tempy = y, i, j, oldy;
			if (brush->GetShape() == TRI_BRUSH)
				tempy = y + ry;
			for (i = x - rx; i <= x; i++) {
				oldy = tempy;
				while (brush->IsInside(i-x,tempy-y))
					tempy = tempy - 1;
				tempy = tempy + 1;
				if (oldy != tempy && brush->GetShape() != SQUARE_BRUSH)
					oldy--;
				if (brush->GetShape() == TRI_BRUSH)
					oldy = tempy;
				for (j = tempy; j <= oldy; j++) {
					int i2 = 2*x-i, j2 = 2*y-j;
					if (brush->GetShape() == TRI_BRUSH)
						j2 = y+ry;
					xor_pixel(i, j, vid);
					if (i2 != i)
						xor_pixel(i2, j, vid);
					if (j2 != j)
						xor_pixel(i, j2, vid);
					if (i2 != i && j2 != j)
						xor_pixel(i2, j2, vid);
				}
			}
		}
	}
#endif
}

int savedWindowX = INT_MAX;
int savedWindowY = INT_MAX;

// Returns 1 if the loaded position was set
// Returns 0 if something went wrong: SDL_GetWMInfo failed or the loaded position was invalid
int LoadWindowPosition(int scale)
{
#ifdef WIN
	SDL_SysWMinfo sysInfo;
	SDL_VERSION(&sysInfo.version);
	if (SDL_GetWMInfo(&sysInfo) > 0)
	{
		const SDL_VideoInfo * vidInfo = SDL_GetVideoInfo();
		int desktopWidth = vidInfo->current_w;
		int desktopHeight = vidInfo->current_h;

		int windowW = (XRES + BARSIZE) * scale;
		int windowH = (YRES + MENUSIZE) * scale;
		
		// Center the window on the primary desktop by default
		int newWindowX = (desktopWidth - windowW) / 2;
		int newWindowY = (desktopHeight - windowH) / 2;

		int success = 0;

		if (savedWindowX != INT_MAX && savedWindowY != INT_MAX)
		{
			POINT windowPoints[] = {
				{savedWindowX, savedWindowY},                       // Top-left
				{savedWindowX + windowW, savedWindowY + windowH}    // Bottom-right
			};

			MONITORINFO monitor;
			monitor.cbSize = sizeof(monitor);
			if (GetMonitorInfo(MonitorFromPoint(windowPoints[0], MONITOR_DEFAULTTONEAREST), &monitor) != 0)
			{
				// Only use the saved window position if it lies inside the visible screen
				if (PtInRect(&monitor.rcMonitor, windowPoints[0]) && PtInRect(&monitor.rcMonitor, windowPoints[1]))
				{
					newWindowX = savedWindowX;
					newWindowY = savedWindowY;

					success = 1;
				}
				else
				{
					// Center the window on the nearest monitor
					newWindowX = monitor.rcMonitor.left + (monitor.rcMonitor.right - monitor.rcMonitor.left - windowW) / 2;
					newWindowY = monitor.rcMonitor.top + (monitor.rcMonitor.bottom - monitor.rcMonitor.top - windowH) / 2;
				}
			}
		}
		
		SetWindowPos(sysInfo.window, 0, newWindowX, newWindowY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

		// 1 if we didn't use the default, i.e. the position was valid
		return success;
	}
#endif

	return 0;
}

// Returns 1 if the window position was saved
int SaveWindowPosition()
{
#ifdef WIN
	SDL_SysWMinfo sysInfo;
	SDL_VERSION(&sysInfo.version);
	if (SDL_GetWMInfo(&sysInfo) > 0)
	{
		WINDOWPLACEMENT placement;
		placement.length = sizeof(placement);
		GetWindowPlacement(sysInfo.window, &placement);

		savedWindowX = (int)placement.rcNormalPosition.left;
		savedWindowY = (int)placement.rcNormalPosition.top;

		return 1;
	}
#elif defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
	SDL_SysWMinfo sysInfo;
	SDL_VERSION(&sysInfo.version);
	if (SDL_GetWMInfo(&sysInfo) > 0)
	{
		Display *display;
		Window window, dummy;
		XWindowAttributes attributes;

		sysInfo.info.x11.lock_func();
		display = sysInfo.info.x11.display;
		window = sysInfo.info.x11.window;
		XSync(display, false);
		XGetWindowAttributes(display, window, &attributes);
		XTranslateCoordinates(display, window, attributes.root, 0, 0, &savedWindowX, &savedWindowY, &dummy);

		//some window managers don't include the border when getting position, so we adjust for that here
		unsigned long nitems, bytes_after;
		unsigned char *property;
		Atom type;
		int format;

		//adjust for window border. Some window managers will have get ignore the window border, so set needs to be adjusted to include it
		//this doesn't actually work though, for some reason. Possible alternative, xwininfo command?
		if (Success == XGetWindowProperty(display, window, XA_NET_FRAME_EXTENTS, 0, 16, 0, XA_CARDINAL, &type, &format, &nitems, &bytes_after, &property))
		{
			if (property)
			{
				long border_left = ((Atom*)property)[0];
				long border_top = ((Atom*)property)[2];

				savedWindowX -= border_left;
				savedWindowY -= border_top;

				XFree(property);
			}
			else
			{
				//hardcode my size :P
				savedWindowX -= 3;
				savedWindowY -= 22;
			}
		}

		sysInfo.info.x11.unlock_func();
		return 1;
	}
#endif

	return 0;
}

int sdl_opened = 0;
int sdl_open()
{
	//char screen_err = 0;
#ifdef WIN
	SDL_SysWMinfo SysInfo;
	HWND WindowHandle;
	HICON hIconSmall;
	HICON hIconBig;
#elif LIN
	char envStr[64];
	sprintf(envStr, "SDL_VIDEO_WINDOW_POS=%i,%i", savedWindowX, savedWindowY);
	SDL_putenv(envStr);
#endif
	if (SDL_Init(SDL_INIT_VIDEO)<0)
	{
		fprintf(stderr, "Initializing SDL: %s\n", SDL_GetError());
		return 0;
	}
	if (!sdl_opened && firstRun)
	{
		const SDL_VideoInfo *info = SDL_GetVideoInfo();
		if (sdl_scale == 1 && info->current_w > ((XRES+BARSIZE)*2) && info->current_h > ((YRES+MENUSIZE)*2))
			sdl_scale = 2;
	}
	
#ifdef WIN
	SDL_VERSION(&SysInfo.version);
	if(SDL_GetWMInfo(&SysInfo) <= 0) {
	    printf("%s : %d\n", SDL_GetError(), SysInfo.window);
	    exit(-1);
	}
	WindowHandle = SysInfo.window;
	hIconSmall = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 16, 16, LR_SHARED);
	hIconBig = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_SHARED);
	SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
	SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
#elif LIN
	SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void*)app_icon, 48, 48, 24, 144, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
	SDL_WM_SetIcon(icon, (Uint8*)app_icon_bitmap);
	SDL_FreeSurface(icon);
#endif
	SDL_WM_SetCaption("Jacob1's Mod", "Jacob1's Mod");
	
	atexit(SDL_Quit);

	LoadWindowPosition(sdl_scale);
#if defined(OGLR)
	sdl_scrn=SDL_SetVideoMode(XRES*sdl_scale + BARSIZE*sdl_scale,YRES*sdl_scale + MENUSIZE*sdl_scale,32,SDL_OPENGL);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	if(sdl_opened)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(0, (XRES+BARSIZE)*sdl_scale, 0, (YRES+MENUSIZE)*sdl_scale, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	else
	{
#ifdef WIN
		int status = glewInit();
		if(status != GLEW_OK)
		{
			fprintf(stderr, "Initializing Glew: %d\n", status);
			return 0;
		}
#endif
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(0, (XRES+BARSIZE)*sdl_scale, 0, (YRES+MENUSIZE)*sdl_scale, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glRasterPos2i(0, (YRES+MENUSIZE));
		glPixelZoom(1, -1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//FBO Texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &partsFboTex);
		glBindTexture(GL_TEXTURE_2D, partsFboTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, XRES, YRES, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		 
		//FBO
		glGenFramebuffers(1, &partsFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, partsFbo);
		glEnable(GL_BLEND);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, partsFboTex, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Reset framebuffer binding
		glDisable(GL_TEXTURE_2D);

		//Texture for main UI
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &vidBuf);
		glBindTexture(GL_TEXTURE_2D, vidBuf);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, XRES+BARSIZE, YRES+MENUSIZE, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Texture for air to be drawn
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &airBuf);
		glBindTexture(GL_TEXTURE_2D, airBuf);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Zoom texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &zoomTex);
		glBindTexture(GL_TEXTURE_2D, zoomTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Texture for velocity maps for gravity
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &partsTFX);
		glBindTexture(GL_TEXTURE_2D, partsTFX);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		glGenTextures(1, &partsTFY);
		glBindTexture(GL_TEXTURE_2D, partsTFY);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Texture for velocity maps for air
		//TODO: Combine all air maps into 3D array or structs
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &airVX);
		glBindTexture(GL_TEXTURE_2D, airVX);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		glGenTextures(1, &airVY);
		glBindTexture(GL_TEXTURE_2D, airVY);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		glGenTextures(1, &airPV);
		glBindTexture(GL_TEXTURE_2D, airPV);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES/CELL, YRES/CELL, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Fire alpha texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &fireAlpha);
		glBindTexture(GL_TEXTURE_2D, fireAlpha);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, CELL*3, CELL*3, 0, GL_ALPHA, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		//Glow alpha texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &glowAlpha);
		glBindTexture(GL_TEXTURE_2D, glowAlpha);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 11, 11, 0, GL_ALPHA, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);


		//Blur Alpha texture
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &blurAlpha);
		glBindTexture(GL_TEXTURE_2D, blurAlpha);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 7, 7, 0, GL_ALPHA, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		loadShaders();
	}
#else
#ifdef PIX16
	if (kiosk_enable)
		sdl_scrn=SDL_SetVideoMode(XRES*sdl_scale + BARSIZE*sdl_scale,YRES*sdl_scale + MENUSIZE*sdl_scale,16,SDL_FULLSCREEN|SDL_SWSURFACE);
	else
		sdl_scrn=SDL_SetVideoMode(XRES*sdl_scale + BARSIZE*sdl_scale,YRES*sdl_scale + MENUSIZE*sdl_scale,16,SDL_SWSURFACE);
#else
	if (kiosk_enable)
		sdl_scrn=SDL_SetVideoMode(XRES*sdl_scale + BARSIZE*sdl_scale,YRES*sdl_scale + MENUSIZE*sdl_scale,32,SDL_FULLSCREEN|SDL_SWSURFACE);
	else
		sdl_scrn=SDL_SetVideoMode(XRES*sdl_scale + BARSIZE*sdl_scale,YRES*sdl_scale + MENUSIZE*sdl_scale,32,SDL_SWSURFACE);
#endif
#endif
	if (!sdl_scrn)
	{
		fprintf(stderr, "Creating window: %s\n", SDL_GetError());
		return 0;
	}
	SDL_EnableUNICODE(1);
	//SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#if defined(LIN) && defined(SDL_VIDEO_DRIVER_X11)
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
	SDL_VERSION(&sdl_wminfo.version);
	SDL_GetWMInfo(&sdl_wminfo);
	sdl_wminfo.info.x11.lock_func();
	XA_CLIPBOARD = XInternAtom(sdl_wminfo.info.x11.display, "CLIPBOARD", 1);
	XA_TARGETS = XInternAtom(sdl_wminfo.info.x11.display, "TARGETS", 1);
	XA_UTF8_STRING = XInternAtom(sdl_wminfo.info.x11.display, "UTF8_STRING", 1);
	XA_NET_FRAME_EXTENTS = XInternAtom(sdl_wminfo.info.x11.display, "_NET_FRAME_EXTENTS", 0);
	sdl_wminfo.info.x11.unlock_func();
#elif WIN
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
#endif

	//if (screen_err)
	//	error_ui(vid_buf, 0, "Can't change scale factor, because screen resolution is too small");
	sdl_opened = 1;
	return 1;
}

int set_scale(int scale, int kiosk){
	int old_scale = sdl_scale, old_kiosk = kiosk_enable;
	sdl_scale = scale;
	kiosk_enable = kiosk;

	SaveWindowPosition();
#ifdef LIN
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
	if (!sdl_open())// || (size_error && !confirm_ui(vid_buf, "Confirm Size Change", "Your screen is too large, press OK to keep the size change anyway", "OK")))
	{
		sdl_scale = old_scale;
		kiosk_enable = old_kiosk;
		sdl_open();
		return 0;
	}
	return 1;
}

#ifdef OGLR
void checkShader(GLuint shader, char * shname)
{
	GLuint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[ GL_INFO_LOG_LENGTH];
		int errLen;
		glGetShaderInfoLog(shader, GL_INFO_LOG_LENGTH, &errLen, errorBuf);
		fprintf(stderr, "Failed to compile %s shader:\n%s\n", shname, errorBuf);
		exit(1);
	}
}
void checkProgram(GLuint program, char * progname)
{
	GLuint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char errorBuf[ GL_INFO_LOG_LENGTH];
		int errLen;
		glGetShaderInfoLog(program, GL_INFO_LOG_LENGTH, &errLen, errorBuf);
		fprintf(stderr, "Failed to link %s program:\n%s\n", progname, errorBuf);
		exit(1);
	}
}
void loadShaders()
{
	GLuint vertexShader, fragmentShader;

	//Particle texture
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &fireVertex, NULL);
	glShaderSource( fragmentShader, 1, &fireFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "FV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "FF");

	fireProg = glCreateProgram();
	glAttachShader( fireProg, vertexShader );
	glAttachShader( fireProg, fragmentShader );
	glLinkProgram( fireProg );
	checkProgram(fireProg, "F");
	
	//Lensing
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &lensVertex, NULL);
	glShaderSource( fragmentShader, 1, &lensFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "LV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "LF");

	lensProg = glCreateProgram();
	glAttachShader( lensProg, vertexShader );
	glAttachShader( lensProg, fragmentShader );
	glLinkProgram( lensProg );
	checkProgram(lensProg, "L");
	
	//Air Velocity
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airVVertex, NULL);
	glShaderSource( fragmentShader, 1, &airVFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "AVX");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "AVF");

	airProg_Velocity = glCreateProgram();
	glAttachShader( airProg_Velocity, vertexShader );
	glAttachShader( airProg_Velocity, fragmentShader );
	glLinkProgram( airProg_Velocity );
	checkProgram(airProg_Velocity, "AV");
	
	//Air Pressure
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airPVertex, NULL);
	glShaderSource( fragmentShader, 1, &airPFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "APV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "APF");

	airProg_Pressure = glCreateProgram();
	glAttachShader( airProg_Pressure, vertexShader );
	glAttachShader( airProg_Pressure, fragmentShader );
	glLinkProgram( airProg_Pressure );
	checkProgram(airProg_Pressure, "AP");
	
	//Air cracker
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource( vertexShader, 1, &airCVertex, NULL);
	glShaderSource( fragmentShader, 1, &airCFragment, NULL);

	glCompileShader( vertexShader );
	checkShader(vertexShader, "ACV");
	glCompileShader( fragmentShader );
	checkShader(fragmentShader, "ACF");

	airProg_Cracker = glCreateProgram();
	glAttachShader( airProg_Cracker, vertexShader );
	glAttachShader( airProg_Cracker, fragmentShader );
	glLinkProgram( airProg_Cracker );
	checkProgram(airProg_Cracker, "AC");
}
#endif

float maxAverage = 0.0f; //for debug mode
int draw_debug_info(pixel* vid, int lm, int lx, int ly, int cx, int cy, int line_x, int line_y)
{
	char infobuf[256];
	if(debug_flags & DEBUG_PERFORMANCE_FRAME || debug_flags & DEBUG_PERFORMANCE_CALC)
	{
		int t1, t2, x = 0, i = debug_perf_istart;
		float partiavg = 0, frameavg = 0;
		while(i != debug_perf_iend)
		{
			partiavg += abs(debug_perf_partitime[i]/100000);
			frameavg += abs(debug_perf_frametime[i]/100000);
			if(debug_flags & DEBUG_PERFORMANCE_CALC)
				t1 = abs(debug_perf_partitime[i]/100000);
			else
				t1 = 0;
				
			if(debug_flags & DEBUG_PERFORMANCE_FRAME)
				t2 = abs(debug_perf_frametime[i]/100000);
			else
				t2 = 0;
				
			if(t1 > YRES)
				t1 = YRES;
			if(t1+t2 > YRES)
				t2 = YRES-t1;
				
			if(t1>0)
				draw_line(vid, x, YRES, x, YRES-t1, 0, 255, 120, XRES+BARSIZE);
			if(t2>0)	
				draw_line(vid, x, YRES-t1, x, YRES-(t1+t2), 255, 120, 0, XRES+BARSIZE);
				
			i++;
			x++;
			i %= DEBUG_PERF_FRAMECOUNT;
		}
		
		if(debug_flags & DEBUG_PERFORMANCE_CALC)
			t1 = abs((int)(partiavg / x));
		else
			t1 = 0;
			
		if(debug_flags & DEBUG_PERFORMANCE_FRAME)
			t2 = abs((int)(frameavg / x));
		else
			t2 = 0;
		
		if(t1 > YRES)
			t1 = YRES;
		if(t1+t2 > YRES)
			t2 = YRES-t1;
		
		if(t1>0)
			fillrect(vid, x, YRES-t1-1, 5, t1+2, 0, 255, 0, 255);
		if(t2>0)	
			fillrect(vid, x, (YRES-t1)-t2-1, 5, t2+1, 255, 0, 0, 255);
	}
	if(debug_flags & DEBUG_DRAWTOOL)
	{
		if(lm == 1) //Line tool
		{
			blend_line(vid, 0, line_y, XRES, line_y, 255, 255, 255, 120);
			blend_line(vid, line_x, 0, line_x, YRES, 255, 255, 255, 120);
	
			blend_line(vid, 0, ly, XRES, ly, 255, 255, 255, 120);
			blend_line(vid, lx, 0, lx, YRES, 255, 255, 255, 120);
			
			sprintf(infobuf, "%d x %d", lx, ly);
			drawtext_outline(vid, lx+(lx>line_x?3:-textwidth(infobuf)-3), ly+(ly<line_y?-10:3), infobuf, 255, 255, 255, 200, 0, 0, 0, 120);
			
			sprintf(infobuf, "%d x %d", line_x, line_y);
			drawtext_outline(vid, line_x+(lx<line_x?3:-textwidth(infobuf)-2), line_y+(ly>line_y?-10:3), infobuf, 255, 255, 255, 200, 0, 0, 0, 120);
			
			sprintf(infobuf, "%d", abs(line_x-lx));
			drawtext_outline(vid, (line_x+lx)/2-textwidth(infobuf)/2, line_y+(ly>line_y?-10:3), infobuf, 255, 255, 255, 200, 0, 0, 0, 120);
			
			sprintf(infobuf, "%d", abs(line_y-ly));
			drawtext_outline(vid, line_x+(lx<line_x?3:-textwidth(infobuf)-2), (line_y+ly)/2-3, infobuf, 255, 255, 255, 200, 0, 0, 0, 120);
		}
	}
	if (debug_flags & DEBUG_ELEMENTPOPULATION)
	{
		int yBottom = YRES-10;
		int xStart = 10;

		std::string maxValString;
		std::string halfValString;
		std::stringstream numtostring;


		int maxVal = 255;
		float scale = 1.0f;
		int bars = 0;
		for (int i = 0; i < PT_NUM; i++)
		{
			if (globalSim->elements[i].Enabled)
			{
				if (maxVal < globalSim->elementCount[i])
					maxVal = globalSim->elementCount[i];
				bars++;
			}
		}
		maxAverage = (maxAverage*(1.0f-0.015f)) + (0.015f*maxVal);
		scale = 255.0f/maxAverage;

		numtostring << maxAverage;
		maxValString = numtostring.str();
		numtostring.str("");
		numtostring << maxAverage/2;
		halfValString = numtostring.str();


		fillrect(vid_buf, xStart-5, yBottom - 263, bars+10+textwidth(maxValString.c_str())+10, 255 + 13, 0, 0, 0, 180);

		bars = 0;
		for (int i = 0; i < PT_NUM; i++)
		{
			if (globalSim->elements[i].Enabled)
			{
				int count = globalSim->elementCount[i];
				int barSize = (int)(count * scale - 0.5f);
				int barX = bars;//*2;

				draw_line(vid_buf, xStart+barX, yBottom+3, xStart+barX, yBottom+2, COLR(globalSim->elements[i].Colour), COLG(globalSim->elements[i].Colour), COLB(globalSim->elements[i].Colour), XRES+BARSIZE);
				if (globalSim->elementCount[i])
				{
					if (barSize > 256)
					{
						barSize = 256;
						blendpixel(vid_buf, xStart+barX, yBottom-barSize-3, COLR(globalSim->elements[i].Colour), COLG(globalSim->elements[i].Colour), COLB(globalSim->elements[i].Colour), 255);
						blendpixel(vid_buf, xStart+barX, yBottom-barSize-5, COLR(globalSim->elements[i].Colour), COLG(globalSim->elements[i].Colour), COLB(globalSim->elements[i].Colour), 255);
						blendpixel(vid_buf, xStart+barX, yBottom-barSize-7, COLR(globalSim->elements[i].Colour), COLG(globalSim->elements[i].Colour), COLB(globalSim->elements[i].Colour), 255);
					}
					else
						draw_line(vid_buf, xStart+barX, yBottom-barSize-3, xStart+barX, yBottom-barSize-2, 255, 255, 255, XRES+BARSIZE);
					draw_line(vid_buf, xStart+barX, yBottom-barSize, xStart+barX, yBottom, COLR(globalSim->elements[i].Colour), COLG(globalSim->elements[i].Colour), COLB(globalSim->elements[i].Colour), XRES+BARSIZE);
				}
				bars++;
			}
		}

		drawtext(vid_buf, xStart + bars + 5, yBottom-5, "0", 255, 255, 255, 255);
		drawtext(vid_buf, xStart + bars + 5, yBottom-132, halfValString.c_str(), 255, 255, 255, 255);
		drawtext(vid_buf, xStart + bars + 5, yBottom-260, maxValString.c_str(), 255, 255, 255, 255);
	}
	if(debug_flags & DEBUG_PARTS)
	{
		int i = 0, x = 0, y = 0, lpx = 0, lpy = 0;
		sprintf(infobuf, "%d/%d (%.2f%%)", globalSim->parts_lastActiveIndex, NPART, (((float)globalSim->parts_lastActiveIndex)/((float)NPART))*100.0f);
		for(i = 0; i < NPART; i++){
			if(parts[i].type){
				drawpixel(vid, x, y, 255, 255, 255, 180);
			} else {
				drawpixel(vid, x, y, 0, 0, 0, 180);
			}
			if(i == globalSim->parts_lastActiveIndex)
			{
				lpx = x;
				lpy = y;
			}
			x++;
			if(x>=XRES){
				y++;
				x = 0;
			}
		}
		draw_line(vid, 0, lpy, XRES, lpy, 0, 255, 120, XRES+BARSIZE);
		draw_line(vid, lpx, 0, lpx, YRES, 0, 255, 120, XRES+BARSIZE);
		drawpixel(vid, lpx, lpy, 255, 50, 50, 220);
				
		drawpixel(vid, lpx+1, lpy, 255, 50, 50, 120);
		drawpixel(vid, lpx-1, lpy, 255, 50, 50, 120);
		drawpixel(vid, lpx, lpy+1, 255, 50, 50, 120);
		drawpixel(vid, lpx, lpy-1, 255, 50, 50, 120);
		
		fillrect(vid, 7, YRES-26, textwidth(infobuf)+5, 14, 0, 0, 0, 180);		
		drawtext(vid, 10, YRES-22, infobuf, 255, 255, 255, 255);
	}
	return 0;
}
