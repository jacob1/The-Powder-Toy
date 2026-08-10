#include <ctime>
#include <cstring>
#include <sstream>

#include "defines.h"
#include "graphics.h"
#include "hud.h"
#include "interface.h"
#include "luaconsole.h"
#include "powder.h"

#include "common/Format.h"
#include "common/tpt-minmax.h"
#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"
#include "simulation/GolNumbers.h"

#include "simulation/elements/ANIM.h"
#include "simulation/elements/LIFE.h"
#include "simulation/elements/PLNT.h"

#include "gui/game/PowderToy.h"

char uitext[512] = "";
char heattext[256] = "";
char coordtext[128] = "";
char tempstring[256] = "";
char timeinfotext[128] = "";
char infotext[512] = "";
int wavelength_gfx = 0;
int frameNum = 0; //for animated LCRY

int normalHud[HUD_OPTIONS];
int debugHud[HUD_OPTIONS];
int currentHud[HUD_OPTIONS];

void HudDefaults()
{
	int defaultNormalHud[HUD_OPTIONS] = {0,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,0,0,2,0,0,0,0,2,0,2,1,2,0,0,0,2,0,2,0,2,0,1,0,0,0,0,2,0,2,1,0,0,1,1,0,0,0,0,0};
	int defaultDebugHud[HUD_OPTIONS] =  {0,0,1,2,1,0,0,0,1,0,1,1,1,0,1,0,0,0,4,1,1,1,0,4,0,4,1,4,1,1,1,4,0,4,0,4,0,1,0,0,0,0,4,0,4,1,0,0,1,1,1,0,0,0,1};
	memcpy(normalHud, defaultNormalHud, sizeof(normalHud));
	memcpy(debugHud, defaultDebugHud, sizeof(debugHud));
}

void SetCurrentHud()
{
	if (!DEBUG_MODE)
		memcpy(currentHud, normalHud, sizeof(currentHud));
	else
		memcpy(currentHud, debugHud, sizeof(currentHud));
}

std::string ElementResolve(Simulation * sim, int type, int ctype)
{
	if (!type || type < 0 || type > PT_NUM)
		return "";
	return sim->ElementResolve(type, ctype);
}

void SetRightHudText(Simulation * sim, int x, int y)
{
	if (x >= 0 && x < XRES && y >= 0 && y < YRES)
	{
		Point cursor = the_game->AdjustCoordinates(Point(x, y));
		x = cursor.X;
		y = cursor.Y;
	}
	heattext[0] = '\0';
	coordtext[0] = '\0';
	if (y>=0 && y<YRES && x>=0 && x<XRES)
	{
		int cr,wl = 0; //cr is particle under mouse, for drawing HUD information
		std::stringstream nametext;
		if (photons[y][x]) {
			cr = photons[y][x];
		} else {
			cr = pmap[y][x];
#ifndef NOMOD
			if (TYP(cr) == PT_PINV && parts[ID(cr)].tmp2)
				cr = parts[ID(cr)].tmp2;
#endif
		}
		if (!cr || !currentHud[10])
		{
			wl = bmap[y/CELL][x/CELL];
		}
		int underType = TYP(cr);
		int underID = ID(cr);
		heattext[0] = '\0';
		tempstring[0] = '\0';
		if (cr)
		{
			if (currentHud[45] && (underType == PT_PHOT || underType == PT_BIZR || underType == PT_BIZRG || underType == PT_BIZRS || underType == PT_FILT || underType == PT_BRAY))
				wavelength_gfx = (parts[underID].ctype&0x3FFFFFFF);
			if (currentHud[10])
			{
				if (underType == PT_LIFE)
				{
					if (currentHud[49] || !currentHud[11])
						nametext << ElementResolve(sim, PT_LIFE, parts[underID].ctype);
					else
						nametext << ElementResolve(sim, underType, 0) << " (" << ElementResolve(sim, PT_LIFE, parts[underID].ctype) << ")";
				}
				else if (currentHud[13] && underType == PT_LAVA && sim->IsElement(parts[underID].ctype))
				{
					nametext << "Molten " << ElementResolve(sim, parts[underID].ctype, 0);
				}
				else if (currentHud[50] && currentHud[11] && underType == PT_FILT)
				{
					const char* filtModes[] = { "set color", "AND", "OR", "AND-NOT", "red shift", "blue shift", "no effect", "XOR", "NOT", "PHOT scatter", "variable red shift", "variable blue shift" };
					if (parts[underID].tmp >= 0 && parts[underID].tmp <= 11)
						nametext << "FILT (" << filtModes[parts[underID].tmp] << ")";
					else
						nametext << "FILT (unknown mode)";
				}
				else if (currentHud[54] && currentHud[11] && (underType == PT_SEED || (underType == PT_PLNT && parts[underID].ctype)))
				{
					int ctype = parts[underID].ctype;
					nametext << ElementResolve(sim, underType, ctype);

					auto water = (ctype >> PLNT_LIFE) & 0xFF;
					auto colour = (ctype >> PLNT_COLOUR) & 0x3F;
					auto dir = (ctype >> PLNT_DIR) & 7;
					auto active = ctype & 1;

					static const std::array<std::string, 8> directions = {"N", "NW", "W", "SW", "S", "SE", "E", "NE"};
					static const std::array<std::array<std::string, 4>, 3> colours = {{
						{{"cc", "cC", "Cc", "CC"}}, {{"mm", "mM", "Mm", "MM"}}, {{"yy", "yY", "Yy", "YY"}}
					}};
					auto cyan = (colour >> 4) & 3;
					auto magenta = (colour >> 2) & 3;
					auto yellow = colour & 3;

					nametext << " (" << water << " " <<
						colours[0][cyan] << colours[1][magenta] << colours[2][yellow] << " " << directions[dir] << " " << active << ")";
				}
				else if (currentHud[11])
				{
					int tctype = parts[underID].ctype;
					nametext << ElementResolve(sim, underType, tctype);
					if (!currentHud[12] && (tctype >= PT_NUM || tctype < 0 || underType == PT_PHOT))
						tctype = 0;
					if (wavelength_gfx || underType == PT_EMBR || underType == PT_PRTI || underType == PT_PRTO)
					{
						// Do nothing, ctype is meaningless for these elements
					}
					else if (currentHud[49] && (underType == PT_CRAY || underType == PT_DRAY || underType == PT_CONV || underType == PT_LDTC))
						nametext << " (" << ElementResolve(sim, TYP(parts[underID].ctype), ID(parts[underID].ctype)) << ")";
					else if (currentHud[49] && (underType == PT_CLNE || underType == PT_BCLN || underType == PT_PCLN || underType == PT_PBCN || underType == PT_DTEC))
						nametext << " (" << ElementResolve(sim, parts[underID].ctype, parts[underID].tmp) << ")";
					else if (sim->IsElement(tctype) && underType != PT_GLOW && underType != PT_WIRE && underType != PT_SOAP && underType != PT_LITH)
						nametext << " (" << ElementResolve(sim, tctype, 0) << ")";
					else if (currentHud[12] && tctype)
						nametext << " (" << tctype << ")";
				}
				else
				{
					nametext << ElementResolve(sim, underType, 0);
				}
			}
			else if (currentHud[11])
			{
				if (parts[underID].ctype > 0 && parts[underID].ctype < PT_NUM)
					nametext << "Ctype: " << ElementResolve(sim, parts[underID].ctype, 0);
				else if (currentHud[12])
					nametext << "Ctype: " << parts[underID].ctype;
			}
			else if (wl && currentHud[48])
			{
				nametext << wallTypes[wl].name;
			}
			if (!nametext.str().empty())
				nametext << ", ";
			strncpy(heattext, nametext.str().c_str(), 50);
			if (currentHud[14])
			{
				std::string tempStr = Format::TemperatureToString(parts[underID].temp, sim->temperatureScale, currentHud[18]);
				sprintf(tempstring,"Temp: %s, ", tempStr.c_str());
				strappend(heattext,tempstring);
			}
			if (currentHud[15] && (!currentHud[14] || sim->temperatureScale != 1))
			{
				std::string tempStr = Format::TemperatureToString(parts[underID].temp, 1, currentHud[18]);
				sprintf(tempstring,"Temp: %s, ", tempStr.c_str());
				strappend(heattext,tempstring);
			}
			if (currentHud[16] && (!currentHud[14] || sim->temperatureScale != 2))
			{
				std::string tempStr = Format::TemperatureToString(parts[underID].temp, 2, currentHud[18]);
				sprintf(tempstring,"Temp: %s, ", tempStr.c_str());
				strappend(heattext,tempstring);
			}
			if (currentHud[17] && (!currentHud[14] || sim->temperatureScale != 0))
			{
				std::string tempStr = Format::TemperatureToString(parts[underID].temp, 0, currentHud[18]);
				sprintf(tempstring,"Temp: %s, ", tempStr.c_str());
				strappend(heattext,tempstring);
			}
			if (currentHud[19])
			{
				sprintf(tempstring,"Life: %d, ",parts[underID].life);
				strappend(heattext,tempstring);
			}
			if (currentHud[20])
			{
				if (underType != PT_RFRG && underType != PT_RFGL && underType != PT_LIFE)
				{
					sprintf(tempstring,"Tmp: %d, ",parts[underID].tmp);
					strappend(heattext,tempstring);
				}
			}
			if (currentHud[53] || (currentHud[21] &&
					(underType == PT_CRAY || underType == PT_DRAY || underType == PT_EXOT || underType == PT_LIGH || underType == PT_SOAP || underType == PT_TRON ||
					 underType == PT_VIBR || underType == PT_VIRS || underType == PT_WARP || underType == PT_LCRY || underType == PT_CBNW || underType == PT_TSNS ||
					 underType == PT_DTEC || underType == PT_LSNS || underType == PT_PSTN || underType == PT_LDTC || underType == PT_VSNS|| underType == PT_LITH ||
					 underType == PT_CONV || underType == PT_ETRD)))
			{
				sprintf(tempstring,"Tmp2: %d, ",parts[underID].tmp2);
				strappend(heattext,tempstring);
			}
			if (currentHud[46])
			{
				sprintf(tempstring,"Dcolor: 0x%.8X, ",parts[underID].dcolour);
				strappend(heattext,tempstring);
			}
			if (currentHud[47])
			{
				sprintf(tempstring,"Flags: 0x%.8X, ",parts[underID].flags);
				strappend(heattext,tempstring);
			}
			if (currentHud[22])
			{
				sprintf(tempstring,"X: %0.*f, Y: %0.*f, ",currentHud[23],parts[underID].x,currentHud[23],parts[underID].y);
				strappend(heattext,tempstring);
			}
			if (currentHud[24])
			{
				sprintf(tempstring,"Vx: %0.*f, Vy: %0.*f, ",currentHud[25],parts[underID].vx,currentHud[25],parts[underID].vy);
				strappend(heattext,tempstring);
			}
			if (currentHud[51])
			{
				sprintf(tempstring,"tmp3: %d, tmp4: %d, ",parts[underID].tmp3,parts[underID].tmp4);
				strappend(heattext,tempstring);
			}
#ifndef NOMOD
			if (underType == PT_ANIM)
				frameNum = parts[underID].tmp2 + 1;
#endif
		}
		else if (wl && currentHud[48])
		{
			sprintf(heattext, "%s, ", wallTypes[wl].name.c_str());
		}
		else
		{
			if (currentHud[10])
				sprintf(heattext,"Empty, ");
		}
		if (currentHud[26])
		{
			sprintf(tempstring,"Pressure: %0.*f, ",currentHud[27],sim->air->pv[y/CELL][x/CELL]);
			strappend(heattext,tempstring);
		}
		if (strlen(heattext) > 1)
			heattext[strlen(heattext)-2] = '\0'; // delete comma and space at end

		if (currentHud[28] && cr)
		{
			if (currentHud[29] || (sim->grav->IsEnabled() && currentHud[30]))
				sprintf(tempstring,"#%d, ",underID);
			else
				sprintf(tempstring,"#%d ",underID);
			strappend(coordtext,tempstring);
		}
		if (currentHud[29])
		{
			sprintf(tempstring,"X:%d Y:%d ",x,y);
			strappend(coordtext,tempstring);
		}
		if (currentHud[30] && sim->grav->IsEnabled() && sim->grav->gravp[((y/CELL)*(XRES/CELL))+(x/CELL)])
		{
			sprintf(tempstring,"GX: %0.*f GY: %0.*f ", currentHud[31], sim->grav->gravx[((y/CELL)*(XRES/CELL))+(x/CELL)], currentHud[31], sim->grav->gravy[((y/CELL)*(XRES/CELL))+(x/CELL)]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[34] && aheat_enable)
		{
			sprintf(tempstring,"A.Heat: %0.*f K ",currentHud[35],sim->air->hv[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[32])
		{
			sprintf(tempstring,"Pressure: %0.*f ",currentHud[33],sim->air->pv[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[43])
		{
			sprintf(tempstring,"VX: %0.*f VY: %0.*f ",currentHud[44],sim->air->vx[y/CELL][x/CELL],currentHud[44],sim->air->vy[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[52])
		{
			sprintf(tempstring,"emap: %d",emap[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (strlen(coordtext) > 0 && coordtext[strlen(coordtext)-1] == ' ')
			coordtext[strlen(coordtext)-1] = 0;
	}
	else
	{
		if (currentHud[10])
			sprintf(heattext, "Empty");
		if (currentHud[29])
			sprintf(coordtext, "X:%d Y:%d", x, y);
	}
}

void SetLeftHudText(Simulation * sim, float FPSB2)
{
#ifdef BETA
	if (currentHud[0] && currentHud[1])
		sprintf(uitext, "Version %d Beta %d (%d) ", SAVE_VERSION, MINOR_VERSION, BUILD_NUM);
	else if (currentHud[0] && !currentHud[1])
		sprintf(uitext, "Version %d Beta %d ", SAVE_VERSION, MINOR_VERSION);
	else if (!currentHud[0] && currentHud[1])
		sprintf(uitext, "Beta Build %d ", BUILD_NUM);
#else
	if (currentHud[0] && currentHud[1])
		sprintf(uitext, "Version %d.%d (%d) ", SAVE_VERSION, MINOR_VERSION, BUILD_NUM);
	else if (currentHud[0] && !currentHud[1])
		sprintf(uitext, "Version %d.%d ", SAVE_VERSION, MINOR_VERSION);
	else if (!currentHud[0] && currentHud[1])
		sprintf(uitext, "Build %d ", BUILD_NUM);
#endif
	else
		uitext[0] = '\0';
	if (currentHud[36] || currentHud[37] || currentHud[38])
	{
		time_t time2 = time(0);
		char *timestr = NULL;

		if (strlen(uitext))
		{
			uitext[strlen(uitext)-1] = ',';
			strappend(uitext," ");
		}
		converttotime(time2,&timestr,currentHud[36],currentHud[38],currentHud[37]);
		strappend(uitext,timestr);
		strappend(uitext,", ");
		free(timestr);
	}
	if (currentHud[2])
	{
		sprintf(tempstring,"FPS:%0.*f ",currentHud[3],FPSB2);
		strappend(uitext,tempstring);
	}
	if (currentHud[4])
	{
		if (finding & ~0x8)
			sprintf(tempstring,"Parts: %d/%d ", foundParticles, NUM_PARTS);
		else
			sprintf(tempstring,"Parts: %d ", NUM_PARTS);
		strappend(uitext,tempstring);
	}
	if (currentHud[5])
	{
		sprintf(tempstring,"Generation:%d ", static_cast<LIFE_ElementDataContainer&>(*sim->elementData[PT_LIFE]).golGeneration);
		strappend(uitext,tempstring);
	}
	if (currentHud[6])
	{
		sprintf(tempstring,"Gravity:%d ", sim->gravityMode);
		strappend(uitext,tempstring);
	}
	if (currentHud[7])
	{
		sprintf(tempstring,"Air:%d ", luaSim->air->airMode);
		strappend(uitext,tempstring);
	}
	if (currentHud[39])
	{
		GetTimeString(currentTime-totalafktime-afktime, timeinfotext, 2);
		sprintf(tempstring,"Time Played: %s ", timeinfotext);
		strappend(uitext,tempstring);
	}
	if (currentHud[40])
	{
		GetTimeString(totaltime+currentTime-totalafktime-afktime, timeinfotext, 1);
		sprintf(tempstring,"Total Time Played: %s ", timeinfotext);
		strappend(uitext,tempstring);
	}
	if (currentHud[41] && frames)
	{
		sprintf(tempstring,"Average FPS: %0.*f ", currentHud[42], totalfps/frames);
		strappend(uitext,tempstring);
	}
	if (REPLACE_MODE && currentHud[8])
		strappend(uitext, "[REPLACE MODE] ");
	if (SPECIFIC_DELETE && currentHud[8])
		strappend(uitext, "[SPECIFIC DELETE] ");
	if ((finding & ~0x8) && currentHud[8])
		strappend(uitext, "[FIND] ");
	if (GRID_MODE && currentHud[9])
	{
		sprintf(tempstring, "[GRID: %d] ", GRID_MODE);
		strappend(uitext, tempstring);
	}
#ifndef NOMOD
	if (active_menu == SC_DECO && frameNum)
	{
		sprintf(tempstring,"[Frame %i/%i] ",frameNum, static_cast<ANIM_ElementDataContainer&>(*sim->elementData[PT_ANIM]).GetMaxFrames());
		strappend(uitext, tempstring);
		frameNum = 0;
	}
#endif
	if (strlen(uitext) > 0 && uitext[strlen(uitext)-1] == ' ')
		uitext[strlen(uitext)-1] = 0;
}

void DrawHud(int introTextAlpha, int qTipAlpha)
{
	int heatlength = textwidth(heattext);
	int coordlength = textwidth(coordtext);
	int heatx, heaty, alpha;
	int introTextInvert = std::max(std::min(250 - introTextAlpha, 250), 0);

	if (strlen(uitext) > 0)
	{
		fillrect(vid_buf, 12, 12, textwidth(uitext)+8, 15, 0, 0, 0, (int)(introTextInvert/2));
		drawtext(vid_buf, 16, 16, uitext, 32, 216, 255, (int)(introTextInvert*.8));
	}
	if (the_game->ZoomWindowShown())
	{
		if (the_game->GetZoomWindowPosition().X >= XRES/2)
			heatx = XRES-16-heatlength;
		else
			heatx = 16;
		heaty = the_game->GetZoomWindowPosition().Y + the_game->GetZoomScopeSize()*the_game->GetZoomWindowFactor() + 7;
		alpha = 127;
	}
	else
	{
		heatx = XRES-16-heatlength;
		heaty = 16;
		alpha = std::min((int)(introTextInvert/2), std::min((int)((285 - qTipAlpha)*.5f), 127));
	}
	if (strlen(heattext) > 0)
	{
		fillrect(vid_buf, heatx-4, heaty-4, heatlength+8, 15, 0, 0, 0, alpha);
		drawtext(vid_buf, heatx, heaty, heattext, 255, 255, 255, (int)(alpha*1.5f));
		if (wavelength_gfx)
			DrawPhotonWavelengths(vid_buf,heatx-4,heaty-5,2,wavelength_gfx);
	}
	if (DEBUG_MODE && strlen(coordtext) > 0)
	{
		if (the_game->ZoomWindowShown() && the_game->GetZoomWindowPosition().X < XRES/2)
		{
			if (coordlength > heatlength)
			{
				fillrect(vid_buf, 19+heatlength, heaty+7, coordlength-heatlength, 15, 0, 0, 0, alpha);
				fillrect(vid_buf, 12, heaty+10, heatlength+8, 12, 0, 0, 0, alpha);
			}
			else
				fillrect(vid_buf, 12, heaty+10, coordlength+8, 12, 0, 0, 0, alpha);
			drawtext(vid_buf, 16, heaty+11, coordtext, 255, 255, 255, (int)(alpha*1.5f));
		}
		else
		{
			if (coordlength > heatlength)
			{
				fillrect(vid_buf, XRES-20-coordlength, heaty+7, coordlength-heatlength+1, 15, 0, 0, 0, alpha);
				fillrect(vid_buf, XRES-20-heatlength, heaty+10, heatlength+8, 12, 0, 0, 0, alpha);
			}
			else
				fillrect(vid_buf, XRES-20-coordlength, heaty+10, coordlength+8, 12, 0, 0, 0, alpha);
			drawtext(vid_buf, XRES-16-coordlength, heaty+11, coordtext, 255, 255, 255, (int)(alpha*1.5f));
		}
	}
	wavelength_gfx = 0;
}

//draws the photon colors in the HUD
void DrawPhotonWavelengths(pixel *vid, int x, int y, int h, int wl)
{
	int i,cr,cg,cb,j;
	int tmp;
	fillrect(vid,x-1,y-1,30+1,h+1,64,64,64,255); // coords -1 size +1 to work around bug in fillrect - TODO: fix fillrect
	for (i=0; i<30; i++)
	{
		if ((wl>>i)&1)
		{
			// Need a spread of wavelengths to get a smooth spectrum, 5 bits seems to work reasonably well
			if (i>2) tmp = 0x1F << (i-2);
			else tmp = 0x1F >> (2-i);
			cg = 0;
			cb = 0;
			cr = 0;
			for (j=0; j<12; j++) {
				cr += (tmp >> (j+18)) & 1;
				cb += (tmp >>  j)     & 1;
			}
			for (j=0; j<13; j++)
				cg += (tmp >> (j+9))  & 1;
			tmp = 624/(cr+cg+cb+1);
			cr *= tmp;
			cg *= tmp;
			cb *= tmp;
			for (j=0; j<h; j++) blendpixel(vid,x+29-i,y+j,cr>255?255:cr,cg>255?255:cg,cb>255?255:cb,255);
		}
	}
}

void DrawRecordsInfo(Simulation * sim)
{
	int ytop = 244, num_parts = 0, totalselected = 0;
	float totaltemp = 0, totalpressure = 0;
	for (int i = 0; i < NPART; i++)
	{
		//average temperature of all particles
		if (parts[i].type)
		{
			totaltemp += parts[i].temp;
			num_parts++;
		}

		//count total number of left selected element particles
		if (parts[i].type == PT_LIFE)
		{
			if (parts[i].ctype == ((GolTool*)activeTools[0])->GetID())
				totalselected++;
		}
		else if (parts[i].type == ((ElementTool*)activeTools[0])->GetID())
			totalselected++;
	}
	for (int y = 0; y < YRES/CELL; y++)
		for (int x = 0; x < XRES/CELL; x++)
		{
			totalpressure += sim->air->pv[y][x];
		}

	GetTimeString(currentTime-totalafktime-afktime, timeinfotext, 0);
	sprintf(infotext,"Time Played: %s", timeinfotext);
	fillrect(vid_buf, 12, ytop-4, textwidth(infotext)+8, 15, 0, 0, 0, 140);
	drawtext(vid_buf, 16, ytop, infotext, 255, 255, 255, 200);
	GetTimeString(totaltime+currentTime-totalafktime-afktime, timeinfotext, 0);
	sprintf(infotext,"Total Time Played: %s", timeinfotext);
	fillrect(vid_buf, 12, ytop+10, textwidth(infotext)+8, 15, 0, 0, 0, 140);
	drawtext(vid_buf, 16, ytop+14, infotext, 255, 255, 255, 200);
	GetTimeString(totalafktime+afktime+prevafktime, timeinfotext, 0);
	sprintf(infotext,"Total AFK Time: %s", timeinfotext);
	fillrect(vid_buf, 12, ytop+24, textwidth(infotext)+8, 15, 0, 0, 0, 140);
	drawtext(vid_buf, 16, ytop+28, infotext, 255, 255, 255, 200);
	if (frames)
	{
		sprintf(infotext,"Average FPS: %f", totalfps/frames);
		fillrect(vid_buf, 12, ytop+38, textwidth(infotext)+8, 15, 0, 0, 0, 140);
		drawtext(vid_buf, 16, ytop+42, infotext, 255, 255, 255, 200);
	}
	sprintf(infotext,"Number of Times Played: %i", timesplayed);
	fillrect(vid_buf, 12, ytop+52, textwidth(infotext)+8, 15, 0, 0, 0, 140);
	drawtext(vid_buf, 16, ytop+56, infotext, 255, 255, 255, 200);
	if (timesplayed)
	{
		GetTimeString((totaltime+currentTime-totalafktime-afktime)/timesplayed, timeinfotext, 0);
		sprintf(infotext,"Average Time Played: %s", timeinfotext);
		fillrect(vid_buf, 12, ytop+66, textwidth(infotext)+8, 15, 0, 0, 0, 140);
		drawtext(vid_buf, 16, ytop+70, infotext, 255, 255, 255, 200);
	}
	if (num_parts)
	{
		sprintf(infotext,"Average Temp: %f C", totaltemp/num_parts-273.15f);
		fillrect(vid_buf, 12, ytop+80, textwidth(infotext)+8, 15, 0, 0, 0, 140);
		drawtext(vid_buf, 16, ytop+84, infotext, 255, 255, 255, 200);
	}
	sprintf(infotext,"Average Pressure: %f", totalpressure/(XRES*YRES/CELL/CELL));
	fillrect(vid_buf, 12, ytop+94, textwidth(infotext)+8, 15, 0, 0, 0, 140);
	drawtext(vid_buf, 16, ytop+98, infotext, 255, 255, 255, 200);
	if (num_parts)
	{
		if (activeTools[0]->GetType() == GOL_TOOL)
			sprintf(infotext,"%%%s: %f", sim->ElementResolve(PT_LIFE, activeTools[0]->GetID()).c_str(),(float)totalselected/num_parts*100);
		else if (((ElementTool*)activeTools[0])->GetID() > 0)
			sprintf(infotext,"%%%s: %f", sim->elements[activeTools[0]->GetID()].Name.c_str(),(float)totalselected/num_parts*100);
		else
			sprintf(infotext,"%%Empty: %f", (float)totalselected/XRES/YRES*100);
		fillrect(vid_buf, 12, ytop+108, textwidth(infotext)+8, 15, 0, 0, 0, 140);
		drawtext(vid_buf, 16, ytop+112, infotext, 255, 255, 255, 200);
	}
}

void DrawLuaLogs()
{
#ifdef LUACONSOLE
	for (int i = logHistory.size() - 1; i >= 0; i--)
	{
		auto logPair = logHistory[i];
		int alpha = logPair.second * 5;
		if (alpha > 255)
			alpha = 255;
		fillrect(vid_buf, 13, (YRES - 19) - i * 13, textwidth(logPair.first.c_str()) + 6, 14, 0, 0, 0, std::min(100, alpha));
		drawtext(vid_buf, 16, (YRES - 16) - i * 13, logPair.first.c_str(), 255, 255, 255, alpha);

		logHistory[i].second--;
		if (logPair.second <= 1)
			logHistory.pop_back();
	}
#endif
}

void GetTimeString(int currtime, char *string, int length)
{
	int years = 0, days = 0, hours = 0, minutes, seconds, milliseconds;
	if (length == 0)
	{
		years = (int)(currtime/31557600000ULL);
		currtime = currtime%31557600000ULL;
		days = currtime/86400000;
		currtime = currtime%86400000;
	}
	if (length <= 1)
	{
		hours = currtime/3600000;
		currtime = currtime%3600000;
	}
	minutes = currtime/60000;
	currtime = currtime%60000;
	seconds = currtime/1000;
	currtime = currtime%1000;
	milliseconds = currtime;
	if (length == 0)
		sprintf(string,"%i year%s, %i day%s, %i hour%s, %i minute%s, %i second%s, %i millisecond%s",years,(years == 1)?"":"s",days,(days == 1)?"":"s",hours,(hours == 1)?"":"s",minutes,(minutes == 1)?"":"s",seconds,(seconds == 1)?"":"s",milliseconds,(milliseconds == 1)?"":"s");
	else if (length == 1)
		sprintf(string,"%i hour%s, %i minute%s, %i second%s",hours,(hours == 1)?"":"s",minutes,(minutes == 1)?"":"s",seconds,(seconds == 1)?"":"s");
	else if (length == 2)
		sprintf(string,"%i minute%s, %i second%s",minutes,(minutes == 1)?"":"s",seconds,(seconds == 1)?"":"s");
}
