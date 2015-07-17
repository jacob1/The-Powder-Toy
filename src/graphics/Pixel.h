/*
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

/* This is the pixel format, used in the VideoBuffer class which is
 * rendered to the screen in SDL_blit
 *
 * It doesn't support alpha, because the new interface code only has
 * what we minimally need. Only the default option is really tested. Use
 * PIXCONV to convert from the pixel format into an RGB colour and PIXPACK
 * to convert from RGB format into pixel format.
 */

#ifndef PIXEL_H
#define PIXEL_H

#define PIXELSIZE 4
typedef unsigned int pixel;

#ifdef PIX32BGR

#define PIXPACK(x) ((((x)>>16)&0x0000FF)|((x)&0x00FF00)|(((x)<<16)&0xFF0000))
#define PIXCONV(x) ((((x)<<16)&0xFF0000)|((x)&0x00FF00)|(((x)>>16)&0x0000FF))
#define PIXRGB(r,g,b) (((b)<<16)|((g)<<8)|((r)))
#define PIXR(x) ((x)&0xFF)
#define PIXG(x) (((x)>>8)&0xFF)
#define PIXB(x) ((x)>>16)

#elif defined(PIX32BGRA)

#define PIXPACK(x) ((((x)>>8)&0x0000FF00)|(((x)<<8)&0x00FF0000)|(((x)<<24)&0xFF000000))
#define PIXCONV(x) ((((x)<<8)&0x00FF0000)|(((x)>>8)&0x0000FF00)|(((x)>>24)&0x000000FF))
#define PIXRGB(r,g,b) (((b)<<24)|((g)<<16)|((r)<<8))
#define PIXR(x) (((x)>>8)&0xFF)
#define PIXG(x) (((x)>>16)&0xFF)
#define PIXB(x) (((x)>>24))

#else

#define PIXPACK(x) (x&0x00FFFFFF)
#define PIXCONV(x) (x&0x00FFFFFF)
#define PIXRGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define PIXR(x) (((x)>>16)&0xFF)
#define PIXG(x) (((x)>>8)&0xFF)
#define PIXB(x) ((x)&0xFF)

#endif

char * generate_gradient(pixel * colours, float * points, int pointcount, int size);
void * ptif_pack(pixel *src, int w, int h, int *result_size);
pixel * ptif_unpack(void *datain, int size, int *w, int *h);
pixel * resample_img_nn(pixel *src, int sw, int sh, int rw, int rh);
pixel * resample_img(pixel *src, int sw, int sh, int rw, int rh);
pixel * rescale_img(pixel *src, int sw, int sh, int *qw, int *qh, int f);

#endif // PIXEL_H
