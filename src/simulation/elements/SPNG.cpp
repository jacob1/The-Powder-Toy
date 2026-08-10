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

int SPNG_update(UPDATE_FUNC_ARGS)
{
	int limit = 50;
	if (parts[i].life<limit && sim->air->pv[y/CELL][x/CELL]<=3&&sim->air->pv[y/CELL][x/CELL]>=-3&&parts[i].temp<=374.0f)
	{
		int absorbChanceDenom = parts[i].life*10000/limit + 500;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					switch (TYP(r))
					{
					case PT_WATR:
					case PT_DSTW:
					case PT_FRZW:
						if (parts[i].life<limit && RNG::Ref().chance(500, absorbChanceDenom))
						{
							parts[i].life++;
							sim->part_kill(ID(r));
						}
						break;
					case PT_SLTW:
						if (parts[i].life<limit && RNG::Ref().chance(50, absorbChanceDenom))
						{
							parts[i].life++;
							if (RNG::Ref().chance(3, 4))
								sim->part_kill(ID(r));
							else
								part_change_type(ID(r), x+rx, y+ry, PT_SALT);
						}
						break;
					case PT_CBNW:
						if (parts[i].life<limit && RNG::Ref().chance(100, absorbChanceDenom))
						{
							parts[i].life++;
							part_change_type(ID(r), x+rx, y+ry, PT_CO2);
						}
						break;
					case PT_PSTE:
						if (parts[i].life<limit && RNG::Ref().chance(20, absorbChanceDenom))
						{
							parts[i].life++;
							sim->part_create(ID(r), x+rx, y+ry, PT_CLST);
						}
						break;
					default:
						continue;
					}
				}
	}
	else
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if ((!r)&&parts[i].life>=1)//if nothing then create water
					{
						int np = sim->part_create(-1,x+rx,y+ry,PT_WATR);
						if (np>-1) parts[i].life--;
					}
				}
	for (int trade = 0; trade < 9; trade++)
	{
		int rx = RNG::Ref().between(-2, 2);
		int ry = RNG::Ref().between(-2, 2);
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r)==PT_SPNG&&(parts[i].life>parts[ID(r)].life)&&parts[i].life>0)//diffusion
			{
				int tmp = parts[i].life - parts[ID(r)].life;
				if (tmp ==1)
				{
					parts[ID(r)].life ++;
					parts[i].life --;
					trade = 9;
				}
				else if (tmp>0)
				{
					parts[ID(r)].life += tmp/2;
					parts[i].life -= tmp/2;
					trade = 9;
				}
			}
		}
	}
	int tmp = 0;
	if (parts[i].life>0)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r)==PT_FIRE)
					{
						tmp++;
						if (parts[ID(r)].life>60)
							parts[ID(r)].life -= parts[ID(r)].life/60;
						else if (parts[ID(r)].life>2)
							parts[ID(r)].life--;
					}
				}
	}
	if (tmp && parts[i].life>3)
		parts[i].life -= parts[i].life/3;
	if (tmp>1)
		tmp = tmp/2;
	if (tmp || parts[i].temp>=374)
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if ((!r)&&parts[i].life>=1)//if nothing then create steam
					{
						int np = sim->part_create(-1,x+rx,y+ry,PT_WTRV);
						if (np>-1)
						{
							parts[np].temp = parts[i].temp;
							tmp--;
							parts[i].life--;
							parts[i].temp = restrict_flt(parts[i].temp - 20.0f, MIN_TEMP, MAX_TEMP);
						}
					}
				}
	if (tmp>0)
	{
		if (parts[i].life>tmp)
			parts[i].life -= tmp;
		else
			parts[i].life = 0;
	}
	return 0;
}

int SPNG_graphics(GRAPHICS_FUNC_ARGS)
{
	*colr -= cpart->life*15;
	*colg -= cpart->life*15;
	*colb -= cpart->life*15;
	if (*colr<=50)
		*colr = 50;
	if (*colg<=50)
		*colg = 50;
	if (*colb<=20)
		*colb = 20;
	return 0;
}

void SPNG_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPNG";
	elem->Name = "SPNG";
	elem->Colour = COLPACK(0xFFBE30);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SOLIDS;
	elem->Enabled = 1;

	elem->Advection = 0.00f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.00f;
	elem->Loss = 1.00f;
	elem->Collision = 0.00f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f  * CFDS;
	elem->Falldown = 0;

	elem->Flammable = 20;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 31;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Sponge, absorbs water. Is a moving solid.";

	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 2730.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &SPNG_update;
	elem->Graphics = &SPNG_graphics;
	elem->Init = &SPNG_init_element;
}
