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

int FIRE_update(UPDATE_FUNC_ARGS);

int PHOT_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	float rr, rrr;
	if (!(parts[i].ctype&0x3FFFFFFF))
	{
		kill_part(i);
		return 1;
	}
	if (parts[i].temp > 506.0f)
		if (!(rand()%10)) FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);

	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)==PT_ISOZ || (r&0xFF)==PT_ISZS)
				{
					if (!(rand()%400))
					{
						parts[i].vx *= 0.90f;
						parts[i].vy *= 0.90f;
						sim->part_create(r>>8, x+rx, y+ry, PT_PHOT);
						rrr = (rand()%360)*M_PI/180.0f;
						if ((r&0xFF) == PT_ISOZ)
							rr = (rand()%128+128)/127.0f;
						else
							rr = (rand()%228+128)/127.0f;
						parts[r>>8].vx = rr*cosf(rrr);
						parts[r>>8].vy = rr*sinf(rrr);
						pv[y/CELL][x/CELL] -= 15.0f * CFDS;
					}
				}
				else if ((r&0xFF) == PT_QRTZ)
				{
					if (!ry && !rx)
					{
						float a = (rand()%360)*M_PI/180.0f;
						parts[i].vx = 3.0f*cosf(a);
						parts[i].vy = 3.0f*sinf(a);
						if (parts[i].ctype == 0x3FFFFFFF)
							parts[i].ctype = 0x1F<<(rand()%26);
						parts[i].life++; //Delay death
					}
				}
				else if ((r&0xFF) == PT_FILT)
				{
					if (parts[r>>8].tmp == 9)
					{
						parts[i].vx += ((float)(rand()%1000-500))/1000.0f;
						parts[i].vy += ((float)(rand()%1000-500))/1000.0f;
					}
				}
			}

	return 0;
}

int PHOT_graphics(GRAPHICS_FUNC_ARGS)
{
	int x = 0;
	*colr = *colg = *colb = 0;
	for (x=0; x<12; x++) {
		*colr += (cpart->ctype >> (x+18)) & 1;
		*colb += (cpart->ctype >>  x)     & 1;
	}
	for (x=0; x<12; x++)
		*colg += (cpart->ctype >> (x+9))  & 1;
	x = 624/(*colr+*colg+*colb+1);
	*colr *= x;
	*colg *= x;
	*colb *= x;

	*firea = 100;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
	if (cpart->flags & FLAG_PHOTDECO)
	{
		*pixel_mode &= ~NO_DECO;
	}
	return 0;
}

void PHOT_create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = (rand()%8) * 0.78540f;
	sim->parts[i].vx = 3.0f*cosf(a);
	sim->parts[i].vy = 3.0f*sinf(a);
	if ((pmap[y][x]&0xFF) == PT_FILT)
		parts[i].ctype = interactWavelengths(&parts[pmap[y][x]>>8], parts[i].ctype);
}

void PHOT_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_PHOT";
	elem->Name = "PHOT";
	elem->Colour = COLPACK(0xFFFFFF);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_NUCLEAR;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 1.00f;
	elem->Loss = 1.00f;
	elem->Collision = -0.99f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = -1;

	elem->DefaultProperties.temp = R_TEMP+900.0f+273.15f;
	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Photons. Refracts through glass, scattered by quartz, and colour-changed by different elements. Ignites flammable materials.";

	elem->State = ST_GAS;
	elem->Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.life = 680;
	elem->DefaultProperties.ctype = 0x3FFFFFFF;

	elem->Update = &PHOT_update;
	elem->Graphics = &PHOT_graphics;
	elem->Func_Create = &PHOT_create;
	elem->Init = &PHOT_init_element;
}
