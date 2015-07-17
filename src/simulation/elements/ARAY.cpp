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

#include "simulation/ElementsCommon.h"

int ARAY_update(UPDATE_FUNC_ARGS)
{
	int r, nxx, nyy, docontinue, nxi, nyi, rx, ry, ry1, rx1;
	if (parts[i].life==0) {
		int coloured =0;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)==PT_SPRK && parts[r>>8].life==3) {
						bool isBlackDeco = false;
						int destroy = (parts[r>>8].ctype==PT_PSCN)?1:0;
						int nostop = (parts[r>>8].ctype==PT_INST)?1:0;
						for (docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi) {
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
								break;
							}
							r = pmap[y+nyi+nyy][x+nxi+nxx];
							if (!r) {
								int nr = sim->part_create(-1, x+nxi+nxx, y+nyi+nyy, PT_BRAY);
								if (nr!=-1) {
									if (destroy) {//if it came from PSCN
										parts[nr].tmp = 2;
										parts[nr].life = 2;
									} else
										parts[nr].ctype = coloured;
									parts[nr].temp = parts[i].temp;
									if (isBlackDeco)
										parts[nr].dcolour = COLRGB(0, 0, 0);
								}
							} else if (!destroy) {
								if ((r&0xFF)==PT_BRAY)
								{
									switch (parts[r>>8].tmp)
									{
									case 0:
										//if it hits another BRAY that isn't red
										if (parts[r>>8].tmp==0)
										{
											if (nyy!=0 || nxx!=0)
											{
												parts[r>>8].life = 1020; //makes it last a while
												parts[r>>8].tmp = 1;
												if (!parts[r>>8].ctype) //and colours it if it isn't already
													parts[r>>8].ctype = coloured;
											}
											docontinue = 0;//then stop it
										}
										break;
									case 2://red bray, stop
									default:
										docontinue = 0;
										break;
									case 1://if it hits one that already was a long life, reset it
										parts[r>>8].life = 1020;
										//docontinue = 1;
										break;
									}
									if (isBlackDeco)
										parts[r>>8].dcolour = COLRGB(0, 0, 0);
								} else if ((r&0xFF)==PT_FILT) {//get colour if passed through FILT
									if (parts[r>>8].tmp != 6)
									{
										coloured = interactWavelengths(&parts[r>>8], coloured);
										if (!coloured)
											break;
									}
									isBlackDeco = (parts[r>>8].dcolour==COLRGB(0, 0, 0));
									parts[r>>8].life = 4;
								//this if prevents BRAY from stopping on certain materials
								} else if ((r&0xFF)!=PT_STOR && (r&0xFF)!=PT_INWR && ((r&0xFF)!=PT_SPRK || parts[r>>8].ctype!=PT_INWR) && (r&0xFF)!=PT_ARAY && (r&0xFF)!=PT_WIFI && !((r&0xFF)==PT_SWCH && parts[r>>8].life>=10)) {
									if ((nyy!=0 || nxx!=0) && (r&0xFF) != PT_WIRE)
									{
										sim->spark_all_attempt(r>>8, x+nxi+nxx, y+nyi+nyy);
									}
									if (!(nostop && parts[r>>8].type==PT_SPRK && parts[r>>8].ctype >= 0 && parts[r>>8].ctype < PT_NUM && (ptypes[parts[r>>8].ctype].properties&PROP_CONDUCTS))) {
										docontinue = 0;
									} else {
										docontinue = 1;
									}
								} else if((r&0xFF)==PT_STOR) {
									if(parts[r>>8].tmp)
									{
										//Cause STOR to release
										for(ry1 = 1; ry1 >= -1; ry1--){
											for(rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1){
												int np = sim->part_create(-1, x+nxi+nxx+rx1, y+nyi+nyy+ry1, parts[r>>8].tmp);
												if (np!=-1)
												{
													parts[np].temp = parts[r>>8].temp;
													parts[np].life = parts[r>>8].tmp2;
													parts[np].tmp = (int)parts[r>>8].pavg[0];
													parts[np].ctype = (int)parts[r>>8].pavg[1];
													parts[r>>8].tmp = 0;
													parts[r>>8].life = 10;
													break;
												}
											}
										}
									}
									else
									{
										parts[r>>8].life = 10;
									}
								}
							} else if (destroy) {
								if ((r&0xFF)==PT_BRAY) {
									parts[r>>8].tmp = 2;
									parts[r>>8].life = 1;
									docontinue = 1;
									if (isBlackDeco)
										parts[r>>8].dcolour = COLRGB(0, 0, 0);
								//this if prevents red BRAY from stopping on certain materials
								} else if ((r&0xFF)==PT_STOR || (r&0xFF)==PT_INWR || ((r&0xFF)==PT_SPRK && parts[r>>8].ctype==PT_INWR) || (r&0xFF)==PT_ARAY || (r&0xFF)==PT_WIFI || (r&0xFF)==PT_FILT || ((r&0xFF)==PT_SWCH && parts[r>>8].life>=10)) {
									if((r&0xFF)==PT_STOR)
									{
										parts[r>>8].tmp = 0;
										parts[r>>8].life = 0;
									}
									else if ((r&0xFF)==PT_FILT)
									{
										isBlackDeco = (parts[r>>8].dcolour==COLRGB(0, 0, 0));
										parts[r>>8].life = 2;
									}
									docontinue = 1;
								} else {
									docontinue = 0;
								}
							}
						}
					}
					//parts[i].life = 4;
				}
	}
	return 0;
}

void ARAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_ARAY";
	elem->Name = "ARAY";
	elem->Colour = COLPACK(0xFFBB00);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_ELEC;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->DefaultProperties.temp = R_TEMP+0.0f +273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Ray Emitter. Rays create points when they collide.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &ARAY_update;
	elem->Graphics = NULL;
	elem->Init = &ARAY_init_element;
}
