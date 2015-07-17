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

int getWavelengths(particle* cpart)
{
	if (cpart->ctype&0x3FFFFFFF)
	{
		return cpart->ctype;
	}
	else
	{
		int temp_bin = (int)((cpart->temp-273.0f)*0.025f);
		if (temp_bin < 0) temp_bin = 0;
		if (temp_bin > 25) temp_bin = 25;
		return (0x1F << temp_bin);
	}
}

// Returns the wavelengths in a particle after FILT interacts with it (e.g. a photon)
// cpart is the FILT particle, origWl the original wavelengths in the interacting particle
int interactWavelengths(particle* cpart, int origWl)
{
	const int mask = 0x3FFFFFFF;
	int filtWl = getWavelengths(cpart);
	switch (cpart->tmp)
	{
	case 0:
		return filtWl; //Assign Colour
	case 1:
		return origWl & filtWl; //Filter Colour
	case 2:
		return origWl | filtWl; //Add Colour
	case 3:
		return origWl & (~filtWl); //Subtract colour of filt from colour of photon
	case 4:
		{
			int shift = int((cpart->temp-273.0f)*0.025f);
			if (shift<=0) shift = 1;
			return (origWl << shift) & mask; // red shift
		  }
	case 5:
		{
			int shift = int((cpart->temp-273.0f)*0.025f);
			if (shift<=0) shift = 1;
			return (origWl >> shift) & mask; // blue shift
		  }
	case 6:
		return origWl; // No change
	case 7:
		return origWl ^ filtWl; // XOR colours
	case 8:
		return (~origWl) & mask; // Invert colours 
	case 9:
	{
		int t1 = (origWl & 0x0000FF)+(rand()%5)-2;
		int t2 = ((origWl & 0x00FF00)>>8)+(rand()%5)-2;
		int t3 = ((origWl & 0xFF0000)>>16)+(rand()%5)-2;
		return (origWl & 0xFF000000) | (t3<<16) | (t2<<8) | t1;
	}
	default:
		return filtWl;
	}
}

int FILT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x, wl = getWavelengths(cpart);
	*colg = 0;
	*colb = 0;
	*colr = 0;
	for (x=0; x<12; x++) {
		*colr += (wl >> (x+18)) & 1;
		*colb += (wl >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (wl >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	if (cpart->life>0 && cpart->life<=4)
		*cola = 127+cpart->life*30;
	else
		*cola = 127;
	*colr *= x;
	*colg *= x;
	*colb *= x;
	*pixel_mode &= ~PMODE;
	*pixel_mode |= PMODE_BLEND;
	return 0;
}

void FILT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FILT";
	elem->Name = "FILT";
	elem->Colour = COLPACK(0x000056);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
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

	elem->DefaultProperties.temp = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Filter for photons, changes the colour.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID | PROP_NOAMBHEAT | PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = NULL;
	elem->Graphics = &FILT_graphics;
	elem->Init = &FILT_init_element;
}
