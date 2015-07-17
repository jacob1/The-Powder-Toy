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
#include "simulation/GolNumbers.h"
#include "LIFE.h"

int LIFE_update(UPDATE_FUNC_ARGS)
{
	parts[i].temp = restrict_flt(parts[i].temp-50.0f, MIN_TEMP, MAX_TEMP);
	return 0;
}

int LIFE_graphics(GRAPHICS_FUNC_ARGS)
{
	ARGBColour col;
	if (cpart->ctype==NGT_LOTE)//colours for life states
	{
		if (cpart->tmp==2)
			col = COLRGB(255, 128, 0);
		else if (cpart->tmp==1)
			col = COLRGB(255, 255, 0);
		else
			col = COLRGB(255, 0, 0);
	}
	else if (cpart->ctype==NGT_FRG2)//colours for life states
	{
		if (cpart->tmp==2)
			col = COLRGB(0, 100, 50);
		else
			col = COLRGB(0, 255, 90);
	}
	else if (cpart->ctype==NGT_STAR)//colours for life states
	{
		if (cpart->tmp==4)
			col = COLRGB(0, 0, 128);
		else if (cpart->tmp==3)
			col = COLRGB(0, 0, 150);
		else if (cpart->tmp==2)
			col = COLRGB(0, 0, 190);
		else if (cpart->tmp==1)
			col = COLRGB(0, 0, 230);
		else
			col = COLRGB(0, 0, 70);
	}
	else if (cpart->ctype==NGT_FROG)//colours for life states
	{
		if (cpart->tmp==2)
			col = COLRGB(0, 100, 0);
		else
			col = COLRGB(0, 255, 0);
	}
	else if (cpart->ctype==NGT_BRAN)//colours for life states
	{
		if (cpart->tmp==1)
			col = COLRGB(150, 150, 0);
		else
			col = COLRGB(255, 255, 0);
	}
	else if (cpart->ctype >= 0 && cpart->ctype < NGOL)
	{
		col = golTypes[cpart->ctype].colour;
	}
	else
		col = globalSim->elements[cpart->type].Colour;
	*colr = COLR(col);
	*colg = COLG(col);
	*colb = COLB(col);
	return 0;
}

void LIFE_create(ELEMENT_CREATE_FUNC_ARGS)
{
	if (v >= 0 && v < NGOL)
	{
		parts[i].tmp = grule[v+1][9] - 1;
		parts[i].ctype = v;
	}
}

void LIFE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LIFE_GOL";
	elem->Name = "LIFE";
	elem->Colour = COLPACK(0x0CAC00);
	elem->MenuVisible = 0;
	elem->MenuSection = SC_LIFE;
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
	elem->Hardness = 0;

	elem->Weight = 100;

	elem->DefaultProperties.temp = 9000.0f;
	elem->HeatConduct = 40;
	elem->Latent = 0;
	elem->Description = "Game Of Life: Begin 3/Stay 23";

	elem->State = ST_NONE;
	elem->Properties = TYPE_SOLID|PROP_LIFE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &LIFE_update;
	elem->Graphics = &LIFE_graphics;
	elem->Func_Create = &LIFE_create;
	elem->Init = &LIFE_init_element;

	if (sim->elementData[t])
	{
		delete sim->elementData[t];
	}
	sim->elementData[t] = new LIFE_ElementDataContainer;
}
