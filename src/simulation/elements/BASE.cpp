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

int BASE_update(UPDATE_FUNC_ARGS)
{
	//Reset spark effect
	parts[i].tmp = 0;

	if (parts[i].life < 1)
		parts[i].life = 1;

	if (parts[i].life > 100)
		parts[i].life = 100;

	float pres = sim->air->pv[y/CELL][x/CELL];

	//Base evaporates into BOYL or increases concentration
	if (parts[i].life < 100 && pres < 10.0f && parts[i].temp > (120.0f + 273.15f))
	{
		//Slow down boiling
		if (RNG::Ref().chance(1, 20))
		{
			//This way we preserve the total amount of concentrated BASE in the solution
			if (RNG::Ref().chance(1, parts[i].life+1))
			{
				sim->part_create_preserve_energy(i, x, y, PT_BOYL);
				return 1;
			}
			else
			{
				parts[i].life++;
				//Enthalpy of vaporization
				parts[i].temp -= 20.0f / ((float)parts[i].life);
			}
		}
	}

	//Base's freezing point lowers with its concentration
	if (parts[i].temp < (273.15f - ((float)parts[i].life) / 4.0f))
	{
		//We don't save base's concentration, so ICEI(BASE) will unfreeze into life = 0
		sim->part_change_type(i, x, y, PT_ICEI);
		parts[i].ctype = PT_BASE;
		parts[i].life = 0;
		return 1;
	}

	//Reactions
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y + ry][x + rx];
				if (!r)
					continue;
				int rt = TYP(r);

				//Don't react with some elements
				if (rt != PT_BASE && rt != PT_SALT && rt != PT_SLTW && rt != PT_BOYL && rt != PT_MERC &&
					rt != PT_BMTL && rt != PT_BRMT && rt != PT_SOAP && rt != PT_CLNE && rt != PT_PCLN &&
					!(rt == PT_ICEI && parts[ID(r)].ctype == PT_BASE) && !(rt == PT_SNOW && parts[ID(r)].ctype == PT_BASE) &&
					!(rt == PT_SPRK && parts[ID(r)].ctype == PT_BMTL) && !(rt == PT_SPRK && parts[ID(r)].ctype == PT_BRMT))
				{
					//Base is diluted by water
					if (parts[i].life > 1 && (rt == PT_WATR || rt == PT_DSTW || rt == PT_CBNW))
					{
						if (RNG::Ref().chance(1, 20))
						{
							int saturh = parts[i].life / 2;

							sim->part_create_preserve_energy(ID(r), x + rx, y + ry, PT_BASE);
							parts[ID(r)].life = saturh;
							parts[ID(r)].temp += ((float)saturh) / 10.0f;
							parts[i].life -= saturh;
						}
					} // Base neutralizes acid
					else if (rt == PT_ACID)
					{
						if (parts[ID(r)].life > 50 && parts[i].life > 0)
						{
							parts[ID(r)].life--;
							parts[i].life--;
						}

						if (parts[ID(r)].life <= 50)
							sim->part_create_preserve_energy(ID(r), x+rx, y+ry, PT_SLTW);
						if (parts[i].life <= 0)
						{
							sim->part_create_preserve_energy(i, x, y, PT_SLTW);
							return 1;
						}
					} // Base neutralizes CAUS
					else if (rt == PT_CAUS)
					{
						if (parts[ID(r)].life > 50 && parts[i].life > 0)
						{
							parts[ID(r)].life--;
							parts[i].life--;
						}

						if (parts[ID(r)].life <= 50)
							sim->part_kill(ID(r));
						if (parts[i].life <= 0)
						{
							sim->part_create_preserve_energy(i, x, y, PT_SLTW);
							return 1;
						}
					} // BASE + OIL = SOAP
					else if (parts[i].life >= 70 && rt == PT_OIL)
					{
						sim->part_create_preserve_energy(i, x, y, PT_SOAP);
						sim->part_kill(ID(r));
						return 1;
					} // BASE + GOO = GEL
					else if (parts[i].life > 1 && rt == PT_GOO)
					{
						sim->part_create_preserve_energy(ID(r), x + rx, y + ry, PT_GEL);
						parts[i].life--;
					} // BASE + BCOL = GUNP
					else if (parts[i].life > 1 && rt == PT_BCOL)
					{
						sim->part_create_preserve_energy(ID(r), x + rx, y + ry, PT_GUNP);
						parts[i].life--;
					} // BASE + Molden ROCK = MERC
					else if (rt == PT_LAVA && parts[ID(r)].ctype == PT_ROCK && pres >= 10.0f && RNG::Ref().chance(1, 1000))
					{
						sim->part_create_preserve_energy(i, x, y, PT_MERC);
						sim->part_kill(ID(r));
						return 1;
					} // Base rusts conductive solids
					else if (parts[i].life >= 10 &&
							 (sim->elements[rt].Properties & (TYPE_SOLID|PROP_CONDUCTS)) == (TYPE_SOLID|PROP_CONDUCTS) && RNG::Ref().chance(1, 10))
					{
						sim->part_create_preserve_energy(ID(r), x + rx, y + ry, PT_BMTL);
						parts[ID(r)].tmp = RNG::Ref().between(20, 29);
						parts[i].life--;
						//Draw a spark effect
						parts[i].tmp = 1;
					} // Base destroys a substance slower if acid destroys it faster
					else if (sim->elements[rt].Hardness > 0 && sim->elements[rt].Hardness < 50 &&
							 parts[i].life >= (2 * sim->elements[rt].Hardness) && RNG::Ref().chance(50 - sim->elements[rt].Hardness, 1000))
					{
						sim->part_kill(ID(r));
						parts[i].life -= 2;
						// Draw a spark
						parts[i].tmp = 1;
					}
				}
			}
		}
	}

	//Diffusion
	for (auto trade = 0; trade<2; trade++)
	{
		auto rx = RNG::Ref().between(-1, 1);
		auto ry = RNG::Ref().between(-1, 1);
		if (rx || ry)
		{
			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			if (TYP(r) == PT_BASE && (parts[i].life > parts[ID(r)].life) && parts[i].life > 1)
			{
				int temp = parts[i].life - parts[ID(r)].life;
				if (temp == 1)
				{
					parts[ID(r)].life++;
					parts[i].life--;
				}
				else if (temp>0)
				{
					parts[ID(r)].life += temp / 2;
					parts[i].life -= temp / 2;
				}
			}
		}
	}
	return 0;
}

int BASE_graphics(GRAPHICS_FUNC_ARGS)
{
	int s = cpart->life;

	if (s <= 25)
	{
		*colr = 0x33;
		*colg = 0x4C;
		*colb = 0xD8;
	}
	else if (s <= 50)
	{
		*colr = 0x58;
		*colg = 0x83;
		*colb = 0xE8;
	}
	else if (s <= 75)
	{
		*colr = 0x7D;
		*colg = 0xBA;
		*colb = 0xF7;
	}

	*pixel_mode |= PMODE_BLUR;

	if (cpart->tmp == 1)
		*pixel_mode |= PMODE_SPARK;

	return 0;
}

void BASE_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_BASE";
	elem->Name = "BASE";
	elem->Colour = COLPACK(0x90D5FF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;
	
	elem->Advection = 0.5f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.97f;
	elem->Loss = 0.96f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.08f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 16;

	elem->HeatConduct = 31;
	elem->HeatCapacity = 1.5f;
	elem->Latent = 0;
	elem->Description = "Corrosive liquid. Rusts conductive solids, neutralizes acid.";

	elem->Properties = TYPE_LIQUID|PROP_DEADLY;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 76;

	elem->Update = &BASE_update;
	elem->Graphics = &BASE_graphics;
	elem->Init = &BASE_init_element;
}
