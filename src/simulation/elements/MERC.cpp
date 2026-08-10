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

int MERC_update(UPDATE_FUNC_ARGS)
{
	// Max number of particles that can be condensed into one
	const int absorbScale = 10000;
	parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
	int maxtmp = ((absorbScale/(parts[i].temp + 1))-1);
	int it1 = int(parts[i].temp) + 1;
	if (it1 < 1)
		it1 = 1;
	if (RNG::Ref().chance(absorbScale % it1, it1))
		maxtmp++;
	if (parts[i].tmp < 0)
		parts[i].tmp = 0;
	if (parts[i].tmp > absorbScale)
		parts[i].tmp = absorbScale;

	if (parts[i].tmp < maxtmp)
	{
		for (int rx = -1; rx <= 1; rx++)
			for (int ry=-1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (!r || (parts[i].tmp >= maxtmp))
						continue;
					if (TYP(r)==PT_MERC && RNG::Ref().chance(1, 3))
					{
						if ((parts[i].tmp + parts[ID(r)].tmp + 1) <= maxtmp)
						{
							parts[i].tmp += parts[ID(r)].tmp + 1;
							sim->part_kill(ID(r));
						}
					}
				}
	}
	else
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					int r = pmap[y+ry][x+rx];
					if (parts[i].tmp <= maxtmp)
						continue;
					//if nothing then create deut
					if ((!r) && parts[i].tmp >= 1)
					{
						int np = sim->part_create(-1,x+rx,y+ry,PT_MERC);
						if (np < 0)
							continue;
						parts[i].tmp--;
						parts[np].temp = parts[i].temp;
						parts[np].tmp = 0;
					}
				}
	for (int trade = 0; trade < 4; trade ++)
	{
		int rx = RNG::Ref().between(-2, 2);
		int ry = RNG::Ref().between(-2, 2);
		if (rx || ry)
		{
			int r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			//diffusion
			if (TYP(r) == PT_MERC && parts[i].tmp > parts[ID(r)].tmp && parts[i].tmp > 0)
			{
				int temp = parts[i].tmp - parts[ID(r)].tmp;
				if (temp == 1)
				{
					parts[ID(r)].tmp ++;
					parts[i].tmp --;
				}
				else if (temp > 0)
				{
					parts[ID(r)].tmp += temp/2;
					parts[i].tmp -= temp/2;
				}
			}
		}
	}
	return 0;
}

void MERC_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_MERC";
	elem->Name = "MERC";
	elem->Colour = COLPACK(0x736B6D);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.4f;
	elem->AirDrag = 0.04f * CFDS;
	elem->AirLoss = 0.94f;
	elem->Loss = 0.80f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.3f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 18;

	elem->Weight = 91;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Mercury. Volume changes with temperature, Conductive.";

	elem->Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_NEUTABSORB|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = 10;

	elem->Update = &MERC_update;
	elem->Graphics = NULL;
	elem->Init = &MERC_init_element;
}
