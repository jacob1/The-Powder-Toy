#include <time.h>

#include "defines.h"
#include "gravity.h"
#include "graphics.h"
#include "hud.h"
#include "interface.h"
#include "luaconsole.h"
#include "powder.h"

#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"
#include "simulation/GolNumbers.h"
#include "simulation/elements/LIFE.h"

char uitext[512] = "";
char heattext[256] = "";
char coordtext[128] = "";
char tempstring[256] = "";
char timeinfotext[512] = "";
char infotext[512] = "";
int wavelength_gfx = 0;
int frameNum = 0; //for animated LCRY

int normalHud[HUD_OPTIONS];
int debugHud[HUD_OPTIONS];
int currentHud[HUD_OPTIONS];

void HudDefaults()
{
	int defaultNormalHud[HUD_OPTIONS] = {0,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,0,0,2,0,0,0,0,2,0,2,1,2,0,0,0,2,0,2,0,2,0,1,0,0,0,0,2,0,2,1,0,0,1,1,0,0,0};
	int defaultDebugHud[HUD_OPTIONS] =  {0,0,1,2,1,0,0,0,1,0,1,1,1,0,1,1,0,0,4,1,0,0,0,4,0,4,1,4,1,1,1,4,0,4,0,4,0,1,0,0,0,0,4,0,4,1,0,0,1,1,1,0,0};
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

void SetRightHudText(int x, int y)
{
	if (x >= 0 && x < XRES && y >= 0 && y < YRES)
		mouse_coords_window_to_sim(&x, &y);
	sprintf(heattext,"");
	sprintf(coordtext,"");
	if (y>=0 && y<YRES && x>=0 && x<XRES)
	{
		int cr,wl = 0; //cr is particle under mouse, for drawing HUD information
		char nametext[50] = "";
		if (photons[y][x]) {
			cr = photons[y][x];
		} else {
			cr = pmap[y][x];
			if ((cr&0xFF) == PT_PINV && parts[cr>>8].tmp2)
				cr = parts[cr>>8].tmp2;
		}
		if (!cr || !currentHud[10])
		{
			wl = bmap[y/CELL][x/CELL];
		}
		sprintf(heattext,""); sprintf(tempstring,"");
		if (cr)
		{
			if (currentHud[10])
			{
				if ((cr&0xFF)==PT_LIFE && parts[cr>>8].ctype>=0 && parts[cr>>8].ctype<NGOL)
				{
					if (currentHud[49] || !currentHud[11])
						sprintf(nametext, "%s, ", golTypes[parts[cr>>8].ctype].name.c_str());
					else
						sprintf(nametext, "%s (%s), ", ptypes[cr&0xFF].name, golTypes[parts[cr>>8].ctype].name.c_str());
				}
				else if (currentHud[13] && (cr&0xFF)==PT_LAVA && globalSim->IsElement(parts[cr>>8].ctype))
				{
					sprintf(nametext, "Molten %s, ", ptypes[parts[cr>>8].ctype].name);
				}
				else if (currentHud[50] && currentHud[11] && (cr&0xFF)==PT_FILT)
				{
					const char* filtModes[] = { "set colour", "AND", "OR", "subtract colour", "red shift", "blue shift", "no effect", "XOR", "NOT", "PHOT scatter" };
					if (parts[cr>>8].tmp>=0 && parts[cr>>8].tmp<=9)
						sprintf(nametext, "FILT (%s), ", filtModes[parts[cr>>8].tmp]);
					else
						sprintf(nametext, "FILT (unknown mode), ");
				}
				else if (currentHud[14] && currentHud[11] && ((cr&0xFF)==PT_PIPE || (cr&0xFF)==PT_PPIP) && globalSim->IsElement(parts[cr>>8].tmp&0xFF))
				{
					sprintf(nametext, "%s (%s), ", globalSim->elements[cr&0xFF].Name.c_str(), ptypes[parts[cr>>8].tmp&0xFF].name);
				}
				else if (currentHud[11])
				{
					int tctype = parts[cr>>8].ctype;
					if ((cr&0xFF)==PT_PIPE && currentHud[12]) //PIPE Overrides CTP2
					{
						tctype = parts[cr>>8].tmp&0xFF;
					}
					if (!currentHud[12] && (tctype>=PT_NUM || tctype<0 || (cr&0xFF)==PT_PHOT))
						tctype = 0;
					if (!tctype || globalSim->IsElement(tctype))
						sprintf(nametext, "%s (%s), ", ptypes[cr&0xFF].name, ptypes[tctype].name);
					else
						sprintf(nametext, "%s (%d), ", ptypes[cr&0xFF].name, tctype);
				}
				else
				{
					sprintf(nametext, "%s, ", ptypes[cr&0xFF].name);
				}
			}
			else if (currentHud[11])
			{
				if (parts[cr>>8].ctype > 0 && parts[cr>>8].ctype < PT_NUM)
					sprintf(nametext,"Ctype: %s ", ptypes[parts[cr>>8].ctype].name);
				else if (currentHud[12])
					sprintf(nametext,"Ctype: %d ", parts[cr>>8].ctype);
			}
			else if (wl && currentHud[48])
			{
				sprintf(nametext, "%s, ", wallTypes[wl].name.c_str());
			}
			else
				sprintf(nametext,"");
			strncpy(heattext,nametext,50);
			if (currentHud[15])
			{
				sprintf(tempstring,"Temp: %0.*f C, ",currentHud[18],parts[cr>>8].temp-273.15f);
				strappend(heattext,tempstring);
			}
			if (currentHud[16])
			{
				sprintf(tempstring,"Temp: %0.*f F, ",currentHud[18],((parts[cr>>8].temp-273.15f)*9/5)+32);
				strappend(heattext,tempstring);
			}
			if (currentHud[17])
			{
				sprintf(tempstring,"Temp: %0.*f K, ",currentHud[18],parts[cr>>8].temp);
				strappend(heattext,tempstring);
			}
			if (currentHud[19])
			{
				sprintf(tempstring,"Life: %d, ",parts[cr>>8].life);
				strappend(heattext,tempstring);
			}
			if (currentHud[20])
			{
				sprintf(tempstring,"Tmp: %d, ",parts[cr>>8].tmp);
				strappend(heattext,tempstring);
			}
			if (currentHud[21])
			{
				sprintf(tempstring,"Tmp2: %d, ",parts[cr>>8].tmp2);
				strappend(heattext,tempstring);
			}
			if (currentHud[46])
			{
				sprintf(tempstring,"Dcolour: 0x%.8X, ",parts[cr>>8].dcolour);
				strappend(heattext,tempstring);
			}
			if (currentHud[47])
			{
				sprintf(tempstring,"Flags: 0x%.8X, ",parts[cr>>8].flags);
				strappend(heattext,tempstring);
			}
			if (currentHud[22])
			{
				sprintf(tempstring,"X: %0.*f, Y: %0.*f, ",currentHud[23],parts[cr>>8].x,currentHud[23],parts[cr>>8].y);
				strappend(heattext,tempstring);
			}
			if (currentHud[24])
			{
				sprintf(tempstring,"Vx: %0.*f, Vy: %0.*f, ",currentHud[25],parts[cr>>8].vx,currentHud[25],parts[cr>>8].vy);
				strappend(heattext,tempstring);
			}
			if (currentHud[51])
			{
				sprintf(tempstring,"pavg[0]: %f, pavg[1]: %f, ",parts[cr>>8].pavg[0],parts[cr>>8].pavg[1]);
				strappend(heattext,tempstring);
			}
			if (currentHud[45] && ((cr&0xFF)==PT_PHOT || (cr&0xFF)==PT_BIZR || (cr&0xFF)==PT_BIZRG || (cr&0xFF)==PT_BIZRS || (cr&0xFF)==PT_FILT || (cr&0xFF)==PT_BRAY))
				wavelength_gfx = parts[cr>>8].ctype;
			if ((cr&0xFF) == PT_ANIM)
				frameNum = parts[cr>>8].tmp2+1;
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
			sprintf(tempstring,"Pressure: %0.*f, ",currentHud[27],pv[y/CELL][x/CELL]);
			strappend(heattext,tempstring);
		}
		if (strlen(heattext) > 1)
			heattext[strlen(heattext)-2] = '\0'; // delete comma and space at end

		if (currentHud[28] && cr)
		{
			if (currentHud[29] || (ngrav_enable && currentHud[30]))
				sprintf(tempstring,"#%d, ",cr>>8);
			else
				sprintf(tempstring,"#%d ",cr>>8);
			strappend(coordtext,tempstring);
		}
		if (currentHud[29])
		{
			sprintf(tempstring,"X:%d Y:%d ",x,y);
			strappend(coordtext,tempstring);
		}
		if (currentHud[30] && ngrav_enable && gravp[((y/CELL)*(XRES/CELL))+(x/CELL)])
		{
			sprintf(tempstring,"GX: %0.*f GY: %0.*f ",currentHud[31],gravx[((y/CELL)*(XRES/CELL))+(x/CELL)],currentHud[31],gravy[((y/CELL)*(XRES/CELL))+(x/CELL)]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[34] && aheat_enable)
		{
			sprintf(tempstring,"A.Heat: %0.*f K ",currentHud[35],hv[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[32])
		{
			sprintf(tempstring,"Pressure: %0.*f ",currentHud[33],pv[y/CELL][x/CELL]);
			strappend(coordtext,tempstring);
		}
		if (currentHud[43])
		{
			sprintf(tempstring,"VX: %0.*f VY: %0.*f ",currentHud[44],vx[y/CELL][x/CELL],currentHud[44],vy[y/CELL][x/CELL]);
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

void SetLeftHudText(float FPSB2)
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
		sprintf(uitext,"");
	if (currentHud[36] || currentHud[37] || currentHud[38])
	{
		time_t time2 = time(0);
		char time[256], *timestr = "";
		sprintf(time,"%i",time2);

		if (strlen(uitext))
		{
			uitext[strlen(uitext)-1] = ',';
			strappend(uitext," ");
		}
		converttotime(time,&timestr,currentHud[36],currentHud[38],currentHud[37]);
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
		sprintf(tempstring,"Parts:%d ",NUM_PARTS);
		strappend(uitext,tempstring);
	}
	if (currentHud[5])
	{
		sprintf(tempstring,"Generation:%d ", ((LIFE_ElementDataContainer*)globalSim->elementData[PT_LIFE])->golGeneration);
		strappend(uitext,tempstring);
	}
	if (currentHud[6])
	{
		sprintf(tempstring,"Gravity:%d ",gravityMode);
		strappend(uitext,tempstring);
	}
	if (currentHud[7])
	{
		sprintf(tempstring,"Air:%d ",airMode);
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
	if ((finding && finding != 8) && currentHud[8])
		strappend(uitext, "[FIND] ");
	if (GRID_MODE && currentHud[9])
	{
		sprintf(tempstring, "[GRID: %d] ", GRID_MODE);
		strappend(uitext, tempstring);
	}
	if (active_menu == SC_DECO && frameNum)
	{
		sprintf(tempstring,"[Frame %i/%i] ",frameNum,globalSim->maxFrames);
		strappend(uitext, tempstring);
		frameNum = 0;
	}
#ifdef INTERNAL
	if (vs)
		strappend(uitext, "[FRAME CAPTURE]");
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
	if (sdl_zoom_trig||zoom_en)
	{
		if (zoom_wx)
			heatx = XRES-16-heatlength;
		else
			heatx = 16;
		heaty = 270;
		alpha = 127;
	}
	else
	{
		heatx = XRES-16-heatlength;
		heaty = 16;
		alpha = std::min((int)((285 - qTipAlpha)*.5f), 127);
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
		if ((sdl_zoom_trig||zoom_en) && !zoom_wx)
		{
			if (coordlength > heatlength)
			{
				fillrect(vid_buf, 19+heatlength, 277, coordlength-heatlength, 15, 0, 0, 0, alpha);
				fillrect(vid_buf, 12, 280, heatlength+8, 12, 0, 0, 0, alpha);
			}
			else
				fillrect(vid_buf, 12, 280, coordlength+8, 12, 0, 0, 0, alpha);
			drawtext(vid_buf, 16, 281, coordtext, 255, 255, 255, (int)(alpha*1.5f));
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

//draws the photon colours in the HUD
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

void DrawRecordsInfo()
{
	int ytop = 244, num_parts = 0, totalselected = 0, x, y;
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
	for (y=0; y<YRES/CELL; y++)
		for (x=0; x<XRES/CELL; x++)
		{
			totalpressure += pv[y][x];
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
				sprintf(infotext,"%%%s: %f", golTypes[activeTools[0]->GetID()].name.c_str(),(float)totalselected/num_parts*100);
			else if (((ElementTool*)activeTools[0])->GetID() > 0)
				sprintf(infotext,"%%%s: %f", ptypes[activeTools[0]->GetID()].name,(float)totalselected/num_parts*100);
			else
				sprintf(infotext,"%%Empty: %f", (float)totalselected/XRES/YRES*100);
			fillrect(vid_buf, 12, ytop+108, textwidth(infotext)+8, 15, 0, 0, 0, 140);
			drawtext(vid_buf, 16, ytop+112, infotext, 255, 255, 255, 200);
		}
}

void DrawLuaLogs()
{
#ifdef LUACONSOLE
	int i;
	for (i = 0; i < 20; i++)
	{
		if (log_history[i])
		{
			int alpha = log_history_times[i]*5>255?255:log_history_times[i]*5;
			drawtext_outline(vid_buf, 16, (YRES-16)-i*12, log_history[i], 255, 255, 255, alpha, 0, 0, 0, alpha);
			log_history_times[i]--;
			if (log_history_times[i] < 0)
			{
				free(log_history[i]);
				log_history[i] = NULL;
			}
		}
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
