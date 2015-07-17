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

void attach(int i1, int i2)
{
	if (!(parts[i2].ctype&4))
	{
		parts[i1].ctype |= 2;
		parts[i1].tmp = i2;

		parts[i2].ctype |= 4;
		parts[i2].tmp2 = i1;
	}
	else if (!(parts[i2].ctype&2))
	{
		parts[i1].ctype |= 4;
		parts[i1].tmp2= i2;

		parts[i2].ctype |= 2;
		parts[i2].tmp = i1;
	}
}

void detach(int i)
{
	if ((parts[i].ctype&2) == 2)
	{
		if ((parts[parts[i].tmp].ctype&4) == 4 && parts[parts[i].tmp].type == PT_SOAP)
			parts[parts[i].tmp].ctype ^= 4;
	}

	if ((parts[i].ctype&4) == 4)
	{
		if ((parts[parts[i].tmp2].ctype&2) == 2 && parts[parts[i].tmp2].type == PT_SOAP)
			parts[parts[i].tmp2].ctype ^= 2;
	}

	parts[i].ctype = 0;
}

#define SOAP_FREEZING 248.15f

int SOAP_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, nr, ng, nb, na;
	float tr, tg, tb, ta;
	float blend;
	
	//0x01 - bubble on/off
	//0x02 - first mate yes/no
	//0x04 - "back" mate yes/no

	if (parts[i].ctype&1)
	{
		if (parts[i].temp>SOAP_FREEZING)
		{
			if (parts[i].tmp < 0 || parts[i].tmp >= NPART || parts[i].tmp2 < 0 || parts[i].tmp2 >= NPART)
			{
				parts[i].tmp = parts[i].tmp2 = parts[i].ctype = 0;
				return 0;
			}
			if (parts[i].life<=0)
			{
				//if only connected on one side
				if ((parts[i].ctype&6) != 6 && (parts[i].ctype&6))
				{
					int target = i;
					//break entire bubble in a loop
					while((parts[target].ctype&6) != 6 && (parts[target].ctype&6) && parts[target].type == PT_SOAP)
					{
						if (parts[target].ctype&2)
						{
							target = parts[target].tmp;
							detach(target);
						}
						if (parts[target].ctype&4)
						{
							target = parts[target].tmp2;
							detach(target);
						}
					}
				}
				if ((parts[i].ctype&6) != 6)
					parts[i].ctype = 0;
				if ((parts[i].ctype&6) == 6 && (parts[parts[i].tmp].ctype&6) == 6 && parts[parts[i].tmp].tmp == i)
					detach(i);
			}
			parts[i].vy = (parts[i].vy-0.1f)*0.5f;
			parts[i].vx *= 0.5f;
		}

		if (!(parts[i].ctype&2))
		{
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
					if (BOUNDS_CHECK && (rx || ry))
					{
						r = pmap[y+ry][x+rx];
						if (!r)
							continue;

						if ((parts[r>>8].type == PT_SOAP) && (parts[r>>8].ctype&1) && !(parts[r>>8].ctype&4))
							attach(i, r>>8);
					}
		}
		else
		{
			if (parts[i].life<=0)
				for (rx=-2; rx<3; rx++)
					for (ry=-2; ry<3; ry++)
						if (BOUNDS_CHECK && (rx || ry))
						{
							r = pmap[y+ry][x+rx];
							if (!r && !bmap[(y+ry)/CELL][(x+rx)/CELL])
								continue;

							if (parts[i].temp>SOAP_FREEZING)
							{
								if (bmap[(y+ry)/CELL][(x+rx)/CELL] ||
									(r && ptypes[r&0xFF].state != ST_GAS && (r&0xFF) != PT_SOAP && (r&0xFF) != PT_GLAS))
								{
									detach(i);
									continue;
								}
							}

							if ((r&0xFF) == PT_SOAP)
							{
								if (parts[r>>8].ctype == 1)
								{
									int buf = parts[i].tmp;

									parts[i].tmp = r>>8;
									if (parts[buf].type == PT_SOAP)
										parts[buf].tmp2 = r>>8;
									parts[r>>8].tmp2 = i;
									parts[r>>8].tmp = buf;
									parts[r>>8].ctype = 7;
								}
								else if (parts[r>>8].ctype == 7 && parts[i].tmp != r>>8 && parts[i].tmp2 != r>>8)
								{
									if (parts[parts[i].tmp].type == PT_SOAP)
										parts[parts[i].tmp].tmp2 = parts[r>>8].tmp2;
									if (parts[parts[r>>8].tmp2].type == PT_SOAP)
										parts[parts[r>>8].tmp2].tmp = parts[i].tmp;
									parts[r>>8].tmp2 = i;
									parts[i].tmp = r>>8;
								}
							}
						}
		}

		if(parts[i].ctype&2)
		{
			float dx = parts[i].x - parts[parts[i].tmp].x;
			float dy = parts[i].y - parts[parts[i].tmp].y;
			float d = 9/(pow(dx, 2)+pow(dy, 2)+9)-0.5f;

			parts[parts[i].tmp].vx -= dx*d;
			parts[parts[i].tmp].vy -= dy*d;
			parts[i].vx += dx*d;
			parts[i].vy += dy*d;

			if ((parts[parts[i].tmp].ctype&2) && (parts[parts[i].tmp].ctype&1) 
				&& (parts[parts[i].tmp].tmp >= 0 && parts[parts[i].tmp].tmp < NPART) 
				&& (parts[parts[parts[i].tmp].tmp].ctype&2) && (parts[parts[parts[i].tmp].tmp].ctype&1))
			{
				int ii = parts[parts[parts[i].tmp].tmp].tmp;
				if (ii >= 0 && ii < NPART)
				{
					dx = parts[ii].x - parts[parts[i].tmp].x;
					dy = parts[ii].y - parts[parts[i].tmp].y;
					d = 81/(pow(dx, 2)+pow(dy, 2)+81)-0.5f;

					parts[parts[i].tmp].vx -= dx*d*0.5f;
					parts[parts[i].tmp].vy -= dy*d*0.5f;
					parts[ii].vx += dx*d*0.5f;
					parts[ii].vy += dy*d*0.5f;
				}
			}
		}
	}
	else
	{
		if (pv[y/CELL][x/CELL]>0.5f || pv[y/CELL][x/CELL]<(-0.5f))
		{
			parts[i].ctype = 1;
			parts[i].life = 10;
		}
		for (rx=-2; rx<3; rx++)
			for (ry=-2; ry<3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;

					if ((r&0xFF) == PT_OIL)
					{
						parts[i].vx = parts[r>>8].vx = (parts[i].vx*0.5f + parts[r>>8].vx)/2;
						parts[i].vy = parts[r>>8].vy = ((parts[i].vy-0.1f)*0.5f + parts[r>>8].vy)/2;
					}
				}
	}
	
	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)!=PT_SOAP)
				{
					blend = 0.85f;
					ta = (float)COLA(parts[r>>8].dcolour);
					tr = (float)COLR(parts[r>>8].dcolour);
					tg = (float)COLG(parts[r>>8].dcolour);
					tb = (float)COLB(parts[r>>8].dcolour);
					
					na = (int)(ta*blend);
					nr = (int)(tr*blend);
					ng = (int)(tg*blend);
					nb = (int)(tb*blend);
					
					parts[r>>8].dcolour = COLARGB(na, nr, ng, nb);
				}
			}

	return 0;
}

void SOAP_ChangeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	if (from==PT_SOAP && to!=PT_SOAP)
	{
		detach(i);
	}
}

void SOAP_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SOAP";
	elem->Name = "SOAP";
	elem->Colour = COLPACK(0xF5F5DC);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_LIQUID;
	elem->Enabled = 1;

	elem->Advection = 0.6f;
	elem->AirDrag = 0.01f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.95f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.1f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 2;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 20;

	elem->Weight = 35;

	elem->DefaultProperties.temp = R_TEMP-2.0f	+273.15f;
	elem->HeatConduct = 29;
	elem->Latent = 0;
	elem->Description = "Soap. Creates bubbles, washes off deco colour, and cures virus.";

	elem->State = ST_LIQUID;
	elem->Properties = TYPE_LIQUID|PROP_NEUTPENETRATE|PROP_LIFE_DEC;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITL;
	elem->HighTemperatureTransitionElement = NT;

	elem->DefaultProperties.tmp = -1;
	elem->DefaultProperties.tmp2 = -1;

	elem->Update = &SOAP_update;
	elem->Graphics = NULL;
	elem->Func_ChangeType = &SOAP_ChangeType;
	elem->Init = &SOAP_init_element;
}
