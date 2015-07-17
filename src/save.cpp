/**
 * Powder Toy - saving and loading functions
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

#include <bzlib.h>
#include <math.h>
#include "defines.h"
#include "powder.h"
#include "save.h"
#include "gravity.h"
#include "BSON.h"
#include "hmap.h"
#include "interface.h"
#include "luaconsole.h"

#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"
#include "simulation/ToolNumbers.h"
#include "simulation/GolNumbers.h"
#include "simulation/elements/FIGH.h"
#include "simulation/elements/PPIP.h"
#include "simulation/elements/LIFE.h"
#include "simulation/elements/MOVS.h"

//Pop
pixel *prerender_save(void *save, int size, int *width, int *height)
{
	unsigned char * saveData = (unsigned char*)save;
	if (size < 16 || !save)
	{
		return NULL;
	}
	if(saveData[0] == 'O' && saveData[1] == 'P' && (saveData[2] == 'S' || saveData[2] == 'J'))
	{
		return prerender_save_OPS(save, size, width, height);
	}
	else if((saveData[0]==0x66 && saveData[1]==0x75 && saveData[2]==0x43) || (saveData[0]==0x50 && saveData[1]==0x53 && saveData[2]==0x76))
	{
		return prerender_save_PSv(save, size, width, height);
	}
	return NULL;
}

int parse_save(void *save, int size, int replace, int x0, int y0, unsigned char bmap[YRES/CELL][XRES/CELL], float vx[YRES/CELL][XRES/CELL], float vy[YRES/CELL][XRES/CELL], float pv[YRES/CELL][XRES/CELL], float fvx[YRES/CELL][XRES/CELL], float fvy[YRES/CELL][XRES/CELL], sign signs[MAXSIGNS], void* partsptr, unsigned pmap[YRES][XRES])
{
	unsigned char * saveData = (unsigned char*)save;
	if (size<16)
	{
		return 1;
	}
	globalSim->forceStackingCheck = 1;//check for excessive stacking of particles next time update_particles is run
	((PPIP_ElementDataContainer*)globalSim->elementData[PT_PPIP])->ppip_changed = 1;
	if(saveData[0] == 'O' && saveData[1] == 'P' && (saveData[2] == 'S' || saveData[2] == 'J'))
	{
		return parse_save_OPS(save, size, replace, x0, y0, bmap, vx, vy, pv, fvx, fvy, signs, partsptr, pmap);
	}
	else if((saveData[0]==0x66 && saveData[1]==0x75 && saveData[2]==0x43) || (saveData[0]==0x50 && saveData[1]==0x53 && saveData[2]==0x76))
	{
		return parse_save_PSv(save, size, replace, x0, y0, bmap, fvx, fvy, signs, partsptr, pmap);
	}
	return 1;
}

int fix_type(int type, int version, int modver, int (elementPalette)[PT_NUM])
{
	if (elementPalette)
	{
		if (elementPalette[type] >= 0)
			type = elementPalette[type];
	}
	else if (modver)
	{
		int max = 161;
		if (modver == 18)
		{
			if (type >= 190 && type <= 204)
				return PT_LOLZ;
		}

		if (version >= 90)
			max = 179;
		else if (version >= 89 || modver >= 16)
			max = 177;
		else if (version >= 87)
			max = 173;
		else if (version >= 86 || modver == 14)
			max = 170;
		else if (version >= 84 || modver == 13)
			max = 167;
		else if (modver == 12)
			max = 165;
		else if (version >= 83)
			max = 163;
		else if (version >= 82)
			max = 162;

		if (type >= max)
		{
			type += (PT_NORMAL_NUM-max);
		}
		//change VIRS into official elements, and CURE into SOAP; adjust ids
		if (modver <= 15)
		{
			if (type >= PT_NORMAL_NUM+6 && type <= PT_NORMAL_NUM+8)
				type = PT_VIRS + type-(PT_NORMAL_NUM+6);
			else if (type == PT_NORMAL_NUM+9)
				type = PT_SOAP;
			else if (type > PT_NORMAL_NUM+9)
				type -= 4;
		}
		//change GRVT and DRAY into official elements
		if (modver <= 19)
		{
			if (type >= PT_NORMAL_NUM+12 && type <= PT_NORMAL_NUM+13)
				type -= 14;
		}
		//change OTWR and COND into METL, adjust ids
		if (modver <= 20)
		{
			if (type == PT_NORMAL_NUM+3 || type == PT_NORMAL_NUM+9)
				type = PT_METL;
			else if (type > PT_NORMAL_NUM+3 && type < PT_NORMAL_NUM+9)
				type--;
			else if (type > PT_NORMAL_NUM+9)
				type -= 2;
		}
	}
	return type;
}

int invalid_element(int save_as, int el)
{
	if (save_as > 0 && (el >= PT_NORMAL_NUM || el < 0 || ptypes[el].enabled == 0)) //Check for mod/disabled elements
		return 1;
#ifdef BETA
	//if (save_as > 1 && (el == PT_GRVT || el == PT_DRAY))
	//	return 1;
#endif
	return 0;
}

//checks all elements and ctypes/tmps of certain elements to make sure there are no mod/beta elements in a save or stamp
int check_save(int save_as, int orig_x0, int orig_y0, int orig_w, int orig_h, int give_warning)
{
	int i, x0, y0, w, h, bx0=orig_x0/CELL, by0=orig_y0/CELL, bw=(orig_w+orig_x0-bx0*CELL+CELL-1)/CELL, bh=(orig_h+orig_y0-by0*CELL+CELL-1)/CELL;
	x0 = bx0*CELL;
	y0 = by0*CELL;
	w  = bw *CELL;
	h  = bh *CELL;

	for (i=0; i<NPART; i++)
	{
		if ((int)(parts[i].x+.5f) > x0 && (int)(parts[i].x+.5f) < x0+w && (int)(parts[i].y+.5f) > y0 && (int)(parts[i].y+.5f) < y0+h)
		{
			if (invalid_element(save_as,parts[i].type))
			{
				if (give_warning)
				{
					char errortext[256] = "", elname[24] = "";
					if (parts[i].type > 0 && parts[i].type < PT_NUM)
						sprintf(elname, "%s", ptypes[parts[i].type].name);
					else
						sprintf(elname, "invalid element number %i", parts[i].type);
					sprintf(errortext,"Found %s at X:%i Y:%i, cannot save",elname,(int)(parts[i].x+.5),(int)(parts[i].y+.5));
					error_ui(vid_buf,0,errortext);
				}
				return 1;
			}
			if ((parts[i].type == PT_CLNE || parts[i].type == PT_PCLN || parts[i].type == PT_BCLN || parts[i].type == PT_PBCN || parts[i].type == PT_STOR || parts[i].type == PT_CONV || ((parts[i].type == PT_STKM || parts[i].type == PT_STKM2 || parts[i].type == PT_FIGH) && parts[i].ctype != SPC_AIR) || parts[i].type == PT_LAVA || parts[i].type == PT_SPRK || parts[i].type == PT_PSTN || parts[i].type == PT_CRAY || parts[i].type == PT_DTEC) && invalid_element(save_as,parts[i].ctype))
			{
				if (give_warning)
				{
					char errortext[256] = "", elname[24] = "";
					if (parts[i].ctype > 0 && parts[i].ctype < PT_NUM)
						sprintf(elname, "%s", ptypes[parts[i].ctype].name);
					else
						sprintf(elname, "invalid elnumber %i", parts[i].ctype);
					sprintf(errortext,"Found %s at X:%i Y:%i, cannot save",elname,(int)(parts[i].x+.5),(int)(parts[i].y+.5));
					error_ui(vid_buf,0,errortext);
				}
				return 1;
			}
			if ((parts[i].type == PT_PIPE || parts[i].type == PT_PPIP) && invalid_element(save_as,parts[i].tmp&0xFF))
			{
				if (give_warning)
				{
					char errortext[256] = "", elname[24] = "";
					if ((parts[i].tmp&0xFF) > 0 && (parts[i].tmp&0xFF) < PT_NUM)
						sprintf(elname, "%s", ptypes[parts[i].tmp&0xFF].name);
					else
						sprintf(elname, "invalid element number %i", parts[i].tmp&0xFF);
					sprintf(errortext,"Found %s at X:%i Y:%i, cannot save",elname,(int)(parts[i].x+.5),(int)(parts[i].y+.5));
					error_ui(vid_buf,0,errortext);
				}
				return 1;
			}
		}
	}
	for (int x = 0; x < XRES/CELL; x++)
		for (int y = 0; y < YRES/CELL; y++)
		{
			if (bmap[y][x] == WL_BLOCKAIR)
			{
				if (give_warning)
				{
					char errortext[256] = "";
					sprintf(errortext, "Found air blocking wall at X:%i Y:%i, cannot save", x*CELL, y*CELL);
				}
				return 1;
			}
		}
	return 0;
}

int change_wall(int wt)
{
	if (wt == 1)
		return WL_WALL;
	else if (wt == 2)
		return WL_DESTROYALL;
	else if (wt == 3)
		return WL_ALLOWLIQUID;
	else if (wt == 4)
		return WL_FAN;
	else if (wt == 5)
		return WL_STREAM;
	else if (wt == 6)
		return WL_DETECT;
	else if (wt == 7)
		return WL_EWALL;
	else if (wt == 8)
		return WL_WALLELEC;
	else if (wt == 9)
		return WL_ALLOWAIR;
	else if (wt == 10)
		return WL_ALLOWSOLID;
	else if (wt == 11)
		return WL_ALLOWALLELEC;
	else if (wt == 12)
		return WL_EHOLE;
	else if (wt == 13)
		return WL_ALLOWGAS;
	return wt;
}

int change_wallpp(int wt)
{
	if (wt == O_WL_WALLELEC)
		return WL_WALLELEC;
	else if (wt == O_WL_EWALL)
		return WL_EWALL;
	else if (wt == O_WL_DETECT)
		return WL_DETECT;
	else if (wt == O_WL_STREAM)
		return WL_STREAM;
	else if (wt == O_WL_FAN)
		return WL_FAN;
	else if (wt == O_WL_ALLOWLIQUID)
		return WL_ALLOWLIQUID;
	else if (wt == O_WL_DESTROYALL)
		return WL_DESTROYALL;
	else if (wt == O_WL_WALL)
		return WL_WALL;
	else if (wt == O_WL_ALLOWAIR)
		return WL_ALLOWAIR;
	else if (wt == O_WL_ALLOWSOLID)
		return WL_ALLOWSOLID;
	else if (wt == O_WL_ALLOWALLELEC)
		return WL_ALLOWALLELEC;
	else if (wt == O_WL_EHOLE)
		return WL_EHOLE;
	else if (wt == O_WL_ALLOWGAS)
		return WL_ALLOWGAS;
	else if (wt == O_WL_GRAV)
		return WL_GRAV;
	else if (wt == O_WL_ALLOWENERGY)
		return WL_ALLOWENERGY;
	return wt;
}

pixel *prerender_save_OPS(void *save, int size, int *width, int *height)
{
	unsigned char * inputData = (unsigned char*)save, *bsonData = NULL, *partsData = NULL, *partsPosData = NULL, *wallData = NULL;
	int inputDataLen = size, bsonDataLen = 0, partsDataLen, partsPosDataLen, wallDataLen;
	int i, x, y, j, type, ctype, wt, pc, gc, modsave = 0, saved_version = inputData[4];
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int bsonInitialised = 0;
	int elementPalette[PT_NUM];
	bool hasPalette = false;
	pixel * vidBuf = NULL;
	bson b;
	bson_iterator iter;

	//Block sizes
	blockX = 0;
	blockY = 0;
	blockW = inputData[6];
	blockH = inputData[7];
	
	//Full size, normalised
	fullX = 0;
	fullY = 0;
	fullW = blockW*CELL;
	fullH = blockH*CELL;
	
	//
	*width = fullW;
	*height = fullH;
	
	//From newer version
	if (saved_version > SAVE_VERSION && saved_version != 87 && saved_version != 222)
	{
		fprintf(stderr, "Save from newer version\n");
		//goto fail;
	}
		
	//Incompatible cell size
	if(inputData[5] != CELL)
	{
		fprintf(stderr, "Cell size mismatch: expected %i but got %i\n", inputData[5]);
		goto fail;
	}
		
	//Too large/off screen
	if(blockX+blockW > XRES/CELL || blockY+blockH > YRES/CELL)
	{
		fprintf(stderr, "Save too large\n");
		goto fail;
	}
	
	bsonDataLen = ((unsigned)inputData[8]);
	bsonDataLen |= ((unsigned)inputData[9]) << 8;
	bsonDataLen |= ((unsigned)inputData[10]) << 16;
	bsonDataLen |= ((unsigned)inputData[11]) << 24;
	
	bsonData = (unsigned char*)malloc(bsonDataLen+1);
	if(!bsonData)
	{
		fprintf(stderr, "Internal error while parsing save: could not allocate buffer\n");
		goto fail;
	}
	//Make sure bsonData is null terminated, since all string functions need null terminated strings
	//(bson_iterator_key returns a pointer into bsonData, which is then used with strcmp)
	bsonData[bsonDataLen] = 0;
	
	if (BZ2_bzBuffToBuffDecompress((char*)bsonData, (unsigned int*)(&bsonDataLen), (char*)inputData+12, inputDataLen-12, 0, 0))
	{
		fprintf(stderr, "Unable to decompress\n");
		goto fail;
	}
	
	bson_init_data(&b, (char*)bsonData);
	bsonInitialised = 1;
	bson_iterator_init(&iter, &b);
	while(bson_iterator_next(&iter))
	{
		if (!strcmp(bson_iterator_key(&iter), "parts"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		if (!strcmp(bson_iterator_key(&iter), "partsPos"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsPosDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsPosData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle position data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "wallMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (wallDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				wallData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of wall data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "palette"))
		{
			if(bson_iterator_type(&iter)==BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while(bson_iterator_next(&subiter))
				{
					if(bson_iterator_type(&subiter)==BSON_INT)
					{
						std::string identifier = std::string(bson_iterator_key(&subiter));
						int ID = -1, oldID = bson_iterator_int(&subiter);
						for (int i = 0; i < PT_NUM; i++)
							if (!identifier.compare(globalSim->elements[i].Identifier))
							{
								ID = i;
								break;
							}

						if (oldID >= 0 && oldID < PT_NUM)
							elementPalette[oldID] = ID;
					}
				}
				hasPalette = true;
			}
			else
			{
				fprintf(stderr, "Wrong type for element palette: %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_ARRAY);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "compatible_with"))
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				if (bson_iterator_int(&iter) > MOD_SAVE_VERSION)
				{
					fprintf(stderr, "Save is not compatible\n");
					goto fail;
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "Jacob1's_Mod"))
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				modsave = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
	}
	
	vidBuf = (pixel*)calloc(fullW*fullH, PIXELSIZE);
	
	//Read wall and fan data
	if(wallData)
	{
		if(blockW * blockH > wallDataLen)
		{
			fprintf(stderr, "Not enough wall data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				if(wallData[y*blockW+x])
				{
					wt = change_wallpp(wallData[y*blockW+x]);
					if (wt < 0 || wt >= WALLCOUNT)
						continue;
					pc = PIXPACK(wallTypes[wt].colour);
					gc = PIXPACK(wallTypes[wt].eglow);
					if (wallTypes[wt].drawstyle==1)
					{
						for (i=0; i<CELL; i+=2)
							for (j=(i>>1)&1; j<CELL; j+=2)
								vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
					}
					else if (wallTypes[wt].drawstyle==2)
					{
						for (i=0; i<CELL; i+=2)
							for (j=0; j<CELL; j+=2)
								vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
					}
					else if (wallTypes[wt].drawstyle==3)
					{
						for (i=0; i<CELL; i++)
							for (j=0; j<CELL; j++)
								vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
					}
					else if (wallTypes[wt].drawstyle==4)
					{
						for (i=0; i<CELL; i++)
							for (j=0; j<CELL; j++)
								if(i == j)
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
								else if  (j == i+1 || (j == 0 && i == CELL-1))
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = gc;
								else 
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = PIXPACK(0x202020);
					}

					// special rendering for some walls
					if (wt==WL_EWALL)
					{
						for (i=0; i<CELL; i++)
							for (j=0; j<CELL; j++)
								if (!(i&j&1))
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
					}
					else if (wt==WL_WALLELEC)
					{
						for (i=0; i<CELL; i++)
							for (j=0; j<CELL; j++)
							{
								if (!((y*CELL+j)%2) && !((x*CELL+i)%2))
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = pc;
								else
									vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = PIXPACK(0x808080);
							}
					}
					else if (wt==WL_EHOLE)
					{
						for (i=0; i<CELL; i+=2)
							for (j=0; j<CELL; j+=2)
								vidBuf[(fullY+i+(y*CELL))*fullW+(fullX+j+(x*CELL))] = PIXPACK(0x242424);
					}
				}
			}
		}
	}
	
	//Read particle data
	if(partsData && partsPosData)
	{
		int fieldDescriptor;
		int posCount, posTotal, partsPosDataIndex = 0;
		int saved_x, saved_y;
		if(fullW * fullH * 3 > partsPosDataLen)
		{
			fprintf(stderr, "Not enough particle position data\n");
			goto fail;
		}
		i = 0;
		for (saved_y=0; saved_y<fullH; saved_y++)
		{
			for (saved_x=0; saved_x<fullW; saved_x++)
			{
				//Read total number of particles at this position
				posTotal = 0;
				posTotal |= partsPosData[partsPosDataIndex++]<<16;
				posTotal |= partsPosData[partsPosDataIndex++]<<8;
				posTotal |= partsPosData[partsPosDataIndex++];
				//Put the next posTotal particles at this position
				for (posCount=0; posCount<posTotal; posCount++)
				{
					//i+3 because we have 4 bytes of required fields (type (1), descriptor (2), temp (1))
					if (i+3 >= partsDataLen)
						goto fail;
					x = saved_x + fullX;
					y = saved_y + fullY;
					fieldDescriptor = partsData[i+1];
					fieldDescriptor |= partsData[i+2] << 8;
					if(x >= XRES || x < 0 || y >= YRES || y < 0)
					{
						fprintf(stderr, "Out of range [%d]: %d %d, [%d, %d], [%d, %d]\n", i, x, y, (unsigned)partsData[i+1], (unsigned)partsData[i+2], (unsigned)partsData[i+3], (unsigned)partsData[i+4]);
						goto fail;
					}
					type = fix_type(partsData[i],saved_version, modsave, hasPalette ? elementPalette : NULL);
					if (type < 0 || type >= PT_NUM || !globalSim->elements[type].Enabled)
						type = PT_NONE; //invalid element
					
					//Draw type
					if (type==PT_STKM || type==PT_STKM2 || type==PT_FIGH)
					{
						pixel lc, hc=PIXRGB(255, 224, 178);
						if (type==PT_STKM || type==PT_FIGH) lc = PIXRGB(255, 255, 255);
						else lc = PIXRGB(100, 100, 255);
						//only need to check upper bound of y coord - lower bounds and x<w are checked in draw_line
						if (type==PT_STKM || type==PT_STKM2)
						{
							draw_line(vidBuf, x-2, y-2, x+2, y-2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							if (y+2<*height)
							{
								draw_line(vidBuf, x-2, y+2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
								draw_line(vidBuf, x-2, y-2, x-2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
								draw_line(vidBuf, x+2, y-2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							}
						}
						else if (y+2<*height)
						{
							draw_line(vidBuf, x-2, y, x, y-2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x-2, y, x, y+2, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x, y-2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), *width);
							draw_line(vidBuf, x, y+2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), *width);
						}
						if (y+6<*height)
						{
							draw_line(vidBuf, x, y+3, x-1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), *width);
							draw_line(vidBuf, x, y+3, x+1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), *width);
						}
						if (y+12<*height)
						{
							draw_line(vidBuf, x-1, y+6, x-3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), *width);
							draw_line(vidBuf, x+1, y+6, x+3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), *width);
						}
					}
					else
						vidBuf[(fullY+y)*fullW+(fullX+x)] = PIXPACK(globalSim->elements[type].Colour);
					i+=3; //Skip Type and Descriptor
					
					//Skip temp
					if(fieldDescriptor & 0x01)
					{
						i+=2;
					}
					else
					{
						i++;
					}
					
					//Skip life
					if(fieldDescriptor & 0x02)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x04)
						{
							if(i++ >= partsDataLen) goto fail;
						}
					}
					
					//Skip tmp
					if(fieldDescriptor & 0x08)
					{
						if(i++ >= partsDataLen) goto fail;
						if(fieldDescriptor & 0x10)
						{
							if(i++ >= partsDataLen) goto fail;
							if(fieldDescriptor & 0x1000)
							{
								if(i+1 >= partsDataLen) goto fail;
								i += 2;
							}
						}
					}
					
					//Skip ctype
					if(fieldDescriptor & 0x20)
					{
						if(i >= partsDataLen) goto fail;
						ctype = partsData[i++];
						if(fieldDescriptor & 0x200)
						{
							if(i+2 >= partsDataLen) goto fail;
							ctype |= (((unsigned)partsData[i++]) << 24);
							ctype |= (((unsigned)partsData[i++]) << 16);
							ctype |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					//Read dcolour
					if(fieldDescriptor & 0x40)
					{
						unsigned char r, g, b, a;
						if(i+3 >= partsDataLen) goto fail;
						a = partsData[i++];
						r = partsData[i++];
						g = partsData[i++];
						b = partsData[i++];
						r = ((a*r + (255-a)*COLR(globalSim->elements[type].Colour))>>8);
						g = ((a*g + (255-a)*COLG(globalSim->elements[type].Colour))>>8);
						b = ((a*b + (255-a)*COLB(globalSim->elements[type].Colour))>>8);
						vidBuf[(fullY+y)*fullW+(fullX+x)] = PIXRGB(r, g, b);
					}
					
					//Skip vx
					if(fieldDescriptor & 0x80)
					{
						if(i++ >= partsDataLen) goto fail;
					}
					
					//Skip vy
					if(fieldDescriptor & 0x100)
					{
						if(i++ >= partsDataLen) goto fail;
					}

					//Skip tmp2
					if(fieldDescriptor & 0x400)
					{
						if(i++ >= partsDataLen) goto fail;
						if (fieldDescriptor & 0x800)
							if(i++ >= partsDataLen) goto fail;
					}

					//Skip pavg (moving solids)
					if (fieldDescriptor & 0x2000)
					{
						i += 4;
						if (i > partsDataLen) goto fail;
					}

					if (modsave)
					{
						//Skip flags (instantly activated powered elements in my mod)
						if(fieldDescriptor & 0x4000)
						{
							if(i++ >= partsDataLen) goto fail;
						}
					}
				}
			}
		}
	}
	goto fin;
fail:
	if(vidBuf)
	{
		free(vidBuf);
		vidBuf = NULL;
	}
fin:
	//Don't call bson_destroy if bson_init wasn't called, or an uninitialized pointer (b.data) will be freed and the game will crash
	if (bsonInitialised)
		bson_destroy(&b);
	return vidBuf;
}

void *build_save(int *size, int orig_x0, int orig_y0, int orig_w, int orig_h, unsigned char bmap[YRES/CELL][XRES/CELL], float vx[YRES/CELL][XRES/CELL], float vy[YRES/CELL][XRES/CELL], float pv[YRES/CELL][XRES/CELL], float fvx[YRES/CELL][XRES/CELL], float fvy[YRES/CELL][XRES/CELL], sign signs[MAXSIGNS], void* o_partsptr, bool tab)
{
	particle *partsptr = (particle*)o_partsptr;
	unsigned char *partsData = NULL, *partsPosData = NULL, *fanData = NULL, *wallData = NULL, *pressData = NULL, *vxData = NULL, *vyData = NULL, *finalData = NULL, *outputData = NULL, *soapLinkData = NULL, *movsData = NULL, *animData = NULL;
	unsigned *partsPosLink = NULL, *partsPosFirstMap = NULL, *partsPosCount = NULL, *partsPosLastMap = NULL;
	unsigned partsCount = 0, *partsSaveIndex = NULL;
	unsigned *elementCount = (unsigned*)calloc(PT_NUM, sizeof(unsigned));
	int partsDataLen, partsPosDataLen, fanDataLen = 0, wallDataLen, pressDataLen = 0, vxDataLen = 0, vyDataLen = 0, finalDataLen, outputDataLen, soapLinkDataLen, movsDataLen = 0, animDataLen = 0;
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int x, y, i, wallDataFound = 0;
	int posCount, signsCount;
	bson b;

	//Get coords in blocks
	blockX = orig_x0/CELL;
	blockY = orig_y0/CELL;

	//Snap full coords to block size
	fullX = blockX*CELL;
	fullY = blockY*CELL;

	//Original size + offset of original corner from snapped corner, rounded up by adding CELL-1
	blockW = (orig_w+orig_x0-fullX+CELL-1)/CELL;
	blockH = (orig_h+orig_y0-fullY+CELL-1)/CELL;
	fullW = blockW*CELL;
	fullH = blockH*CELL;
	
	//malloc data for walls and fans, and pressure and velocity if needed
	wallData = (unsigned char*)malloc(blockW*blockH);
	wallDataLen = blockW*blockH;
	fanData = (unsigned char*)malloc((blockW*blockH)*2);
	if (!wallData || !fanData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	pressData = (unsigned char*)malloc((blockW*blockH)*2);
	vxData = (unsigned char*)malloc((blockW*blockH)*2);
	vyData = (unsigned char*)malloc((blockW*blockH)*2);
	if (!pressData || !vxData || !vyData)
	{
	puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	//Copy wall and fan data
	for(x = blockX; x < blockX+blockW; x++)
	{
		for(y = blockY; y < blockY+blockH; y++)
		{
			wallData[(y-blockY)*blockW+(x-blockX)] = bmap[y][x];
			
			//save pressure and x/y velocity grids (mod only)
			float pres = std::max(-255.0f,std::min(255.0f,pv[y][x]))+256.0f;
			float velX = std::max(-255.0f,std::min(255.0f,vx[y][x]))+256.0f;
			float velY = std::max(-255.0f,std::min(255.0f,vy[y][x]))+256.0f;
			pressData[pressDataLen++] = (unsigned char)((int)(pres*128)&0xFF);
			pressData[pressDataLen++] = (unsigned char)((int)(pres*128)>>8);

			vxData[vxDataLen++] = (unsigned char)((int)(velX*128)&0xFF);
			vxData[vxDataLen++] = (unsigned char)((int)(velX*128)>>8);

			vyData[vyDataLen++] = (unsigned char)((int)(velY*128)&0xFF);
			vyData[vyDataLen++] = (unsigned char)((int)(velY*128)>>8);

			if(bmap[y][x] && !wallDataFound)
				wallDataFound = 1;
			if(bmap[y][x]==WL_FAN)
			{
				i = (int)(fvx[y][x]*64.0f+127.5f);
				if (i<0) i=0;
				if (i>255) i=255;
				fanData[fanDataLen++] = i;
				i = (int)(fvy[y][x]*64.0f+127.5f);
				if (i<0) i=0;
				if (i>255) i=255;
				fanData[fanDataLen++] = i;
			}
		}
	}
	if(!fanDataLen)
	{
		free(fanData);
		fanData = NULL;
	}
	if(!wallDataFound)
	{
		free(wallData);
		wallData = NULL;
	}
	if(!pressDataLen)
	{
		free(pressData);
		pressData = NULL;
	}
	if(!vxDataLen)
	{
		free(vxData);
		vxData = NULL;
	}
	if(!vyDataLen)
	{
		free(vyData);
		vyData = NULL;
	}
	
	//Index positions of all particles, using linked lists
	//partsPosFirstMap is pmap for the first particle in each position
	//partsPosLastMap is pmap for the last particle in each position
	//partsPosCount is the number of particles in each position
	//partsPosLink contains, for each particle, (i<<8)|1 of the next particle in the same position
	partsPosFirstMap = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosLastMap = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosCount = (unsigned*)calloc(fullW*fullH, sizeof(unsigned));
	partsPosLink = (unsigned*)calloc(NPART, sizeof(unsigned));
	if (!partsPosFirstMap || !partsPosLastMap || !partsPosCount || !partsPosLink)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for(i = 0; i < NPART; i++)
	{
		if(partsptr[i].type)
		{
			x = (int)(partsptr[i].x+0.5f);
			y = (int)(partsptr[i].y+0.5f);
			if (x>=orig_x0 && x<orig_x0+orig_w && y>=orig_y0 && y<orig_y0+orig_h)
			{
				//Coordinates relative to top left corner of saved area
				x -= fullX;
				y -= fullY;
				if (!partsPosFirstMap[y*fullW + x])
				{
					//First entry in list
					partsPosFirstMap[y*fullW + x] = (i<<8)|1;
					partsPosLastMap[y*fullW + x] = (i<<8)|1;
				}
				else
				{
					//Add to end of list
					partsPosLink[partsPosLastMap[y*fullW + x]>>8] = (i<<8)|1;//link to current end of list
					partsPosLastMap[y*fullW + x] = (i<<8)|1;//set as new end of list
				}
				partsPosCount[y*fullW + x]++;
			}
		}
	}

	//Store number of particles in each position
	partsPosData = (unsigned char*)malloc(fullW*fullH*3);
	partsPosDataLen = 0;
	if (!partsPosData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for (y=0;y<fullH;y++)
	{
		for (x=0;x<fullW;x++)
		{
			posCount = partsPosCount[y*fullW + x];
			partsPosData[partsPosDataLen++] = (posCount&0x00FF0000)>>16;
			partsPosData[partsPosDataLen++] = (posCount&0x0000FF00)>>8;
			partsPosData[partsPosDataLen++] = (posCount&0x000000FF);
		}
	}
	
	i = pmap[4][4]>>8;
	bool solids[MAX_MOVING_SOLIDS]; //used to remember which moving solids are in this save
	memset(solids, 0, MAX_MOVING_SOLIDS*sizeof(bool));
	//Copy parts data
	/* Field descriptor format:
	|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|		0		|
	|				|				|	  pavg		|	tmp[3+4]	|		tmp2[2]	|		tmp2	|	ctype[2]	|		vy		|		vx		|	dcolour		|	ctype[1]	|		tmp[2]	|		tmp[1]	|		life[2]	|		life[1]	|	temp dbl len|
	life[2] means a second byte (for a 16 bit field) if life[1] is present
	*/
	partsData = (unsigned char*)malloc(NPART * (sizeof(particle)+1));
	partsDataLen = 0;
	partsSaveIndex = (unsigned*)calloc(NPART, sizeof(unsigned));
	partsCount = 0;
	if (!partsData || !partsSaveIndex)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}
	for (y=0;y<fullH;y++)
	{
		for (x=0;x<fullW;x++)
		{
			//Find the first particle in this position
			i = partsPosFirstMap[y*fullW + x];

			//Loop while there is a pmap entry
			while (i)
			{
				unsigned short fieldDesc = 0;
				int fieldDescLoc = 0, tempTemp, vTemp;
				
				//Turn pmap entry into a partsptr index
				i = i>>8;

				//Store saved particle index+1 for this partsptr index (0 means not saved)
				partsSaveIndex[i] = (partsCount++) + 1;

				//Type (required)
				partsData[partsDataLen++] = partsptr[i].type;
				elementCount[partsptr[i].type]++;
				
				//Location of the field descriptor
				fieldDescLoc = partsDataLen++;
				partsDataLen++;
				
				//Extra Temperature (2nd byte optional, 1st required), 1 to 2 bytes
				//Store temperature as an offset of 21C(294.15K) or go into a 16byte int and store the whole thing
				if(fabs(partsptr[i].temp-294.15f)<127)
				{
					tempTemp = (int)floor(partsptr[i].temp-294.15f+0.5f);
					partsData[partsDataLen++] = tempTemp;
				}
				else
				{
					fieldDesc |= 1;
					tempTemp = (int)(partsptr[i].temp+0.5f);
					partsData[partsDataLen++] = tempTemp;
					partsData[partsDataLen++] = tempTemp >> 8;
				}
				
				//Life (optional), 1 to 2 bytes
				if(partsptr[i].life)
				{
					int life = partsptr[i].life;
					if (life > 0xFFFF)
						life = 0xFFFF;
					else if (life < 0)
						life = 0;
					fieldDesc |= 1 << 1;
					partsData[partsDataLen++] = life;
					if (life & 0xFF00)
					{
						fieldDesc |= 1 << 2;
						partsData[partsDataLen++] = life >> 8;
					}
				}
				
				//Tmp (optional), 1, 2 or 4 bytes
				if(partsptr[i].tmp)
				{
					fieldDesc |= 1 << 3;
					partsData[partsDataLen++] = partsptr[i].tmp;
					if(partsptr[i].tmp & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 4;
						partsData[partsDataLen++] = partsptr[i].tmp >> 8;
						if(partsptr[i].tmp & 0xFFFF0000)
						{
							fieldDesc |= 1 << 12;
							partsData[partsDataLen++] = (partsptr[i].tmp&0xFF000000)>>24;
							partsData[partsDataLen++] = (partsptr[i].tmp&0x00FF0000)>>16;
						}
					}
				}
				
				//Ctype (optional), 1 or 4 bytes
				if(partsptr[i].ctype)
				{
					fieldDesc |= 1 << 5;
					partsData[partsDataLen++] = partsptr[i].ctype;
					if(partsptr[i].ctype & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 9;
						partsData[partsDataLen++] = (partsptr[i].ctype&0xFF000000)>>24;
						partsData[partsDataLen++] = (partsptr[i].ctype&0x00FF0000)>>16;
						partsData[partsDataLen++] = (partsptr[i].ctype&0x0000FF00)>>8;
					}
				}
				
				//Dcolour (optional), 4 bytes
				if(partsptr[i].dcolour && COLA(partsptr[i].dcolour))
				{
					fieldDesc |= 1 << 6;
					partsData[partsDataLen++] = COLA(partsptr[i].dcolour);
					partsData[partsDataLen++] = COLR(partsptr[i].dcolour);
					partsData[partsDataLen++] = COLG(partsptr[i].dcolour);
					partsData[partsDataLen++] = COLB(partsptr[i].dcolour);
				}
				
				//VX (optional), 1 byte
				if(fabs(partsptr[i].vx) > 0.001f)
				{
					fieldDesc |= 1 << 7;
					vTemp = (int)(partsptr[i].vx*16.0f+127.5f);
					if (vTemp<0) vTemp=0;
					if (vTemp>255) vTemp=255;
					partsData[partsDataLen++] = vTemp;
				}
				
				//VY (optional), 1 byte
				if(fabs(partsptr[i].vy) > 0.001f)
				{
					fieldDesc |= 1 << 8;
					vTemp = (int)(partsptr[i].vy*16.0f+127.5f);
					if (vTemp<0) vTemp=0;
					if (vTemp>255) vTemp=255;
					partsData[partsDataLen++] = vTemp;
				}

				//Tmp2 (optional), 1 or 2 bytes
				if (partsptr[i].tmp2 && partsptr[i].type != PT_PINV)
				{
					fieldDesc |= 1 << 10;
					partsData[partsDataLen++] = partsptr[i].tmp2;
					if(partsptr[i].tmp2 & 0xFF00)
					{
						fieldDesc |= 1 << 11;
						partsData[partsDataLen++] = partsptr[i].tmp2 >> 8;
					}
				}

				//Don't save pavg for things that break under pressure, because then they will break when the save is loaded, since pressure isn't also loaded
				if ((partsptr[i].pavg[0] || partsptr[i].pavg[1]) && !(partsptr[i].type == PT_QRTZ || partsptr[i].type == PT_GLAS || partsptr[i].type == PT_TUNG))
				{
					fieldDesc |= 1 << 13;
					partsData[partsDataLen++] = (int)partsptr[i].pavg[0];
					partsData[partsDataLen++] = ((int)partsptr[i].pavg[0])>>8;
					partsData[partsDataLen++] = (int)partsptr[i].pavg[1];
					partsData[partsDataLen++] = ((int)partsptr[i].pavg[1])>>8;

					//add to a list of moving solids confirmed in the area to be saved
					if (partsptr[i].type == PT_MOVS && partsptr[i].tmp2 >= 0 && partsptr[i].tmp2 < MAX_MOVING_SOLIDS && !solids[partsptr[i].tmp2])
						solids[partsptr[i].tmp2] = true;
				}
				
				//Write the field descriptor
				partsData[fieldDescLoc] = fieldDesc&0xFF;
				partsData[fieldDescLoc+1] = fieldDesc>>8;

				//Get the pmap entry for the next particle in the same position
				i = partsPosLink[i];
			}
		}
	}
	if (!partsDataLen)
	{
		free(partsData);
		partsData = NULL;
	}

	if (elementCount[PT_MOVS])
	{
		movsData = (unsigned char*)malloc(MAX_MOVING_SOLIDS*2);
		if (!movsData)
		{
			puts("Save Error, out of memory\n");
			outputData = NULL;
			goto fin;
		}

		for (int bn = 0; bn < MAX_MOVING_SOLIDS; bn++)
			if (solids[bn]) //list of moving solids that are in the save area, filled above
			{
				MovingSolid* movingSolid = ((MOVS_ElementDataContainer*)globalSim->elementData[PT_MOVS])->GetMovingSolid(bn);
				if (movingSolid && (movingSolid->index || movingSolid->particleCount))
				{
					movsData[movsDataLen++] = bn;
					movsData[movsDataLen++] = (int)((movingSolid->rotationOld + 2*M_PI)*20);
				}
			}

		if (!movsDataLen)
		{
			free(movsData);
			movsData = NULL;
		}
	}

	if (elementCount[PT_ANIM])
	{
		animData = (unsigned char*)malloc((globalSim->maxFrames*4+1)*elementCount[PT_ANIM]);
		if (!animData)
		{
			puts("Save Error, out of memory\n");
			outputData = NULL;
			goto fin;
		}

		//Iterate through particles in the same order that they were saved
		for (y=0;y<fullH;y++)
		{
			for (x=0;x<fullW;x++)
			{
				//Find the first particle in this position
				i = partsPosFirstMap[y*fullW + x];

				//Loop while there is a pmap entry
				while (i)
				{
					//Turn pmap entry into a partsptr index
					i = i>>8;

					if (partsptr[i].type == PT_ANIM && partsptr[i].animations)
					{
						int animLength = std::min(partsptr[i].ctype, globalSim->maxFrames-1); //make sure we don't try to read past what is allocated
						animData[animDataLen++] = animLength; //first byte stores data length, rest is length*4 bytes
						for (int j = 0; j <= animLength; j++)
						{
							animData[animDataLen++] = COLA(partsptr[i].animations[j]);
							animData[animDataLen++] = COLR(partsptr[i].animations[j]);
							animData[animDataLen++] = COLG(partsptr[i].animations[j]);
							animData[animDataLen++] = COLB(partsptr[i].animations[j]);
						}
					}

					//Get the pmap entry for the next particle in the same position
					i = partsPosLink[i];
				}
			}
		}
		if(!animDataLen)
		{
			free(animData);
			animData = NULL;
		}
	}

	if (elementCount[PT_SOAP])
	{
		soapLinkData = (unsigned char*)malloc(3*elementCount[PT_SOAP]);
		soapLinkDataLen = 0;
		if (!soapLinkData)
		{
			puts("Save Error, out of memory\n");
			outputData = NULL;
			goto fin;
		}
		//Iterate through particles in the same order that they were saved
		for (y=0;y<fullH;y++)
		{
			for (x=0;x<fullW;x++)
			{
				//Find the first particle in this position
				i = partsPosFirstMap[y*fullW + x];

				//Loop while there is a pmap entry
				while (i)
				{
					//Turn pmap entry into a partsptr index
					i = i>>8;

					if (partsptr[i].type==PT_SOAP)
					{
						//Only save forward link for each particle, back links can be deduced from other forward links
						//linkedIndex is index within saved particles + 1, 0 means not saved or no link
						unsigned linkedIndex = 0;
						if ((partsptr[i].ctype&2) && partsptr[i].tmp>=0 && partsptr[i].tmp<NPART)
						{
							linkedIndex = partsSaveIndex[partsptr[i].tmp];
						}
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0xFF0000)>>16;
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0x00FF00)>>8;
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0x0000FF);
					}

					//Get the pmap entry for the next particle in the same position
					i = partsPosLink[i];
				}
			}
		}
		if(!soapLinkDataLen)
		{
			free(soapLinkData);
			soapLinkData = NULL;
		}
	}
	
	bson_init(&b);
	bson_append_start_object(&b, "origin");
	bson_append_int(&b, "majorVersion", SAVE_VERSION);
	bson_append_int(&b, "minorVersion", MINOR_VERSION);
	bson_append_int(&b, "buildNum", BUILD_NUM);
	bson_append_int(&b, "snapshotId", 0);
#ifdef BETA
	bson_append_string(&b, "releaseType", "B");
#else
	bson_append_string(&b, "releaseType", "S");
#endif
#ifdef WIN
#ifdef _64BIT
	bson_append_string(&b, "platform", "WIN64");
#else
	bson_append_string(&b, "platform", "WIN32");
#endif
#elif defined MACOSX
	bson_append_string(&b, "platform", "MACOSX");
#elif defined LIN
#ifdef _64BIT
	bson_append_string(&b, "platform", "LIN64");
#else
	bson_append_string(&b, "platform", "LIN32");
#endif
#else
	bson_append_string(&b, "platform", "UNKNOWN");
#endif
#if defined X86_SSE3
	bson_append_string(&b, "builtType", "SSE3");
#elif defined X86_SSE2
		bson_append_string(&b, "builtType", "SSE2");
#elif defined X86_SSE
	bson_append_string(&b, "builtType", "SSE");
#else
	bson_append_string(&b, "builtType", "NO");
#endif
	bson_append_finish_object(&b);

	bson_append_bool(&b, "waterEEnabled", water_equal_test);
	bson_append_bool(&b, "legacyEnable", legacy_enable);
	bson_append_bool(&b, "gravityEnable", ngrav_enable);
	bson_append_bool(&b, "paused", sys_pause);
	bson_append_int(&b, "gravityMode", gravityMode);
	bson_append_int(&b, "airMode", airMode);
	bson_append_bool(&b, "msrotation", globalSim->msRotation);
	bson_append_bool(&b, "decorations_enaable", decorations_enable);
	bson_append_bool(&b, "hud_enable", hud_enable);
	bson_append_bool(&b, "aheat_enable", aheat_enable);
	bson_append_int(&b, "render_mode", render_mode);
	bson_append_int(&b, "display_mode", display_mode);
	bson_append_int(&b, "colour_mode", colour_mode);
	bson_append_int(&b, "Jacob1's_Mod", MOD_SAVE_VERSION);
	bson_append_int(&b, "edgeMode", edgeMode);
	bson_append_int(&b, "compatible_with", 8); //unused?
	
	bson_append_string(&b, "leftSelectedElementIdentifier", activeTools[0]->GetIdentifier().c_str());
	bson_append_string(&b, "rightSelectedElementIdentifier", activeTools[1]->GetIdentifier().c_str());
	bson_append_int(&b, "activeMenu", active_menu);
	if (partsData)
	{
		bson_append_binary(&b, "parts", (char)BSON_BIN_USER, (const char*)partsData, partsDataLen);

		bson_append_start_array(&b, "palette");
		for (int i = 0; i < PT_NUM; i++)
			bson_append_int(&b, globalSim->elements[i].Identifier.c_str(), i);

		bson_append_finish_array(&b);
	}
	if (partsPosData)
		bson_append_binary(&b, "partsPos", (char)BSON_BIN_USER, (const char*)partsPosData, partsPosDataLen);
	if (wallData)
		bson_append_binary(&b, "wallMap", (char)BSON_BIN_USER, (const char*)wallData, wallDataLen);
	if (fanData)
		bson_append_binary(&b, "fanMap", (char)BSON_BIN_USER, (const char*)fanData, fanDataLen);
	if (pressData)
		bson_append_binary(&b, "pressMap", (char)BSON_BIN_USER, (const char*)pressData, pressDataLen);
	if (vxData)
		bson_append_binary(&b, "vxMap", (char)BSON_BIN_USER, (const char*)vxData, vxDataLen);
	if (vyData)
		bson_append_binary(&b, "vyMap", (char)BSON_BIN_USER, (const char*)vyData, vyDataLen);
	if (soapLinkData)
		bson_append_binary(&b, "soapLinks", (char)BSON_BIN_USER, (const char*)soapLinkData, soapLinkDataLen);
	if (movsData)
		bson_append_binary(&b, "movs", (char)BSON_BIN_USER, (const char*)movsData, movsDataLen);
	if (animData)
		bson_append_binary(&b, "anim", (char)BSON_BIN_USER, (const char*)animData, animDataLen);
#ifdef LUACONSOLE
	if (LuaCode && LuaCodeLen)
	{
		bson_append_binary(&b, "LuaCode", (char)BSON_BIN_USER, (const char*)LuaCode, LuaCodeLen);
	}
#endif
	signsCount = 0;
	for(i = 0; i < MAXSIGNS; i++)
	{
		if(signs[i].text[0] && signs[i].x>=orig_x0 && signs[i].x<=orig_x0+orig_w && signs[i].y>=orig_y0 && signs[i].y<=orig_y0+orig_h)
		{
			signsCount++;
		}
	}
	if(signsCount)
	{
		bson_append_start_array(&b, "signs");
		for(i = 0; i < MAXSIGNS; i++)
		{
			if(signs[i].text[0] && signs[i].x>=orig_x0 && signs[i].x<=orig_x0+orig_w && signs[i].y>=orig_y0 && signs[i].y<=orig_y0+orig_h)
			{
				bson_append_start_object(&b, "sign");
				bson_append_string(&b, "text", signs[i].text);
				bson_append_int(&b, "justification", signs[i].ju);
				bson_append_int(&b, "x", signs[i].x-fullX);
				bson_append_int(&b, "y", signs[i].y-fullY);
				bson_append_finish_object(&b);
			}
		}
		bson_append_finish_array(&b);
	}
	if (tab)
	{
		bson_append_start_object(&b, "saveInfo");
		bson_append_int(&b, "saveOpened", svf_open);
		bson_append_int(&b, "fileOpened", svf_fileopen);
		bson_append_string(&b, "saveName", svf_name);
		bson_append_string(&b, "fileName", svf_filename);
		bson_append_int(&b, "published", svf_publish);
		bson_append_string(&b, "ID", svf_id);
		bson_append_string(&b, "description", svf_description);
		bson_append_string(&b, "author", svf_author);
		bson_append_string_n(&b, "tags", svf_tags, 255);
		bson_append_int(&b, "myVote", svf_myvote);
		bson_append_finish_object(&b);
	}
	bson_finish(&b);
	//bson_print(&b);
	
	finalData = (unsigned char*)bson_data(&b);
	finalDataLen = bson_size(&b);
	outputDataLen = finalDataLen*2+12;
	outputData = (unsigned char*)malloc(outputDataLen);
	if (!outputData)
	{
		puts("Save Error, out of memory\n");
		outputData = NULL;
		goto fin;
	}

	outputData[0] = 'O';
	outputData[1] = 'P';
	outputData[2] = 'S';
	outputData[3] = '1';
	outputData[4] = SAVE_VERSION;
	outputData[5] = CELL;
	outputData[6] = blockW;
	outputData[7] = blockH;
	outputData[8] = finalDataLen;
	outputData[9] = finalDataLen >> 8;
	outputData[10] = finalDataLen >> 16;
	outputData[11] = finalDataLen >> 24;
	
	if (BZ2_bzBuffToBuffCompress((char*)outputData+12, (unsigned*)(&outputDataLen), (char*)finalData, bson_size(&b), 9, 0, 0) != BZ_OK)
	{
		puts("Save Error\n");
		free(outputData);
		*size = 0;
		outputData = NULL;
		goto fin;
	}
	
	//printf("compressed data: %d\n", outputDataLen);
	*size = outputDataLen + 12;
	
fin:
	bson_destroy(&b);
	if(partsData)
		free(partsData);
	if(wallData)
		free(wallData);
	if(fanData)
		free(fanData);
	if (pressData)
		free(pressData);
	if (vxData)
		free(vxData);
	if (vyData)
		free(vyData);
	if (elementCount)
		free(elementCount);
	if (partsSaveIndex)
		free(partsSaveIndex);
	if (soapLinkData)
		free(soapLinkData);
	if (partsPosData)
		free(partsPosData);
	if (partsPosLink)
		free(partsPosLink);
	if (partsPosCount)
		free(partsPosCount);
	if (partsPosFirstMap)
		free(partsPosFirstMap);
	if (partsPosLastMap)
		free(partsPosLastMap);
	
	return outputData;
}

int parse_save_OPS(void *save, int size, int replace, int x0, int y0, unsigned char bmap[YRES/CELL][XRES/CELL], float vx[YRES/CELL][XRES/CELL], float vy[YRES/CELL][XRES/CELL], float pv[YRES/CELL][XRES/CELL], float fvx[YRES/CELL][XRES/CELL], float fvy[YRES/CELL][XRES/CELL], sign signs[MAXSIGNS], void* o_partsptr, unsigned pmap[YRES][XRES])
{
	particle *partsptr = (particle*)o_partsptr;
	unsigned char * inputData = (unsigned char*)save, *bsonData = NULL, *partsData = NULL, *partsPosData = NULL, *fanData = NULL, *wallData = NULL, *pressData = NULL, *vxData = NULL, *vyData = NULL, *soapLinkData = NULL, *movsData = NULL, *animData = NULL;
	int inputDataLen = size, bsonDataLen = 0, partsDataLen, partsPosDataLen, fanDataLen, wallDataLen, pressDataLen, vxDataLen, vyDataLen, soapLinkDataLen, movsDataLen, animDataLen;
	unsigned partsCount = 0, *partsSimIndex = NULL;
	int i, freeIndicesCount, x, y, returnCode = 0, j, modsave = 0;
	int *freeIndices = NULL;
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int saved_version = inputData[4];
	int elementPalette[PT_NUM];
	bool hasPallete = false;
	bson b;
	bson_iterator iter;

	memset(elementPalette, -1, sizeof(elementPalette));
	//Block sizes
	blockX = x0/CELL;
	blockY = y0/CELL;
	blockW = inputData[6];
	blockH = inputData[7];
	
	//Full size, normalized
	fullX = blockX*CELL;
	fullY = blockY*CELL;
	fullW = blockW*CELL;
	fullH = blockH*CELL;
	
	//From newer version
	if (saved_version > SAVE_VERSION && saved_version != 87 && saved_version != 222)
	{
		info_ui(vid_buf,"Save is from a newer version","Attempting to load it anyway, this may cause a crash");
	}
		
	//Incompatible cell size
	if(inputData[5] > CELL)
	{
		fprintf(stderr, "Cell size mismatch\n");
		return 1;
	}
		
	//Too large/off screen
	if(blockX+blockW > XRES/CELL || blockY+blockH > YRES/CELL)
	{
		fprintf(stderr, "Save too large\n");
		return 1;
	}
	
	bsonDataLen = ((unsigned)inputData[8]);
	bsonDataLen |= ((unsigned)inputData[9]) << 8;
	bsonDataLen |= ((unsigned)inputData[10]) << 16;
	bsonDataLen |= ((unsigned)inputData[11]) << 24;
	
	//Check for overflows, don't load saves larger than 200MB
	unsigned int toAlloc = bsonDataLen+1;
	if (toAlloc > 209715200 || !toAlloc)
	{
		fprintf(stderr, "Save data too large, refusing\n");
		return 3;
	}

	bsonData = (unsigned char*)malloc(bsonDataLen+1);
	if(!bsonData)
	{
		fprintf(stderr, "Internal error while parsing save: could not allocate buffer\n");
		return 3;
	}
	//Make sure bsonData is null terminated, since all string functions need null terminated strings
	//(bson_iterator_key returns a pointer into bsonData, which is then used with strcmp)
	bsonData[bsonDataLen] = 0;
	
	if (BZ2_bzBuffToBuffDecompress((char*)bsonData, (unsigned*)(&bsonDataLen), (char*)inputData+12, inputDataLen-12, 0, 0))
	{
		fprintf(stderr, "Unable to decompress\n");
		return 1;
	}
	
	if(replace)
	{
		//Remove everything
		clear_sim();
		erase_bframe();
		globalSim->instantActivation = false;
	}
	
	bson_init_data(&b, (char*)bsonData);
	bson_iterator_init(&iter, &b);
	while(bson_iterator_next(&iter))
	{
		if (!strcmp(bson_iterator_key(&iter), "signs"))
		{
			if(bson_iterator_type(&iter)==BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while(bson_iterator_next(&subiter))
				{
					if (!strcmp(bson_iterator_key(&subiter), "sign"))
					{
						if(bson_iterator_type(&subiter)==BSON_OBJECT)
						{
							bson_iterator signiter;
							bson_iterator_subiterator(&subiter, &signiter);
							//Find a free sign ID
							for (i = 0; i < MAXSIGNS; i++)
								if (!signs[i].text[0])
									break;
							//Stop reading signs if we have no free spaces
							if(i >= MAXSIGNS)
								break;
							while(bson_iterator_next(&signiter))
							{
								if (!strcmp(bson_iterator_key(&signiter), "text") && bson_iterator_type(&signiter)==BSON_STRING)
								{
									strncpy(signs[i].text, bson_iterator_string(&signiter), 255);
									//clean_text(signs[i].text, -1);  //disabling this since I love colours :D
								}
								else if (!strcmp(bson_iterator_key(&signiter), "justification") && bson_iterator_type(&signiter)==BSON_INT)
								{
									signs[i].ju = bson_iterator_int(&signiter);
								}
								else if (!strcmp(bson_iterator_key(&signiter), "x") && bson_iterator_type(&signiter)==BSON_INT)
								{
									signs[i].x = bson_iterator_int(&signiter)+fullX;
								}
								else if (!strcmp(bson_iterator_key(&signiter), "y") && bson_iterator_type(&signiter)==BSON_INT)
								{
									signs[i].y = bson_iterator_int(&signiter)+fullY;
								}
								else
								{
									fprintf(stderr, "Unknown sign property %s\n", bson_iterator_key(&signiter));
								}
							}
						}
						else
						{
							fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&subiter));
						}
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "parts"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		if (!strcmp(bson_iterator_key(&iter), "partsPos"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (partsPosDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				partsPosData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of particle position data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "wallMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (wallDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				wallData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of wall data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "pressMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (pressDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				pressData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of pressure data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "vxMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (vxDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				vxData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of vx data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "vyMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (vyDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				vyData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of vy data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "fanMap"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (fanDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				fanData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of fan data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "soapLinks"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (soapLinkDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				soapLinkData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of soap data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "movs"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (movsDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				movsData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of movs data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "anim"))
		{
			if(bson_iterator_type(&iter)==BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER && (animDataLen = bson_iterator_bin_len(&iter)) > 0)
			{
				animData = (unsigned char*)bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of anim data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
#ifdef LUACONSOLE
		else if (!strcmp(bson_iterator_key(&iter), "LuaCode"))
		{
			if (bson_iterator_type(&iter) == BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter)) == BSON_BIN_USER && (LuaCodeLen = bson_iterator_bin_len(&iter)) > 0)
			{
				//this reads directly into the variables from luaconsole.h
				if (LuaCode)
					free(LuaCode);
				LuaCode = mystrdup(bson_iterator_bin_data(&iter));
				ranLuaCode = false;
			}
			else
			{
				fprintf(stderr, "Invalid datatype of anim data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter) == BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter)) == BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
#endif
		else if (!strcmp(bson_iterator_key(&iter), "palette"))
		{
			if(bson_iterator_type(&iter)==BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while(bson_iterator_next(&subiter))
				{
					if(bson_iterator_type(&subiter)==BSON_INT)
					{
						std::string identifier = std::string(bson_iterator_key(&subiter));
						int ID = -1, oldID = bson_iterator_int(&subiter);
						for (int i = 0; i < PT_NUM; i++)
							if (!identifier.compare(globalSim->elements[i].Identifier))
							{
								ID = i;
								break;
							}

						if (oldID >= 0 && oldID < PT_NUM)
							elementPalette[oldID] = ID;
					}
				}
				hasPallete = true;
			}
			else
			{
				fprintf(stderr, "Wrong type for element palette: %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_ARRAY);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "legacyEnable") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				legacy_enable = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "gravityEnable") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				int tempGrav = ngrav_enable;
				tempGrav = ((int)bson_iterator_bool(&iter))?1:0;
#ifndef RENDERER
				//Change the gravity state
				if(ngrav_enable != tempGrav)
				{
					if(tempGrav)
						start_grav_async();
					else
						stop_grav_async();
				}
#endif
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "waterEEnabled") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				water_equal_test = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "paused") && (!sys_pause || replace == 2))
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				sys_pause = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "gravityMode") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				gravityMode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "airMode") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				airMode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if((!strcmp(bson_iterator_key(&iter), "leftSelectedElementIdentifier") || !strcmp(bson_iterator_key(&iter), "rightSelectedElementIdentifier")) && replace == 2)
		{
			if (bson_iterator_type(&iter) == BSON_STRING)
			{
				if (bson_iterator_key(&iter)[0] == 'l')
				{
					Tool *temp = GetToolFromIdentifier(bson_iterator_string(&iter));
					if (temp)
						activeTools[0] = temp;
				}
				else
				{
					Tool *temp = GetToolFromIdentifier(bson_iterator_string(&iter));
					if (temp)
						activeTools[1] = temp;
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "activeMenu") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_INT && bson_iterator_int(&iter) >= 0 && ((bson_iterator_int(&iter) < SC_TOTAL && menuSections[bson_iterator_int(&iter)]->enabled) || replace == 2))
			{
				active_menu = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong value for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "msrotation") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				globalSim->msRotation = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "decorations_enable") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				decorations_enable = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "hud_enable") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				hud_enable = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "aheat_enable") && replace)
		{
			if(bson_iterator_type(&iter)==BSON_BOOL)
			{
				aheat_enable = ((int)bson_iterator_bool(&iter))?1:0;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "render_mode") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				render_mode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "display_mode") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				display_mode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "colour_mode") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				colour_mode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "Jacob1's_Mod"))
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				if (replace == 1)
				{
					modsave = bson_iterator_int(&iter);
#ifdef LUACONSOLE
					//TODO: don't use lua logging
					if (!strcmp(svf_user,"jacob1") && log_history[19] == NULL)
					{
						char* modver = (char*)calloc(32, sizeof(char));
						sprintf(modver, "Made in jacob1's mod version %d", modsave);
						luacon_log(modver);
					}
#endif
					globalSim->instantActivation = true;
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "edgeMode") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_INT)
			{
				edgeMode = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "saveInfo") && replace == 2)
		{
			if(bson_iterator_type(&iter)==BSON_OBJECT)
			{
				bson_iterator saveInfoiter;
				bson_iterator_subiterator(&iter, &saveInfoiter);
				while(bson_iterator_next(&saveInfoiter))
				{
					if(!strcmp(bson_iterator_key(&saveInfoiter), "saveOpened") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						svf_open = bson_iterator_int(&saveInfoiter);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "fileOpened") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						svf_fileopen = bson_iterator_int(&saveInfoiter);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "saveName") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_name, bson_iterator_string(&saveInfoiter), 63);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "fileName") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_filename, bson_iterator_string(&saveInfoiter), 254);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "published") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						svf_publish = bson_iterator_int(&saveInfoiter);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "ID") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_id, bson_iterator_string(&saveInfoiter), 15);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "description") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_description, bson_iterator_string(&saveInfoiter), 254);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "author") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_author, bson_iterator_string(&saveInfoiter), 63);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "tags") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						strncpy(svf_tags, bson_iterator_string(&saveInfoiter), 255);
					else if(!strcmp(bson_iterator_key(&saveInfoiter), "myVote") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						svf_myvote = bson_iterator_int(&saveInfoiter);
					else
						fprintf(stderr, "Unknown save info property %s\n", bson_iterator_key(&saveInfoiter));
				}
				svf_own = svf_login && !strcmp(svf_author, svf_user);
				svf_publish = svf_publish && svf_login && !strcmp(svf_author, svf_user);
				if (svf_last)
					free(svf_last);
				svf_last = save;
				svf_lsize = size;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
	}
	
	//Read wall and fan data
	if (wallData)
	{
		j = 0;
		if (blockW * blockH > wallDataLen)
		{
			fprintf(stderr, "Not enough wall data\n");
			goto fail;
		}
		for (x = 0; x < blockW; x++)
		{
			for (y = 0; y < blockH; y++)
			{
				if (wallData[y*blockW+x])
				{
					int wt = change_wallpp(wallData[y*blockW+x]);
					if (wt < 0 || wt >= WALLCOUNT)
						continue;
					bmap[blockY+y][blockX+x] = wt;
				}
				if (wallData[y*blockW+x] == WL_FAN && fanData)
				{
					if(j+1 >= fanDataLen)
					{
						fprintf(stderr, "Not enough fan data\n");
					}
					fvx[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
					fvy[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
				}
			}
		}
		gravity_mask();
	}
	
	//Read pressure data
	if (pressData)
	{
		j = 0;
		if(blockW * blockH > pressDataLen)
		{
			fprintf(stderr, "Not enough pressure data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				int i2;
				i = pressData[j++];
				i2 = pressData[j++];
				pv[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	//Read vx data
	if (vxData)
	{
		j = 0;
		if(blockW * blockH > vxDataLen)
		{
			fprintf(stderr, "Not enough vx data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				int i2;
				i = vxData[j++];
				i2 = vxData[j++];
				vx[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	//Read vy data
	if (vyData)
	{
		j = 0;
		if(blockW * blockH > vyDataLen)
		{
			fprintf(stderr, "Not enough vy data\n");
			goto fail;
		}
		for(x = 0; x < blockW; x++)
		{
			for(y = 0; y < blockH; y++)
			{
				int i2;
				i = vyData[j++];
				i2 = vyData[j++];
				vy[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	//Read particle data
	if (partsData && partsPosData)
	{
		int newIndex = 0, fieldDescriptor, tempTemp;
		int posCount, posTotal, partsPosDataIndex = 0;
		int saved_x, saved_y;
		int freeIndicesIndex = 0;
		if(fullW * fullH * 3 > partsPosDataLen)
		{
			fprintf(stderr, "Not enough particle position data\n");
			goto fail;
		}
		globalSim->parts_lastActiveIndex = NPART-1;
		freeIndicesCount = 0;
		freeIndices = (int*)calloc(sizeof(int), NPART);
		partsSimIndex = (unsigned*)calloc(NPART, sizeof(unsigned));
		partsCount = 0;
		for (int i = 0; i < NPART; i++)
		{
			// keep a track of indices we can use
			if (!partsptr[i].type)
				freeIndices[freeIndicesCount++] = i;
		}
		i = 0;
		for (saved_y=0; saved_y<fullH; saved_y++)
		{
			for (saved_x=0; saved_x<fullW; saved_x++)
			{
				//Read total number of particles at this position
				posTotal = 0;
				posTotal |= partsPosData[partsPosDataIndex++]<<16;
				posTotal |= partsPosData[partsPosDataIndex++]<<8;
				posTotal |= partsPosData[partsPosDataIndex++];
				//Put the next posTotal particles at this position
				for (posCount=0; posCount<posTotal; posCount++)
				{
					//i+3 because we have 4 bytes of required fields (type (1), descriptor (2), temp (1))
					if (i+3 >= partsDataLen)
						goto fail;
					x = saved_x + fullX;
					y = saved_y + fullY;
					fieldDescriptor = partsData[i+1];
					fieldDescriptor |= partsData[i+2] << 8;
					if (x >= XRES || x < 0 || y >= YRES || y < 0)
					{
						fprintf(stderr, "Out of range [%d]: %d %d, [%d, %d], [%d, %d]\n", i, x, y, (unsigned)partsData[i+1], (unsigned)partsData[i+2], (unsigned)partsData[i+3], (unsigned)partsData[i+4]);
						goto fail;
					}
					if (partsData[i] >= PT_NUM)
						partsData[i] = PT_DMND;	//Replace all invalid elements with diamond
					if (pmap[y][x] && posCount==0) // Check posCount to make sure an existing particle is not replaced twice if two particles are saved in that position
					{
						//Replace existing particle or allocated block
						newIndex = pmap[y][x]>>8;
						globalSim->elementCount[parts[newIndex].type]--;
						pmap[y][x] = 0;
					}
					else if(photons[y][x] && posCount==0)
					{
						//Replace existing particle or allocated block
						newIndex = photons[y][x]>>8;
						globalSim->elementCount[parts[newIndex].type]--;
						photons[y][x] = 0;
					}
					else if(freeIndicesIndex<freeIndicesCount)
					{
						//Create new particle
						newIndex = freeIndices[freeIndicesIndex++];
					}
					else
					{
						//Nowhere to put new particle, tpt is sad :(
						break;
					}
					if(newIndex < 0 || newIndex >= NPART)
						goto fail;

					//Store partsptr index+1 for this saved particle index (0 means not loaded)
					partsSimIndex[partsCount++] = newIndex+1;

					//Clear the particle, ready for our new properties
					memset(&(partsptr[newIndex]), 0, sizeof(particle));
					
					//Required fields
					partsptr[newIndex].type = partsData[i];
					partsptr[newIndex].x = (float)x;
					partsptr[newIndex].y = (float)y;
					i+=3;
					
					partsptr[newIndex].type = fix_type(partsptr[newIndex].type, saved_version, modsave, hasPallete ? elementPalette : NULL);

					//Read temp
					if(fieldDescriptor & 0x01)
					{
						//Full 16bit int
						tempTemp = partsData[i++];
						tempTemp |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].temp = (float)tempTemp;
					}
					else
					{
						//1 Byte room temp offset
						tempTemp = (signed char)partsData[i++];
						partsptr[newIndex].temp = tempTemp+294.15f;
					}
					
					//Read life
					if(fieldDescriptor & 0x02)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].life = partsData[i++];
						//Read 2nd byte
						if(fieldDescriptor & 0x04)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].life |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					//Read tmp
					if(fieldDescriptor & 0x08)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].tmp = partsData[i++];
						//Read 2nd byte
						if(fieldDescriptor & 0x10)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 8);
							//Read 3rd and 4th bytes
							if(fieldDescriptor & 0x1000)
							{
								if(i+1 >= partsDataLen) goto fail;
								partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 24);
								partsptr[newIndex].tmp |= (((unsigned)partsData[i++]) << 16);
							}
						}
						if (partsptr[newIndex].type == PT_PIPE || partsptr[newIndex].type == PT_PPIP)
							partsptr[newIndex].tmp = fix_type(partsptr[newIndex].tmp&0xFF, saved_version, modsave, hasPallete ? elementPalette : NULL)|(parts[newIndex].tmp&~0xFF);
					}
					
					//Read ctype
					if(fieldDescriptor & 0x20)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].ctype = partsData[i++];
						//Read additional bytes
						if(fieldDescriptor & 0x200)
						{
							if(i+2 >= partsDataLen) goto fail;
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 24);
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 16);
							partsptr[newIndex].ctype |= (((unsigned)partsData[i++]) << 8);
						}
						if (partsptr[newIndex].type == PT_CLNE || partsptr[newIndex].type == PT_PCLN || partsptr[newIndex].type == PT_BCLN || partsptr[newIndex].type == PT_PBCN || partsptr[newIndex].type == PT_STOR || partsptr[newIndex].type == PT_CONV || ((partsptr[newIndex].type == PT_STKM || partsptr[newIndex].type == PT_STKM2 || partsptr[newIndex].type == PT_FIGH) && partsptr[newIndex].ctype != SPC_AIR) || partsptr[newIndex].type == PT_LAVA || partsptr[newIndex].type == PT_SPRK || partsptr[newIndex].type == PT_PSTN || partsptr[newIndex].type == PT_CRAY || partsptr[newIndex].type == PT_DTEC || partsptr[newIndex].type == PT_DRAY)
							partsptr[newIndex].ctype = fix_type(partsptr[newIndex].ctype, saved_version, modsave, hasPallete ? elementPalette : NULL);
					}
					
					//Read dcolour
					if(fieldDescriptor & 0x40)
					{
						if(i+3 >= partsDataLen) goto fail;
						unsigned char alpha = partsData[i++];
						unsigned char red = partsData[i++];
						unsigned char green = partsData[i++];
						unsigned char blue = partsData[i++];
						partsptr[newIndex].dcolour = COLARGB(alpha, red, green, blue);
					}
					
					//Read vx
					if(fieldDescriptor & 0x80)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].vx = (partsData[i++]-127.0f)/16.0f;
					}
					
					//Read vy
					if(fieldDescriptor & 0x100)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].vy = (partsData[i++]-127.0f)/16.0f;
					}

					//Read tmp2
					if(fieldDescriptor & 0x400)
					{
						if(i >= partsDataLen) goto fail;
						partsptr[newIndex].tmp2 = partsData[i++];
						//Read 2nd byte
						if (fieldDescriptor & 0x800)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].tmp2 |= (((unsigned)partsData[i++]) << 8);
						}
						if (partsptr[newIndex].type == PT_VIRS || partsptr[newIndex].type == PT_VRSS || partsptr[newIndex].type == PT_VRSG)
							partsptr[newIndex].tmp2 = fix_type(partsptr[newIndex].tmp2, saved_version, modsave, hasPallete ? elementPalette : NULL);
					}

					//Read pavg (for moving solids)
					if (fieldDescriptor & 0x2000)
					{
						if(i+3 >= partsDataLen) goto fail;
						int pavg = partsData[i++];
						pavg |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].pavg[0] = (float)pavg;
						pavg = partsData[i++];
						pavg |= (((unsigned)partsData[i++]) << 8);
						partsptr[newIndex].pavg[1] = (float)pavg;
					}

#ifdef OGLR
					partsptr[newIndex].lastX = partsptr[newIndex].x - partsptr[newIndex].vx;
					partsptr[newIndex].lastY = partsptr[newIndex].y - partsptr[newIndex].vy;
#endif

					if (modsave && modsave <= 20)
					{
						//Read flags (for instantly activated powered elements in my mod)
						//now removed so that the partsData save format is exactly the same as tpt and won't cause errors
						if(fieldDescriptor & 0x4000)
						{
							if(i >= partsDataLen) goto fail;
							partsptr[newIndex].flags = partsData[i++];
						}
					}

					// no more particle properties to load, so we can change type here without messing up loading
					if ((globalSim->elementCount[PT_STKM]>0 && partsptr[newIndex].type==PT_STKM) || (globalSim->elementCount[PT_STKM2]>0 && partsptr[newIndex].type==PT_STKM2))
					{
						partsptr[newIndex].type = PT_NONE;
					}
					else if (partsptr[newIndex].type == PT_STKM)
					{
						STKM_init_legs(&player, newIndex);
						player.elem = PT_DUST;
						if (parts[newIndex].ctype == OLD_SPC_AIR)
							parts[newIndex].ctype = SPC_AIR;
					}
					else if (partsptr[newIndex].type == PT_STKM2)
					{
						STKM_init_legs(&player2, newIndex);
						player2.elem = PT_DUST;
						if (parts[newIndex].ctype == OLD_SPC_AIR)
							parts[newIndex].ctype = SPC_AIR;
					}
					else if (partsptr[newIndex].type == PT_FIGH)
					{
						partsptr[newIndex].tmp = ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->Alloc();
						if (partsptr[newIndex].tmp >= 0)
						{
							playerst* figh = ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->Get((unsigned char)partsptr[newIndex].tmp);
							figh->spwn = 1;
							figh->elem = PT_DUST;
							STKM_init_legs(figh, newIndex);
							if (parts[newIndex].ctype == OLD_SPC_AIR)
								parts[newIndex].ctype = SPC_AIR;
						}
						else
							partsptr[newIndex].type = PT_NONE;
					}
					else if (partsptr[newIndex].type == PT_SPAWN)
					{
						if (globalSim->elementCount[PT_SPAWN])
							partsptr[newIndex].type = PT_NONE;
						else
							player.spawnID = newIndex;
					}
					else if (partsptr[newIndex].type == PT_SPAWN2)
					{
						if (globalSim->elementCount[PT_SPAWN2])
							partsptr[newIndex].type = PT_NONE;
						else
							player2.spawnID = newIndex;
					}
					if (partsptr[newIndex].type == PT_SOAP)
						partsptr[newIndex].ctype &= ~6; // delete all soap connections, but it looks like if tmp & tmp2 were saved to 3 bytes, connections would load properly
					if (!ptypes[partsptr[newIndex].type].enabled && !secret_els)
						partsptr[newIndex].type = PT_NONE;

					if (saved_version<81)
					{
						if (partsptr[newIndex].type==PT_BOMB && partsptr[newIndex].tmp!=0)
						{
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].ctype = 0;
							if (partsptr[newIndex].tmp==1)
								partsptr[newIndex].tmp = 0;
						}
						if (partsptr[newIndex].type==PT_DUST && partsptr[newIndex].life>0)
						{
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].ctype = (partsptr[newIndex].tmp2<<16) | (partsptr[newIndex].tmp<<8) | partsptr[newIndex].ctype;
							partsptr[newIndex].tmp = 1;
						}
						if (partsptr[newIndex].type==PT_FIRW && partsptr[newIndex].tmp>=2)
						{
							int caddress = (int)restrict_flt(restrict_flt((float)(partsptr[newIndex].tmp-4), 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
							partsptr[newIndex].type = PT_EMBR;
							partsptr[newIndex].tmp = 1;
							partsptr[newIndex].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
						}
					}
					if (saved_version < 87 && partsptr[newIndex].type == PT_PSTN && partsptr[newIndex].ctype)
						partsptr[newIndex].life = 1;
					if (saved_version < 89)
					{
						if (partsptr[newIndex].type == PT_FILT)
						{
							if (partsptr[newIndex].tmp<0 || partsptr[newIndex].tmp>3)
								partsptr[newIndex].tmp = 6;
							partsptr[newIndex].ctype = 0;
						}
						else if (partsptr[newIndex].type == PT_QRTZ || partsptr[newIndex].type == PT_PQRT)
						{
							partsptr[newIndex].tmp2 = partsptr[newIndex].tmp;
							partsptr[newIndex].tmp = partsptr[newIndex].ctype;
							partsptr[newIndex].ctype = 0;
						}
					}
					if (saved_version < 90)
					{
						if (partsptr[newIndex].type == PT_PHOT)
							partsptr[newIndex].flags |= FLAG_PHOTDECO;
					}
					if (saved_version < 91)
					{
						if (partsptr[newIndex].type == PT_VINE)
							partsptr[newIndex].tmp = 1;
						else if (partsptr[newIndex].type == PT_PSTN)
							partsptr[newIndex].temp = 283.15;
						else if (partsptr[newIndex].type == PT_DLAY)
							partsptr[newIndex].temp = partsptr[newIndex].temp - 1.0f;
					}
					//note: PSv was used in version 77.0 and every version before, add something in PSv too if the element is that old

					globalSim->elementCount[partsptr[newIndex].type]++;
				}
			}
		}
		if (movsData)
		{
			int movsDataPos = 0, numBalls = ((MOVS_ElementDataContainer*)globalSim->elementData[PT_MOVS])->GetNumBalls();
			int solids[MAX_MOVING_SOLIDS]; //solids is a map of the old .tmp2 it was saved with, to the new ball number it is getting
			memset(solids, MAX_MOVING_SOLIDS, sizeof(solids)); //default to invalid ball
			for (int i = 0; i < movsDataLen/2; i++)
			{
				int bn = movsData[movsDataPos++];
				if (bn >= 0 && bn < MAX_MOVING_SOLIDS)
				{
					solids[bn] = numBalls;
					MovingSolid *movingSolid = ((MOVS_ElementDataContainer*)globalSim->elementData[PT_MOVS])->GetMovingSolid(numBalls++);
					if (movingSolid) //create a moving solid and clear all it's variables
					{
						movingSolid->Simulation_Cleared();
						movingSolid->rotationOld = movingSolid->rotation = movsData[movsDataPos++]/20.0f - 2*M_PI; //set its rotation
					}
				}
				else
					movsDataPos++;
			}
			((MOVS_ElementDataContainer*)globalSim->elementData[PT_MOVS])->SetNumBalls(numBalls); //new number of known moving solids
			for (unsigned int i = 0; i < partsCount; i++)
			{
				if (partsSimIndex[i] && partsptr[partsSimIndex[i]-1].type == PT_MOVS)
				{
					int newIndex = partsSimIndex[i]-1;
					if (!(partsptr[newIndex].flags&FLAG_DISAPPEAR) && partsptr[newIndex].tmp2 >= 0 && partsptr[newIndex].tmp2 < MAX_MOVING_SOLIDS)
					{
						partsptr[newIndex].tmp2 = solids[partsptr[newIndex].tmp2];
						MovingSolid *movingSolid = ((MOVS_ElementDataContainer*)globalSim->elementData[PT_MOVS])->GetMovingSolid(partsptr[newIndex].tmp2);
						if (movingSolid)
						{
							movingSolid->particleCount++; //increase ball particle count
							//set center "controlling" particle
							if (partsptr[newIndex].pavg[0] == 0 && partsptr[newIndex].pavg[1] == 0)
								movingSolid->index = newIndex+1;
						}
					}
					else
						partsptr[newIndex].tmp2 = MAX_MOVING_SOLIDS; //default to invalid ball

					if (partsptr[newIndex].pavg[0] > 32768)
						partsptr[newIndex].pavg[0] -= 65536;
					if (partsptr[newIndex].pavg[1] > 32768)
						partsptr[newIndex].pavg[1] -= 65536;
				}
			}
		}
		if (animData)
		{
			int animDataPos = 0;
			for (unsigned i = 0; i < partsCount; i++)
			{
				if (partsSimIndex[i] && partsptr[partsSimIndex[i]-1].type == PT_ANIM)
				{
					if (animDataPos >= animDataLen) break;

					newIndex = partsSimIndex[i]-1;
					int origanimLen = animData[animDataPos++];
					int animLen = std::min(origanimLen, globalSim->maxFrames-1); //read animation length, make sure it doesn't go past the current frame limit
					partsptr[newIndex].ctype = animLen;
					partsptr[newIndex].animations = (ARGBColour*)calloc(globalSim->maxFrames, sizeof(ARGBColour));
					if (animDataPos+4*animLen > animDataLen || partsptr[newIndex].animations == NULL)
						goto fail;

					for (int j = 0; j < globalSim->maxFrames; j++)
					{
						if (j <= animLen) //read animation data
						{
							unsigned char alpha = animData[animDataPos++];
							unsigned char red = animData[animDataPos++];
							unsigned char green = animData[animDataPos++];
							unsigned char blue = animData[animDataPos++];
							partsptr[newIndex].animations[j] = COLARGB(alpha, red, green, blue);
						}
						else //set the rest to 0
							partsptr[newIndex].animations[j] = 0;
					}
					//ignore any extra data in case user set maxFrames to something small
					if (origanimLen+1 > globalSim->maxFrames)
						animDataPos += 4*(origanimLen+1-globalSim->maxFrames);
				}
			}
		}
		if (soapLinkData)
		{
			int soapLinkDataPos = 0;
			for (unsigned int i = 0; i < partsCount; i++)
			{
				if (partsSimIndex[i] && partsptr[partsSimIndex[i]-1].type == PT_SOAP)
				{
					// Get the index of the particle forward linked from this one, if present in the save data
					unsigned int linkedIndex = 0;
					if (soapLinkDataPos+3 > soapLinkDataLen) break;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<16;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<8;
					linkedIndex |= soapLinkData[soapLinkDataPos++];
					// All indexes in soapLinkData and partsSimIndex have 1 added to them (0 means not saved/loaded)
					if (!linkedIndex || linkedIndex-1>=partsCount || !partsSimIndex[linkedIndex-1])
						continue;
					linkedIndex = partsSimIndex[linkedIndex-1]-1;
					newIndex = partsSimIndex[i]-1;

					//Attach the two particles
					partsptr[newIndex].ctype |= 2;
					partsptr[newIndex].tmp = linkedIndex;
					partsptr[linkedIndex].ctype |= 4;
					partsptr[linkedIndex].tmp2 = newIndex;
				}
			}
		}
	}
	goto fin;
fail:
	//Clean up everything
	returnCode = 1;
fin:
	bson_destroy(&b);
	if(freeIndices)
		free(freeIndices);
	if(partsSimIndex)
		free(partsSimIndex);
	return returnCode;
}

//Old saving
pixel *prerender_save_PSv(void *save, int size, int *width, int *height)
{
	unsigned char *d,*c=(unsigned char*)save,*m=NULL;
	int i,j,k,x,y,rx,ry,p=0, pc, gc;
	int bw,bh,w,h,new_format = 0;
	pixel *fb;

	if (size<16)
		return NULL;
	if (!(c[2]==0x43 && c[1]==0x75 && c[0]==0x66) && !(c[2]==0x76 && c[1]==0x53 && c[0]==0x50))
		return NULL;
	if (c[2]==0x43 && c[1]==0x75 && c[0]==0x66) {
		new_format = 1;
	}
	//if (c[4]>SAVE_VERSION)
	//	return NULL;

	bw = c[6];
	bh = c[7];
	w = bw*CELL;
	h = bh*CELL;

	if (c[5]!=CELL)
		return NULL;

	i = (unsigned)c[8];
	i |= ((unsigned)c[9])<<8;
	i |= ((unsigned)c[10])<<16;
	i |= ((unsigned)c[11])<<24;
	d = (unsigned char*)malloc(i);
	if (!d)
		return NULL;
	fb = (pixel*)calloc(w*h, PIXELSIZE);
	if (!fb)
	{
		free(d);
		return NULL;
	}
	m = (unsigned char*)calloc(w*h, sizeof(int));
	if (!m)
	{
		free(d);
		return NULL;
	}

	if (BZ2_bzBuffToBuffDecompress((char *)d, (unsigned *)&i, (char *)(c+12), size-12, 0, 0))
		goto corrupt;
	size = i;

	if (size < bw*bh)
		goto corrupt;

	k = 0;
	for (y=0; y<bh; y++)
		for (x=0; x<bw; x++)
		{
			int wt = change_wall(d[p++]);
			if (c[4] >= 44)
				wt = change_wallpp(wt);
			if (wt < 0 || wt >= WALLCOUNT)
				continue;
			rx = x*CELL;
			ry = y*CELL;
			pc = PIXPACK(wallTypes[wt].colour);
			gc = PIXPACK(wallTypes[wt].eglow);
			if (wallTypes[wt].drawstyle==1)
			{
				for (i=0; i<CELL; i+=2)
					for (j=(i>>1)&1; j<CELL; j+=2)
						fb[(i+ry)*w+(j+rx)] = pc;
			}
			else if (wallTypes[wt].drawstyle==2)
			{
				for (i=0; i<CELL; i+=2)
					for (j=0; j<CELL; j+=2)
						fb[(i+ry)*w+(j+rx)] = pc;
			}
			else if (wallTypes[wt].drawstyle==3)
			{
				for (i=0; i<CELL; i++)
					for (j=0; j<CELL; j++)
						fb[(i+ry)*w+(j+rx)] = pc;
			}
			else if (wallTypes[wt].drawstyle==4)
			{
				for (i=0; i<CELL; i++)
					for (j=0; j<CELL; j++)
						if(i == j)
							fb[(i+ry)*w+(j+rx)] = pc;
						else if  (j == i+1 || (j == 0 && i == CELL-1))
							fb[(i+ry)*w+(j+rx)] = gc;
						else 
							fb[(i+ry)*w+(j+rx)] = PIXPACK(0x202020);
			}

			// special rendering for some walls
			if (wt==WL_EWALL)
			{
				for (i=0; i<CELL; i++)
					for (j=0; j<CELL; j++)
						if (!(i&j&1))
							fb[(i+ry)*w+(j+rx)] = pc;
			}
			else if (wt==WL_WALLELEC)
			{
				for (i=0; i<CELL; i++)
					for (j=0; j<CELL; j++)
					{
						if (!((y*CELL+j)%2) && !((x*CELL+i)%2))
							fb[(i+ry)*w+(j+rx)] = pc;
						else
							fb[(i+ry)*w+(j+rx)] = PIXPACK(0x808080);
					}
			}
			else if (wt==WL_EHOLE)
			{
				for (i=0; i<CELL; i+=2)
					for (j=0; j<CELL; j+=2)
						fb[(i+ry)*w+(j+rx)] = PIXPACK(0x242424);
			}
			else if (wt==WL_FAN)
				k++;
		}
	p += 2*k;
	if (p>=size)
		goto corrupt;

	for (y=0; y<h; y++)
		for (x=0; x<w; x++)
		{
			if (p >= size)
				goto corrupt;
			j=d[p++];
			if (j<PT_NUM && j>0)
			{
				if (j==PT_STKM || j==PT_STKM2 || j==PT_FIGH)
				{
					pixel lc, hc=PIXRGB(255, 224, 178);
					if (j==PT_STKM || j==PT_FIGH) lc = PIXRGB(255, 255, 255);
					else lc = PIXRGB(100, 100, 255);
					//only need to check upper bound of y coord - lower bounds and x<w are checked in draw_line
					if (j==PT_STKM || j==PT_STKM2)
					{
						draw_line(fb , x-2, y-2, x+2, y-2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						if (y+2<h)
						{
							draw_line(fb , x-2, y+2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
							draw_line(fb , x-2, y-2, x-2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
							draw_line(fb , x+2, y-2, x+2, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						}
					}
					else if (y+2<h)
					{
						draw_line(fb, x-2, y, x, y-2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x-2, y, x, y+2, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x, y-2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), w);
						draw_line(fb, x, y+2, x+2, y, PIXR(hc), PIXG(hc), PIXB(hc), w);
					}
					if (y+6<h)
					{
						draw_line(fb , x, y+3, x-1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), w);
						draw_line(fb , x, y+3, x+1, y+6, PIXR(lc), PIXG(lc), PIXB(lc), w);
					}
					if (y+12<h)
					{
						draw_line(fb , x-1, y+6, x-3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), w);
						draw_line(fb , x+1, y+6, x+3, y+12, PIXR(lc), PIXG(lc), PIXB(lc), w);
					}
				}
				else
					fb[y*w+x] = PIXPACK(globalSim->elements[j].Colour);
				m[(x-0)+(y-0)*w] = j;
			}
		}
	for (j=0; j<w*h; j++)
	{
		if (m[j])
			p += 2;
	}
	for (j=0; j<w*h; j++)
	{
		if (m[j])
		{
			if (c[4]>=44)
				p+=2;
			else
				p++;
		}
	}
	if (c[4]>=44) {
		for (j=0; j<w*h; j++)
		{
			if (m[j])
			{
				p+=2;
			}
		}
	}
	if (c[4]>=53) {
		if (m[j]==PT_PBCN || (m[j]==PT_TRON && c[4] > 77))
			p++;
	}
	if (c[4]>=49) {
		for (j=0; j<w*h; j++)
		{
			if (m[j])
			{
				if (p >= size) {
					goto corrupt;
				}
				if (d[p++])
					fb[j] = PIXRGB(0,0,0);
			}
		}
		//Read RED component
		for (j=0; j<w*h; j++)
		{
			if (m[j])
			{
				if (p >= size) {
					goto corrupt;
				}
				if (m[j] <= NPART) {
					fb[j] |= PIXRGB(d[p++],0,0);
				} else {
					p++;
				}
			}
		}
		//Read GREEN component
		for (j=0; j<w*h; j++)
		{
			if (m[j])
			{
				if (p >= size) {
					goto corrupt;
				}
				if (m[j] <= NPART) {
					fb[j] |= PIXRGB(0,d[p++],0);
				} else {
					p++;
				}
			}
		}
		//Read BLUE component
		for (j=0; j<w*h; j++)
		{
			if (m[j])
			{
				if (p >= size) {
					goto corrupt;
				}
				if (m[j] <= NPART) {
					fb[j] |= PIXRGB(0,0,d[p++]);
				} else {
					p++;
				}
			}
		}
	}

	free(d);
	free(m);
	*width = w;
	*height = h;
	return fb;

corrupt:
	free(d);
	free(fb);
	free(m);
	return NULL;
}

int parse_save_PSv(void *save, int size, int replace, int x0, int y0, unsigned char bmap[YRES/CELL][XRES/CELL], float fvx[YRES/CELL][XRES/CELL], float fvy[YRES/CELL][XRES/CELL], sign signs[MAXSIGNS], void* partsptr, unsigned pmap[YRES][XRES])
{
	unsigned char *d=NULL,*c=(unsigned char*)save;
	int q,i,j,k,x,y,p=0,*m=NULL, ver, pty, ty, legacy_beta=0, tempGrav = 0, modver = 0;
	int bx0=x0/CELL, by0=y0/CELL, bw, bh, w, h;
	int nf=0, new_format = 0, ttv = 0;
	particle *parts = (particle*)partsptr;
	int *fp = (int*)malloc(NPART*sizeof(int));

	//New file header uses PSv, replacing fuC. This is to detect if the client uses a new save format for temperatures
	//This creates a problem for old clients, that display and "corrupt" error instead of a "newer version" error

	if (size<16)
		return 1;
	if (!(c[2]==0x43 && c[1]==0x75 && c[0]==0x66) && !(c[2]==0x76 && c[1]==0x53 && c[0]==0x50))
		return 1;
	if (c[2]==0x76 && c[1]==0x53 && c[0]==0x50) {
		new_format = 1;
	}
	ver = c[4];
	if ((ver>SAVE_VERSION && ver < 200) || (ver < 237 && ver > 200+MOD_SAVE_VERSION))
		info_ui(vid_buf,"Save is from a newer version","Attempting to load it anyway, this may cause a crash");
	if (ver == 240) {
		ver = 65;
		modver = 3;
	}
	else if (ver == 242) {
		ver = 66;
		modver = 5;
	}
	else if (ver == 243) {
		ver = 68;
		modver = 6;
	}
	else if (ver == 244) {
		ver = 69;
		modver = 7;
	}
	else if (ver >= 200) {
		ver = 71;
		modver = 8;
	}

	bw = c[6];
	bh = c[7];
	if (bx0+bw > XRES/CELL)
		bx0 = XRES/CELL - bw;
	if (by0+bh > YRES/CELL)
		by0 = YRES/CELL - bh;
	if (bx0 < 0)
		bx0 = 0;
	if (by0 < 0)
		by0 = 0;

	if (c[5]!=CELL || bx0+bw>XRES/CELL || by0+bh>YRES/CELL)
		return 3;
	i = (unsigned)c[8];
	i |= ((unsigned)c[9])<<8;
	i |= ((unsigned)c[10])<<16;
	i |= ((unsigned)c[11])<<24;
	if (i > 209715200 || !i)
		return 1;

	d = (unsigned char*)malloc(i);
	if (!d)
		return 1;

	if (BZ2_bzBuffToBuffDecompress((char *)d, (unsigned *)&i, (char *)(c+12), size-12, 0, 0))
		return 1;
	size = i;

	if (size < bw*bh)
		return 1;

	// normalize coordinates
	x0 = bx0*CELL;
	y0 = by0*CELL;
	w  = bw *CELL;
	h  = bh *CELL;

	if (replace)
	{
		if (ver<46) {
			gravityMode = 0;
			airMode = 0;
		}
		clear_sim();
		erase_bframe();
	}
	globalSim->parts_lastActiveIndex = NPART-1;
	m = (int*)calloc(XRES*YRES, sizeof(int));

	if (modver)
	{
		info_ui(vid_buf, "Unsupported save", "Most support for mod saves in the old PSv format was removed, use version 29.6 or below to convert it if it doesn't load.");
		globalSim->instantActivation = true;
	}
	else
		globalSim->instantActivation = false;

	if (ver<34)
	{
		legacy_enable = 1;
	}
	else
	{
		if (ver >= 44) {
			legacy_enable = c[3] & 0x01;
			if (!sys_pause) {
				sys_pause = (c[3] >> 1) & 0x01;
			}
			if (ver >= 46 && replace) {
				gravityMode = ((c[3] >> 2) & 0x03);// | ((c[3]>>2)&0x01);
				airMode = ((c[3] >> 4) & 0x07);// | ((c[3]>>4)&0x02) | ((c[3]>>4)&0x01);
			}
			if (ver >= 49 && replace) {
				tempGrav = ((c[3] >> 7) & 0x01);
			}
		}
		else {
			if (c[3] == 1 || c[3] == 0) {
				legacy_enable = c[3];
			}
			else {
				legacy_beta = 1;
			}
		}
	}

	// make a catalog of free parts
	//memset(pmap, 0, sizeof(pmap)); "Using sizeof for array given as function argument returns the size of pointer."
	memset(pmap, 0, sizeof(unsigned)*(XRES*YRES));
	for (i=0; i<NPART; i++)
		if (parts[i].type)
		{
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			pmap[y][x] = (i<<8)|1;
		}
		else
			fp[nf++] = i;

	// load the required air state
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
		{
			if (d[p])
			{
				//In old saves, ignore walls created by sign tool bug
				//Not ignoring other invalid walls or invalid walls in new saves, so that any other bugs causing them are easier to notice, find and fix
				if (ver>=44 && ver<71 && d[p]==OLD_WL_SIGN)
				{
					p++;
					continue;
				}

				//TODO: if wall id's are changed look at https://github.com/simtr/The-Powder-Toy/commit/02a4c17d72def847205c8c89dacabe9ecdcb0dab
				//for now, old saves shouldn't have id's this large
				if (ver < 44 && bmap[y][x] >= 122)
					bmap[y][x] = 0;
				bmap[y][x] = change_wall(d[p]);
				if (ver >= 44)
					bmap[y][x] = change_wallpp(bmap[y][x]);
				if (bmap[y][x] < 0 || bmap[y][x] >= WALLCOUNT)
					bmap[y][x] = 0;
			}

			p++;
		}
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
			if (d[(y-by0)*bw+(x-bx0)]==4||(ver>=44 && d[(y-by0)*bw+(x-bx0)]==O_WL_FAN))
			{
				if (p >= size)
					goto corrupt;
				fvx[y][x] = (d[p++]-127.0f)/64.0f;
			}
	for (y=by0; y<by0+bh; y++)
		for (x=bx0; x<bx0+bw; x++)
			if (d[(y-by0)*bw+(x-bx0)]==4||(ver>=44 && d[(y-by0)*bw+(x-bx0)]==O_WL_FAN))
			{
				if (p >= size)
					goto corrupt;
				fvy[y][x] = (d[p++]-127.0f)/64.0f;
			}

	// load the particle map
	i = 0;
	pty = p;
	for (y=y0; y<y0+h; y++)
		for (x=x0; x<x0+w; x++)
		{
			if (p >= size)
				goto corrupt;
			j=d[p++];
			if (j >= PT_NUM) {
				//TODO: Possibly some server side translation
				j = PT_DUST;//goto corrupt;
			}
			if (j)
			{
				if (modver > 0 && modver <= 5)
				{
					if (j >= 136 && j <= 140)
						j += (PT_NORMAL_NUM - 136);
					else if (j >= 142 && j <= 146)
						j += (PT_NORMAL_NUM - 137);
					d[p-1] = j;
				}
				if (pmap[y][x])
				{
					k = pmap[y][x]>>8;
					globalSim->elementCount[pmap[y][x]&0xFF]--;
				}
				else if (i<nf)
				{
					k = fp[i];
					i++;
				}
				else
				{
					m[(x-x0)+(y-y0)*w] = NPART+1;
					continue;
				}
				memset(parts+k, 0, sizeof(particle));
				parts[k].type = j;
				if (j == PT_COAL)
					parts[k].tmp = 50;
				if (j == PT_FUSE)
					parts[k].tmp = 50;
				if (j == PT_PHOT)
					parts[k].ctype = 0x3fffffff;
				if (j == PT_SOAP)
					parts[k].ctype = 0;
				if (j==PT_BIZR || j==PT_BIZRG || j==PT_BIZRS)
					parts[k].ctype = 0x47FFFF;
				parts[k].x = (float)x;
				parts[k].y = (float)y;
				m[(x-x0)+(y-y0)*w] = k+1;
			}
		}

	// load particle properties
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			i--;
			if (p+1 >= size)
				goto corrupt;
			if (i < NPART)
			{
				parts[i].vx = (d[p++]-127.0f)/16.0f;
				parts[i].vy = (d[p++]-127.0f)/16.0f;
#ifdef OGLR
				parts[i].lastX = parts[i].x - parts[i].vx;
				parts[i].lastY = parts[i].y - parts[i].vy;
#endif
			}
			else
				p += 2;
		}
	}
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=44) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					ttv = (d[p++])<<8;
					ttv |= (d[p++]);
					parts[i-1].life = ttv;
				} else {
					p+=2;
				}
			} else {
				if (p >= size)
					goto corrupt;
				if (i <= NPART)
					parts[i-1].life = d[p++]*4;
				else
					p++;
			}
		}
	}
	if (ver>=44) {
		for (j=0; j<w*h; j++)
		{
			i = m[j];
			if (i)
			{
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					ttv = (d[p++])<<8;
					ttv |= (d[p++]);
					parts[i-1].tmp = ttv;
					if (ver<53 && !parts[i-1].tmp)
						for (q = 1; q<=NGOL; q++) {
							if (parts[i-1].type==oldgolTypes[q-1] && grule[q][9]==2)
								parts[i-1].tmp = grule[q][9]-1;
						}
					if (ver>=51 && ver<53 && parts[i-1].type==PT_PBCN)
					{
						parts[i-1].tmp2 = parts[i-1].tmp;
						parts[i-1].tmp = 0;
					}
				} else {
					p+=2;
				}
			}
		}
	}
	if (ver>=53) {
		for (j=0; j<w*h; j++)
		{
			i = m[j];
			ty = d[pty+j];
			if (i && (ty==PT_PBCN || (ty==PT_TRON && ver>=77)))
			{
				if (p >= size)
					goto corrupt;
				if (i <= NPART)
				{
					parts[i-1].tmp2 = d[p++];
				}
				else
					p++;
			}
		}
	}
	//Read ALPHA component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour = (d[p++]<<24);
				} else {
					p++;
				}
			}
		}
	}
	//Read RED component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= (d[p++]<<16);
				} else {
					p++;
				}
			}
		}
	}
	//Read GREEN component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= (d[p++]<<8);
				} else {
					p++;
				}
			}
		}
	}
	//Read BLUE component
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		if (i)
		{
			if (ver>=49) {
				if (p >= size) {
					goto corrupt;
				}
				if (i <= NPART) {
					parts[i-1].dcolour |= d[p++];
				} else {
					p++;
				}
			}
		}
	}
	for (j=0; j<w*h; j++)
	{
		i = m[j];
		ty = d[pty+j];
		if (i)
		{
			if (ver>=34&&legacy_beta==0)
			{
				if (p >= size)
				{
					goto corrupt;
				}
				if (i <= NPART)
				{
					if (ver>=42) {
						if (new_format) {
							ttv = (d[p++])<<8;
							ttv |= (d[p++]);
							if (parts[i-1].type==PT_PUMP) {
								parts[i-1].temp = ttv + 0.15f;//fix PUMP saved at 0, so that it loads at 0.
							} else {
								parts[i-1].temp = (float)ttv;
							}
						} else {
							parts[i-1].temp = (float)(d[p++]*((MAX_TEMP+(-MIN_TEMP))/255))+MIN_TEMP;
						}
					} else {
						parts[i-1].temp = ((d[p++]*((O_MAX_TEMP+(-O_MIN_TEMP))/255))+O_MIN_TEMP)+273.0f;
					}
				}
				else
				{
					p++;
					if (new_format) {
						p++;
					}
				}
			}
			else
			{
				parts[i-1].temp = ptypes[parts[i-1].type].heat;
			}
		}
	} 
	for (j=0; j<w*h; j++)
	{
		int gnum = 0;
		i = m[j];
		ty = d[pty+j];
		if (i && (ty==PT_CLNE || (ty==PT_PCLN && ver>=43) || (ty==PT_BCLN && ver>=44) || (ty==PT_SPRK && ver>=21) || (ty==PT_LAVA && ver>=34) || (ty==PT_PIPE && ver>=43) || (ty==PT_LIFE && ver>=51) || (ty==PT_PBCN && ver>=52) || (ty==PT_WIRE && ver>=55) || (ty==PT_STOR && ver>=59) || (ty==PT_CONV && ver>=60)))
		{
			if (p >= size)
				goto corrupt;
			if (i <= NPART)
				parts[i-1].ctype = d[p++];
			else
				p++;
		}
		// no more particle properties to load, so we can change type here without messing up loading
		if (i && i<=NPART)
		{
			if ((globalSim->elementCount[PT_STKM]>0 && ty==PT_STKM) || (globalSim->elementCount[PT_STKM2]>0 && ty==PT_STKM2))
			{
				parts[i-1].type = PT_NONE;
			}
			else if (parts[i-1].type == PT_STKM)
			{
				STKM_init_legs(&player, i-1);
				player.elem = PT_DUST;
				if (parts[i-1].ctype == OLD_SPC_AIR)
					parts[i-1].ctype = SPC_AIR;
			}
			else if (parts[i-1].type == PT_STKM2)
			{
				STKM_init_legs(&player2, i-1);
				player2.elem = PT_DUST;
				if (parts[i-1].ctype == OLD_SPC_AIR)
					parts[i-1].ctype = SPC_AIR;
			}
			else if (parts[i-1].type == PT_SPAWN)
			{
				if (globalSim->elementCount[PT_SPAWN])
					parts[i-1].type = PT_NONE;
				else
					player.spawnID = i-1;
			}
			else if (parts[i-1].type == PT_SPAWN2)
			{
				if (globalSim->elementCount[PT_SPAWN2])
					parts[i-1].type = PT_NONE;
				else
					player2.spawnID = i-1;
			}
			else if (parts[i-1].type == PT_FIGH)
			{
				parts[i-1].tmp = ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->Alloc();
				if (parts[i-1].tmp >= 0)
				{
					playerst* figh = ((FIGH_ElementDataContainer*)globalSim->elementData[PT_FIGH])->Get((unsigned char)parts[i-1].tmp);
					figh->spwn = 1;
					figh->elem = PT_DUST;
					STKM_init_legs(figh, i-1);
					if (parts[i-1].ctype == OLD_SPC_AIR)
						parts[i-1].ctype = SPC_AIR;
				}
				else
					parts[i-1].type = PT_NONE;
			}
			if (parts[i-1].type == PT_MOVS)
			{
				parts[i-1].pavg[0] = (float)parts[i-1].tmp;
				parts[i-1].pavg[1] = (float)parts[i-1].tmp2;
				parts[i-1].tmp2 = parts[i-1].life;
				parts[i-1].tmp = 0;
			}

			if (ver<48 && (ty==OLD_PT_WIND || (ty==PT_BRAY&&parts[i-1].life==0)))
			{
				// Replace invisible particles with something sensible and add decoration to hide it
				x = (int)(parts[i-1].x+0.5f);
				y = (int)(parts[i-1].y+0.5f);
				parts[i-1].dcolour = COLARGB(255, 0, 0, 0);
				parts[i-1].type = PT_DMND;
			}
			if(ver<51 && ((ty>=78 && ty<=89) || (ty>=134 && ty<=146 && ty!=141))){
				//Replace old GOL
				parts[i-1].type = PT_LIFE;
				for (gnum = 0; gnum<NGOL; gnum++){
					if (ty==oldgolTypes[gnum])
						parts[i-1].ctype = gnum;
				}
				ty = PT_LIFE;
			}
			if(ver<52 && (ty==PT_CLNE || ty==PT_PCLN || ty==PT_BCLN)){
				//Replace old GOL ctypes in clone
				for (gnum = 0; gnum<NGOL; gnum++){
					if (parts[i-1].ctype==oldgolTypes[gnum])
					{
						parts[i-1].ctype = PT_LIFE;
						parts[i-1].tmp = gnum;
					}
				}
			}
			if(ty==PT_LCRY){
				if(ver<67)
				{
					//New LCRY uses TMP not life
					if(parts[i-1].life>=10)
					{
						parts[i-1].life = 10;
						parts[i-1].tmp2 = 10;
						parts[i-1].tmp = 3;
					}
					else if(parts[i-1].life<=0)
					{
						parts[i-1].life = 0;
						parts[i-1].tmp2 = 0;
						parts[i-1].tmp = 0;
					}
					else if(parts[i-1].life < 10 && parts[i-1].life > 0)
					{
						parts[i-1].tmp = 1;
					}
				}
				else
				{
					parts[i-1].tmp2 = parts[i-1].life;
				}
			}
			if (!ptypes[parts[i-1].type].enabled)
				parts[i-1].type = PT_NONE;
			
			//PSv isn't used past version 77, but check version anyway ...
			if (ver<81)
			{
				if (parts[i-1].type==PT_BOMB && parts[i-1].tmp!=0)
				{
					parts[i-1].type = PT_EMBR;
					parts[i-1].ctype = 0;
					if (parts[i-1].tmp==1)
						parts[i-1].tmp = 0;
				}
				if (parts[i-1].type==PT_DUST && parts[i-1].life>0)
				{
					parts[i-1].type = PT_EMBR;
					parts[i-1].ctype = (parts[i-1].tmp2<<16) | (parts[i-1].tmp<<8) | parts[i-1].ctype;
					parts[i-1].tmp = 1;
				}
				if (parts[i-1].type==PT_FIRW && parts[i-1].tmp>=2)
				{
					int caddress = (int)restrict_flt(restrict_flt((float)(parts[i-1].tmp-4), 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
					parts[i-1].type = PT_EMBR;
					parts[i-1].tmp = 1;
					parts[i-1].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
				}
			}
			if (ver < 89)
			{
				if (parts[i-1].type == PT_FILT)
				{
					if (parts[i-1].tmp<0 || parts[i-1].tmp>3)
						parts[i-1].tmp = 6;
					parts[i-1].ctype = 0;
				}
				if (parts[i-1].type == PT_QRTZ || parts[i-1].type == PT_PQRT)
				{
					parts[i-1].tmp2 = parts[i-1].tmp;
					parts[i-1].tmp = parts[i-1].ctype;
					parts[i-1].ctype = 0;
				}
			}
			if (ver<90 && parts[i-1].type == PT_PHOT)
			{
				parts[i-1].flags |= FLAG_PHOTDECO;
			}
			if (ver<91 && parts[i-1].type == PT_VINE)
			{
				parts[i-1].tmp = 1;
			}

			globalSim->elementCount[parts[i-1].type]++;
		}
	}

	#ifndef RENDERER
	//Change the gravity state
	if(ngrav_enable != tempGrav && replace)
	{
		if(tempGrav)
			start_grav_async();
		else
			stop_grav_async();
	}
	#endif
	
	gravity_mask();

	if (p >= size)
		goto version1;
	j = d[p++];
	for (i=0; i<j; i++)
	{
		if (p+6 > size)
			goto corrupt;
		for (k=0; k<MAXSIGNS; k++)
			if (!signs[k].text[0])
				break;
		x = d[p++];
		x |= ((unsigned)d[p++])<<8;
		if (k<MAXSIGNS)
			signs[k].x = x+x0;
		x = d[p++];
		x |= ((unsigned)d[p++])<<8;
		if (k<MAXSIGNS)
			signs[k].y = x+y0;
		x = d[p++];
		if (k<MAXSIGNS)
			signs[k].ju = x;
		x = d[p++];
		if (p+x > size)
			goto corrupt;
		if (k<MAXSIGNS)
		{
			memcpy(signs[k].text, d+p, x);
			signs[k].text[x] = 0;
			clean_text(signs[k].text, 158-14); //Current max sign length //Note: did not remove the limit here since this limit was always present when PSV format was used
		}
		p += x;
	}

	if (modver >= 3)
	{
		if (p >= size)
			goto version1;
		if (!decorations_enable) {
			decorations_enable = (d[p++])&0x01;
		}
		aheat_enable = (d[p]>>1)&0x01;
		hud_enable = (d[p]>>2)&0x01;
		water_equal_test = (d[p]>>3)&0x01;
		//if (replace)
		//	cmode = (d[p]>>4)&0x0F;
	}

version1:
	if (m) free(m);
	if (d) free(d);
	if (fp) free(fp);

	return 0;

corrupt:
	if (m) free(m);
	if (d) free(d);
	if (fp) free(fp);
	if (replace)
	{
		legacy_enable = 0;
		clear_sim();
		erase_bframe();
	}
	return 1;
}

void *build_thumb(int *size, int bzip2)
{
	unsigned char *d=(unsigned char*)calloc(1,XRES*YRES), *c;
	int i,j,x,y;
	for (i=0; i<NPART; i++)
		if (parts[i].type)
		{
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			if (x>=0 && x<XRES && y>=0 && y<YRES)
				d[x+y*XRES] = parts[i].type;
		}
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
			if (bmap[y][x])
				for (j=0; j<CELL; j++)
					for (i=0; i<CELL; i++)
						d[x*CELL+i+(y*CELL+j)*XRES] = 0xFF;
	j = XRES*YRES;

	if (bzip2)
	{
		i = (j*101+99)/100 + 608;
		c = (unsigned char*)malloc(i);

		c[0] = 0x53;
		c[1] = 0x68;
		c[2] = 0x49;
		c[3] = 0x74;
		c[4] = SAVE_VERSION;
		c[5] = CELL;
		c[6] = XRES/CELL;
		c[7] = YRES/CELL;

		i -= 8;

		if (BZ2_bzBuffToBuffCompress((char *)(c+8), (unsigned *)&i, (char *)d, j, 9, 0, 0) != BZ_OK)
		{
			free(d);
			free(c);
			return NULL;
		}
		free(d);
		*size = i+8;
		return c;
	}

	*size = j;
	return d;
}

void *transform_save(void *odata, int *size, matrix2d transform, vector2d translate)
{
	void *ndata;
	unsigned char (*bmapo)[XRES/CELL] = (unsigned char(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(unsigned char));
	unsigned char (*bmapn)[XRES/CELL] = (unsigned char(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(unsigned char));
	particle *partst = (particle*)calloc(sizeof(particle), NPART);
	sign *signst = (sign*)calloc(MAXSIGNS, sizeof(sign));
	unsigned (*pmapt)[XRES] = (unsigned(*)[XRES])calloc(YRES*XRES, sizeof(unsigned));
	float (*fvxo)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*fvyo)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*fvxn)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*fvyn)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*vxo)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*vyo)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*vxn)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*vyn)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*pvo)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	float (*pvn)[XRES/CELL] = (float(*)[XRES/CELL])calloc((YRES/CELL)*(XRES/CELL), sizeof(float));
	int i, x, y, nx, ny, w, h, nw, nh;
	vector2d pos, tmp, ctl, cbr;
	vector2d vel;
	vector2d cornerso[4];
	unsigned char *odatac = (unsigned char*)odata;
	if (parse_save(odata, *size, 0, 0, 0, bmapo, vxo, vyo, pvo, fvxo, fvyo, signst, partst, pmapt))
	{
		free(bmapo);
		free(bmapn);
		free(partst);
		free(signst);
		free(pmapt);
		free(fvxo);
		free(fvyo);
		free(fvxn);
		free(fvyn);
		free(vxo);
		free(vyo);
		free(vxn);
		free(vyn);
		free(pvo);
		free(pvn);
		return odata;
	}
	w = odatac[6]*CELL;
	h = odatac[7]*CELL;
	// undo any translation caused by rotation
	cornerso[0] = v2d_new(0,0);
	cornerso[1] = v2d_new(w-1.0f,0.0f);
	cornerso[2] = v2d_new(0.0f,h-1.0f);
	cornerso[3] = v2d_new(w-1.0f,h-1.0f);
	for (i=0; i<4; i++)
	{
		tmp = m2d_multiply_v2d(transform,cornerso[i]);
		if (i==0) ctl = cbr = tmp; // top left, bottom right corner
		if (tmp.x<ctl.x) ctl.x = tmp.x;
		if (tmp.y<ctl.y) ctl.y = tmp.y;
		if (tmp.x>cbr.x) cbr.x = tmp.x;
		if (tmp.y>cbr.y) cbr.y = tmp.y;
	}
	// casting as int doesn't quite do what we want with negative numbers, so use floor()
	tmp = v2d_new(floor(ctl.x+0.5f),floor(ctl.y+0.5f));
	translate = v2d_sub(translate,tmp);
	nw = (int)(floor(cbr.x+0.5f)-floor(ctl.x+0.5f)+1);
	nh = (int)(floor(cbr.y+0.5f)-floor(ctl.y+0.5f)+1);
	if (nw>XRES) nw = XRES;
	if (nh>YRES) nh = YRES;
	// rotate and translate signs, parts, walls
	for (i=0; i<MAXSIGNS; i++)
	{
		if (!signst[i].text[0]) continue;
		pos = v2d_new((float)signst[i].x, (float)signst[i].y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
		nx = (int)floor(pos.x+0.5f);
		ny = (int)floor(pos.y+0.5f);
		if (nx<0 || nx>=nw || ny<0 || ny>=nh)
		{
			signst[i].text[0] = 0;
			continue;
		}
		signst[i].x = nx;
		signst[i].y = ny;
	}
	for (i=0; i<NPART; i++)
	{
		if (!partst[i].type) continue;
		pos = v2d_new(partst[i].x, partst[i].y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
		nx = (int)floor(pos.x+0.5f);
		ny = (int)floor(pos.y+0.5f);
		if (nx<0 || nx>=nw || ny<0 || ny>=nh)
		{
			partst[i].type = PT_NONE;
			continue;
		}
		partst[i].x = (float)nx;
		partst[i].y = (float)ny;
		vel = v2d_new(partst[i].vx, partst[i].vy);
		vel = m2d_multiply_v2d(transform, vel);
		partst[i].vx = vel.x;
		partst[i].vy = vel.y;
	}
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
		{
			pos = v2d_new(x*CELL+CELL*0.4f, y*CELL+CELL*0.4f);
			pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
			nx = (int)(pos.x/CELL);
			ny = (int)(pos.y/CELL);
			if (nx<0 || nx>=nw/CELL || ny<0 || ny>=nh/CELL)
				continue;
			if (bmapo[y][x])
			{
				bmapn[ny][nx] = bmapo[y][x];
				if (bmapo[y][x]==WL_FAN)
				{
					vel = v2d_new(fvxo[y][x], fvyo[y][x]);
					vel = m2d_multiply_v2d(transform, vel);
					fvxn[ny][nx] = vel.x;
					fvyn[ny][nx] = vel.y;
				}
			}
			vel = v2d_new(vxo[y][x], vyo[y][x]);
			vel = m2d_multiply_v2d(transform, vel);
			vxn[ny][nx] = vel.x;
			vyn[ny][nx] = vel.y;
			pvn[ny][nx] = pvo[y][x];
		}
	ndata = build_save(size,0,0,nw,nh,bmapn,vxn,vyn,pvn,fvxn,fvyn,signst,partst);
	free(bmapo);
	free(bmapn);
	free(partst);
	free(signst);
	free(pmapt);
	free(fvxo);
	free(fvyo);
	free(fvxn);
	free(fvyn);
	free(vxo);
	free(vyo);
	free(vxn);
	free(vyn);
	free(pvo);
	free(pvn);
	return ndata;
}
