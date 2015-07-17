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

unsigned int wavelengthToDecoColour(int wavelength)
{
	int colr = 0, colg = 0, colb = 0, x;
	for (x=0; x<12; x++) {
		colr += (wavelength >> (x+18)) & 1;
		colb += (wavelength >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		colg += (wavelength >> (x+9))  & 1;
	x = 624/(colr+colg+colb+1);
	colr *= x;
	colg *= x;
	colb *= x;

	if(colr > 255) colr = 255;
	else if(colr < 0) colr = 0;
	if(colg > 255) colg = 255;
	else if(colg < 0) colg = 0;
	if(colb > 255) colb = 255;
	else if(colb < 0) colb = 0;

	return (255<<24) | (colr<<16) | (colg<<8) | colb;
}

int CRAY_update(UPDATE_FUNC_ARGS)
{
	int r, nxx, nyy, docontinue, nxi, nyi, rx, ry;
	// set ctype to things that touch it if it doesn't have one already
	if(parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !ptypes[parts[i].ctype].enabled) {
		int r, rx, ry;
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK)
				{
					r = photons[y+ry][x+rx];
					if (!r)
						r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if ((r&0xFF)!=PT_CRAY && (r&0xFF)!=PT_PSCN && (r&0xFF)!=PT_INST && (r&0xFF)!=PT_METL && (r&0xFF)!=PT_SPRK && (r&0xFF)<PT_NUM)
					{
						parts[i].ctype = r&0xFF;
						parts[i].temp = parts[r>>8].temp;
					}
				}
	} else if (parts[i].life==0) { // only fire when life is 0, but nothing sets the life right now
		for (rx=-1; rx<2; rx++)
			for (ry=-1; ry<2; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!(r&0xFF))
						continue;
					if ((r&0xFF)==PT_SPRK && parts[r>>8].life==3) { //spark found, start creating
						ARGBColour coloured = COLARGB(0, 0, 0, 0);
						int destroy = parts[r>>8].ctype==PT_PSCN;
						int nostop = parts[r>>8].ctype==PT_INST;
						int createSpark = (parts[r>>8].ctype==PT_INWR);
						int partsRemaining = 255;
						if (parts[i].tmp) //how far it shoots
							partsRemaining = parts[i].tmp;
						for (docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi) {
							if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
								break;
							}
							r = pmap[y+nyi+nyy][x+nxi+nxx];
							if (!sim->IsWallBlocking(x+nxi+nxx, y+nyi+nyy, parts[i].ctype) && (!pmap[y+nyi+nyy][x+nxi+nxx] || createSpark)) { // create, also set colour if it has passed through FILT
								int nr;
								// TODO: change these create_parts
								if (parts[i].ctype == PT_LIFE)
									nr = create_part(-1, x+nxi+nxx, y+nyi+nyy, parts[i].ctype|(parts[i].tmp2<<8));
								else
									nr = create_part(-1, x+nxi+nxx, y+nyi+nyy, parts[i].ctype);
								if (nr!=-1) {
									if (coloured)
										parts[nr].dcolour = coloured;
									parts[nr].temp = parts[i].temp;
									if(!--partsRemaining)
										docontinue = 0;
								}
							} else if ((r&0xFF)==PT_FILT) { // get colour if passed through FILT
								if (parts[r>>8].dcolour == COLRGB(0, 0, 0))
									coloured = COLRGB(0, 0, 0);
								else if (parts[r>>8].tmp == 0)
								{
									coloured = wavelengthToDecoColour(getWavelengths(&parts[r>>8]));
								}
								else if (coloured == COLRGB(0, 0, 0))
									coloured = COLARGB(0, 0, 0, 0);
								parts[r>>8].life = 4;
							} else if ((r&0xFF) == PT_CRAY || nostop) {
								docontinue = 1;
							} else if(destroy && r && ((r&0xFF) != PT_DMND)) {
								kill_part(r>>8);
								if(!--partsRemaining)
									docontinue = 0;
							}
							else
								docontinue = 0;
							if(!partsRemaining)
								docontinue = 0;
						}
					}
				}
	}
	return 0;
}

void CRAY_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_CRAY";
	elem->Name = "CRAY";
	elem->Colour = COLPACK(0xBBFF00);
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

	elem->DefaultProperties.temp = R_TEMP + 273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Particle Ray Emitter. Creates a beam of particles set by its ctype, with a range set by tmp.";

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

	elem->Update = &CRAY_update;
	elem->Graphics = NULL;
	elem->Init = &CRAY_init_element;
}
