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

#include <climits>
#include <stdexcept>
#include <bzlib.h>
#include "defines.h"
#include "powder.h"
#include "save_legacy.h"
#include "graphics.h"
#include "BSON.h"
#include "interface.h"

#include "simulation/GolNumbers.h"
#include "simulation/Simulation.h"
#include "simulation/ToolNumbers.h"
#include "simulation/WallNumbers.h"

//Pop
pixel *prerender_save(void *save, int size, int *width, int *height)
{
	unsigned char * saveData = (unsigned char*)save;
	if (size < 16 || !save)
	{
		return NULL;
	}
	try
	{
		if(saveData[0] == 'O' && saveData[1] == 'P' && (saveData[2] == 'S' || saveData[2] == 'J'))
		{
			return prerender_save_OPS(save, size, width, height);
		}
		else if((saveData[0]==0x66 && saveData[1]==0x75 && saveData[2]==0x43) || (saveData[0]==0x50 && saveData[1]==0x53 && saveData[2]==0x76))
		{
			return prerender_save_PSv(save, size, width, height);
		}
	}
	catch (std::runtime_error & e)
	{
		return NULL;
	}
	return NULL;
}

int fix_type(int type, int version, int modver, int (elementPalette)[PT_NUM])
{
	// invalid element, we don't care about it
	if (type < 0 || type > PT_NUM)
		return type;
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
	if (save_as > 0 && (el >= PT_NORMAL_NUM || el < 0 || globalSim->elements[el].Enabled == 0)) //Check for mod/disabled elements
		return 1;
	//if (save_as > 1 && (el == PT_BASE || el == PT_SEED))
	//	return 1;
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

	auto &possiblyCarriesType = particle::PossiblyCarriesType();
	auto &properties = particle::GetProperties();

	for (i=0; i<NPART; i++)
	{
		if ((int)(parts[i].x+.5f) > x0 && (int)(parts[i].x+.5f) < x0+w && (int)(parts[i].y+.5f) > y0 && (int)(parts[i].y+.5f) < y0+h)
		{
			if (invalid_element(save_as, parts[i].type))
			{
				if (give_warning)
				{
					std::stringstream errorText;
					errorText << "Found ";
					if (parts[i].type > 0 && parts[i].type < PT_NUM)
						errorText << globalSim->elements[parts[i].type].Name;
					else
						errorText << "invalid element # " << parts[i].type;
					errorText << " at X:" << (int)(parts[i].x+.5) << " Y:" << (int)(parts[i].y+.5) << ", cannot publish";
					error_ui(vid_buf, 0, errorText.str());
				}
				return 1;
			}
			if (globalSim->elements[parts[i].type].CarriesTypeIn)
			{
				for (auto index : possiblyCarriesType)
				{
					if (globalSim->elements[parts[i].type].CarriesTypeIn & (1U << index))
					{
						auto *prop = reinterpret_cast<const int *>(reinterpret_cast<const char *>(&parts[i]) + properties[index].Offset);
						if (invalid_element(save_as, TYP(*prop)))
						{
							if (give_warning)
							{
								std::stringstream errorText;
								errorText << "Found ";
								if (*prop > 0 && *prop < PT_NUM)
									errorText << globalSim->elements[*prop].Name;
								else
									errorText << "invalid element # " << *prop;
								errorText << " at X:" << (int)(parts[i].x+.5) << " Y:" << (int)(parts[i].y+.5) << ", cannot publish";
								error_ui(vid_buf, 0, errorText.str());
							}
							return 1;
						}
					}
				}
			}
		}
	}
	/*for (int x = 0; x < XRES/CELL; x++)
		for (int y = 0; y < YRES/CELL; y++)
		{
			if (bmap[y][x] == WL_STASIS)
			{
				if (give_warning)
				{
					char errortext[256] = "";
					sprintf(errortext, "Found stasis wall at X:%i Y:%i, cannot save", x*CELL, y*CELL);
				}
				return 1;
			}
		}*/
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
		return WL_ALLOWPOWDER;
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
		return WL_ALLOWPOWDER;
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
	int i, x, y, j, wt, pc, gc, modsave = 0, saved_version = inputData[4];
	int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	int bsonInitialised = 0;
	int elementPalette[PT_NUM];
	bool hasPalette = false;
	pixel * vidBuf = NULL;
	bson b;
	bson_iterator iter;

	for (int i = 0; i < PT_NUM; i++)
		elementPalette[i] = i;
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
		fprintf(stderr, "Cell size mismatch: expected %i but got %i\n", CELL, inputData[5]);
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
		free(bsonData);
		goto fail;
	}

	bson_init_data_size(&b, (char*)bsonData, bsonDataLen);
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
			if (bson_iterator_type(&iter) == BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (bson_iterator_type(&subiter) == BSON_INT)
					{
						std::string identifier = std::string(bson_iterator_key(&subiter));
						int ID = 0, oldID = bson_iterator_int(&subiter);
						if (oldID <= 0 || oldID >= PT_NUM)
							continue;
						for (int i = 0; i < PT_NUM; i++)
							if (!identifier.compare(globalSim->elements[i].Identifier))
							{
								ID = i;
								break;
							}

						if (ID != 0 || identifier.find("DEFAULT_PT_") != 0)
							elementPalette[oldID] = ID;
					}
				}
				hasPalette = true;
			}
			else
			{
				fprintf(stderr, "Wrong type for element palette: %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_ARRAY);
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
		else if (!strcmp(bson_iterator_key(&iter), "minimumVersion"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				int major = INT_MAX, minor = INT_MAX;
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (bson_iterator_type(&subiter) == BSON_INT)
					{
						if (!strcmp(bson_iterator_key(&subiter), "major"))
							major = bson_iterator_int(&subiter);
						else if (!strcmp(bson_iterator_key(&subiter), "minor"))
							minor = bson_iterator_int(&subiter);
						else
							fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
					}
				}
				if (major > FAKE_SAVE_VERSION || (major == FAKE_SAVE_VERSION && minor > FAKE_MINOR_VER))
				{
					if (!loadIncompatibleSaves)
						goto fail;
				}
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
					if (wt == WL_EWALL || wt == WL_STASIS)
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
					int type = 0, ctype = 0, tmp = 0, tmp2 = 0, dcolor = 0;
					x = saved_x + fullX;
					y = saved_y + fullY;
					unsigned int fieldDescriptor = (unsigned int)(partsData[i+1]);
					fieldDescriptor |= (unsigned int)(partsData[i+2]) << 8;
					if(x >= XRES || x < 0 || y >= YRES || y < 0)
					{
						fprintf(stderr, "Out of range [%d]: %d %d, [%d, %d], [%d, %d]\n", i, x, y, (unsigned)partsData[i+1], (unsigned)partsData[i+2], (unsigned)partsData[i+3], (unsigned)partsData[i+4]);
						goto fail;
					}
					int tempType = partsData[i];
					i+=3; //Skip Type and Descriptor
					if (fieldDescriptor & 0x4000)
						tempType |= (((unsigned)partsData[++i]) << 8);

					type = fix_type(tempType,saved_version, modsave, hasPalette ? elementPalette : NULL);
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

					//Skip temp
					if(fieldDescriptor & 0x01)
					{
						i+=2;
					}
					else
					{
						i++;
					}

					// fieldDesc3
					if (fieldDescriptor & 0x8000)
					{
						if (i >= partsDataLen)
							goto fail;
						fieldDescriptor |= (unsigned int)(partsData[i++]) << 16;
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
						if (i >= partsDataLen) goto fail;
						tmp = partsData[i++];
						if (fieldDescriptor & 0x10)
						{
							if (i >= partsDataLen) goto fail;
							tmp |= (((unsigned)partsData[i++]) << 8);
							if (fieldDescriptor & 0x1000)
							{
								if (i+1 >= partsDataLen) goto fail;
								tmp |= (((unsigned)partsData[i++]) << 24);
								tmp |= (((unsigned)partsData[i++]) << 16);
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
						if (type != PT_LIFE)
						{
							r = ((a*r + (255-a)*COLR(globalSim->elements[type].Colour))>>8);
							g = ((a*g + (255-a)*COLG(globalSim->elements[type].Colour))>>8);
							b = ((a*b + (255-a)*COLB(globalSim->elements[type].Colour))>>8);
						}
						dcolor = PIXRGB(r, g, b);
						vidBuf[(fullY+y)*fullW+(fullX+x)] = dcolor;
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
						if (i >= partsDataLen) goto fail;
						tmp2 = partsData[i++];
						if (fieldDescriptor & 0x800)
						{
							if (i >= partsDataLen) goto fail;
							tmp2 |= (((unsigned)partsData[i++]) << 8);
						}
					}

					//Skip tmp3/tmp4
					if (fieldDescriptor & 0x2000)
					{
						i += 4;
						if (fieldDescriptor & 0x10000)
							i += 4;
						if (i > partsDataLen) goto fail;
					}

					if (modsave && modsave <= 20)
					{
						//Skip flags (instantly activated powered elements in my mod)
						if(fieldDescriptor & 0x4000)
						{
							if(i++ >= partsDataLen) goto fail;
						}
					}

					if (type == PT_LIFE)
					{
						auto color1 = dcolor;
						auto color2 = tmp;
						if (!color1)
						{
							color1 = PIXPACK(0xFFFFFF);
						}
						auto ruleset = ctype;
						if (ruleset >= 0 && ruleset < NGOL)
						{
							if (!decorations_enable)
							{
								color1 = builtinGol[ruleset].color;
								color2 = builtinGol[ruleset].color2;
							}
							ruleset = builtinGol[ruleset].ruleset;
						}

						auto states = ((ruleset >> 17) & 0xF) + 2;
						if (states == 2)
						{
							vidBuf[(fullY+y)*fullW+(fullX+x)] = COLMODALPHA(color1, 255);
						}
						else
						{
							auto mul = (tmp2 - 1) / float(states - 2);
							int colr = int(PIXR(color1) * mul + PIXR(color2) * (1.f - mul));
							int colg = int(PIXG(color1) * mul + PIXG(color2) * (1.f - mul));
							int colb = int(PIXB(color1) * mul + PIXB(color2) * (1.f - mul));
							vidBuf[(fullY+y)*fullW+(fullX+x)] = COLRGB(colr, colg, colb);
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

//Old saving
pixel *prerender_save_PSv(void *save, int size, int *width, int *height)
{
	unsigned char *d,*c=(unsigned char*)save,*m=NULL;
	int i,j,k,x,y,rx,ry,p=0, pc, gc;
	int bw,bh,w,h;
	pixel *fb;

	if (size<16)
		return NULL;
	if (!(c[2]==0x43 && c[1]==0x75 && c[0]==0x66) && !(c[2]==0x76 && c[1]==0x53 && c[0]==0x50))
		return NULL;
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
			if (wt == WL_EWALL || wt == WL_STASIS)
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
				//if (m[j] <= NPART) {
					fb[j] |= PIXRGB(d[p++],0,0);
				//} else {
				//	p++;
				//}
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
				//if (m[j] <= NPART) {
					fb[j] |= PIXRGB(0,d[p++],0);
				//} else {
				//	p++;
				//}
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
				//if (m[j] <= NPART) {
					fb[j] |= PIXRGB(0,0,d[p++]);
				//} else {
				//	p++;
				//}
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
