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
#include "EMP.h"
#include "ETRD.h"

int NPTCT_update(UPDATE_FUNC_ARGS);
int FIRE_update(UPDATE_FUNC_ARGS);

int SPRK_update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry, pavg, ct = parts[i].ctype, sender, receiver;
	FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);

	if (parts[i].life<=0)
	{
		if (ct==PT_WATR||ct==PT_SLTW||ct==PT_PSCN||ct==PT_NSCN||ct==PT_ETRD||ct==PT_INWR)
			parts[i].temp = R_TEMP + 273.15f;
		if (ct<=0 || ct>=PT_NUM || !sim->elements[ct].Enabled)
			ct = PT_METL;
		parts[i].ctype = PT_NONE;
		parts[i].life = 4;
		if (ct == PT_WATR)
			parts[i].life = 64;
		else if (ct == PT_SLTW)
			parts[i].life = 54;
#ifdef NOMOD
		else if (ct == PT_SWCH)
#else
		else if (ct == PT_SWCH || ct == PT_BUTN)
#endif
			parts[i].life = 14;
		else if (ct == PT_RSST) //RSST disappears at the end of its spark cycle
		{
			sim->part_kill(i);
			return 1;
		}
		part_change_type(i,x,y,ct);
		return 0;
	}

	//Some functions of SPRK based on ctype (what it is on)
	switch (ct)
	{
	case PT_SPRK:
		sim->part_kill(i);
		return 1;
	case PT_NTCT:
	case PT_PTCT:
		NPTCT_update(UPDATE_FUNC_SUBCALL_ARGS);
		break;
	case PT_ETRD:
		if (parts[i].life == 1)
		{
			int nearp = nearestSparkablePart(sim, i);
			if (nearp == -1)
				break;
			int pavg = parts_avg(i, nearp, PT_INSL);
			if (pavg != PT_INSL && pavg != PT_RSSS)
			{
				sim->CreateLine(x, y, (int)(parts[nearp].x+0.5f), (int)(parts[nearp].y+0.5f), PT_PLSM, 0);
				parts[i].life = 20;
				part_change_type(i, x, y, ct);
				ct = parts[i].ctype = PT_NONE;
				sim->spark_conductive(nearp, (int)(parts[nearp].x+0.5f),(int)(parts[nearp].y+0.5f));
				parts[nearp].life = 9;
			}
		}
		break;
	case PT_NBLE:
		if (parts[i].life <= 1 && !(parts[i].tmp&0x1))
		{
			parts[i].life = RNG::Ref().between(50, 199);
			part_change_type(i, x, y, PT_PLSM);
			parts[i].ctype = PT_NBLE;
			if (parts[i].temp > 5273.15)
				parts[i].tmp |= 0x4;
			parts[i].temp = 3500;
			sim->air->pv[y/CELL][x/CELL] += 1;
		}
		break;
	case PT_TESC:
		if (parts[i].tmp>300)
			parts[i].tmp=300;
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					r = pmap[y+ry][x+rx];
					if (r)
						continue;
					if (parts[i].tmp> 4 && RNG::Ref().chance(1, parts[i].tmp * parts[i].tmp / 20 + 6))
					{
						int p=sim->part_create(-1, x+rx*2, y+ry*2, PT_LIGH);
						if (p!=-1)
						{
							parts[p].life = RNG::Ref().between(0, (1 + parts[i].tmp / 15) + parts[i].tmp / 7);
							if (parts[i].life>60)
								parts[i].life=60;
							parts[p].temp=parts[p].life*parts[i].tmp/2.5f;
							parts[p].tmp2=1;
							parts[p].tmp=(int)(atan2((float)-ry, (float)rx)/M_PI*360);
							parts[p].dcolour = parts[i].dcolour;
							parts[i].temp = restrict_flt(parts[i].temp - (parts[i].tmp * 2 + parts[i].temp / 5), MIN_TEMP, MAX_TEMP); // slight self-cooling
							if (fabs(sim->air->pv[y/CELL][x/CELL])!=0.0f)
							{
								if (fabs(sim->air->pv[y/CELL][x/CELL])<=0.5f)
									sim->air->pv[y/CELL][x/CELL] = 0;
								else
									sim->air->pv[y/CELL][x/CELL] -= (sim->air->pv[y/CELL][x/CELL]>0) ? 0.5f : -0.5f;
							}
						}
					}
				}
		break;
	case PT_IRON:
		for (int rx = -1; rx <= 1; rx++)
			for (int ry = -1; ry <= 1; ry++)
				if (rx || ry)
				{
					r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					if (TYP(r) == PT_DSTW || TYP(r) == PT_SLTW || (TYP(r) == PT_WATR))
					{
						int rnd = RNG::Ref().between(0, 99);
						if (!rnd)
							part_change_type(ID(r), x+rx, y+ry, PT_O2);
						else if (3 > rnd)
							part_change_type(ID(r), x+rx, y+ry, PT_H2);
					}
				}
		break;
	case PT_TUNG:
		if (parts[i].temp < 3595.0)
			parts[i].temp += RNG::Ref().between(-4, 15);
		break;
	default:
		break;
	}
	for (rx=-2; rx<=2; rx++)
		for (ry=-2; ry<=2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				//receiver is the element SPRK is trying to conduct to
				//sender is the element the SPRK is on
				//pavg is the element in the middle of them both
				receiver = TYP(r);
				sender = ct;
				pavg = parts_avg(ID(r), i, PT_INSL);

				//First, some checks usually for (de)activation of elements
				switch (receiver)
				{
				case PT_SWCH:
#ifndef NOMOD
				case PT_BUTN:
#endif
					// make sparked SWCH and BUTN turn off correctly
					if (!sim->instantActivation && pavg != PT_INSL && pavg != PT_RSSS && parts[i].life < 4)
					{
						if (sender == PT_PSCN && parts[ID(r)].life<10)
						{
							parts[ID(r)].life = 10;
						}
						else if (sender == PT_NSCN)
						{
							parts[ID(r)].ctype = PT_NONE;
							parts[ID(r)].life = 9;
						}
					}
					break;
				case PT_SPRK:
					if (pavg != PT_INSL && pavg != PT_RSSS && parts[i].life < 4)
					{
#ifdef NOMOD
						if (parts[ID(r)].ctype == PT_SWCH)
#else
						if (parts[ID(r)].ctype == PT_SWCH || parts[ID(r)].ctype == PT_BUTN)
#endif
						{
							if (sender == PT_NSCN)
							{
								part_change_type(ID(r), x+rx, y+ry, parts[ID(r)].ctype);
								parts[ID(r)].ctype = PT_NONE;
								parts[ID(r)].life = 9;
							}
						}
						else if (parts[ID(r)].ctype==PT_NTCT || parts[ID(r)].ctype==PT_PTCT)
						{
							if (sender == PT_METL)
								parts[ID(r)].temp = 473.0f;
						}
					}
					break;
				case PT_PUMP:
				case PT_GPMP:
				case PT_HSWC:
				case PT_PBCN:
					if (!sim->instantActivation && parts[i].life < 4) // PROP_PTOGGLE, Maybe? We seem to use 2 different methods for handling actived elements, this one seems better. Yes, use this one
					{
						if (sender==PT_PSCN)
							parts[ID(r)].life = 10;
						else if (sender==PT_NSCN && parts[ID(r)].life>=10)
							parts[ID(r)].life = 9;
					}
					break;
				case PT_LCRY:
					if (abs(rx) < 2 && abs(ry) < 2 && parts[i].life < 4)
					{
						if (sender==PT_PSCN && parts[ID(r)].tmp == 0)
							parts[ID(r)].tmp = 2;
						else if (sender==PT_NSCN && parts[ID(r)].tmp == 3)
							parts[ID(r)].tmp = 1;
					}
					break;
				case PT_PPIP:
					if (parts[i].life == 3 && pavg!=PT_INSL && pavg != PT_RSSS)
					{
						if (sender == PT_NSCN || sender == PT_PSCN || sender == PT_INST)
							PPIP_flood_trigger(sim, x+rx, y+ry, sender);
					}
					break;
				case PT_NTCT:
				case PT_PTCT:
				case PT_INWR:
					if (sender==PT_METL && pavg!=PT_INSL && pavg != PT_RSSS && parts[i].life<4)
					{
						parts[ID(r)].temp = 473.0f;
						if (receiver==PT_NTCT || receiver==PT_PTCT)
							continue;
					}
					break;
				case PT_EMP:
					if (!parts[ID(r)].life && parts[i].life < 4)
					{
						static_cast<EMP_ElementDataContainer&>(*sim->elementData[PT_EMP]).Activate();
						parts[ID(r)].life = 220;
						break;
					}
				default:
					break;
				}

				if (pavg == PT_INSL || pavg == PT_RSSS) //Insulation blocks everything past here
					continue;
				if (!((sim->elements[receiver].Properties&PROP_CONDUCTS) || receiver==PT_INST || receiver==PT_QRTZ)) //Stop non-conducting receivers, allow INST and QRTZ as special cases
					continue;
				if (abs(rx)+abs(ry)>=4 && receiver!=PT_SWCH && sender!=PT_SWCH) //Only SWCH conducts really far
					continue;
				if (receiver == sender && receiver != PT_INST && receiver != PT_QRTZ) //Everything conducts to itself, except INST and QRTZ
					goto conduct;

				//Sender cases, where elements can have specific outputs
				switch (sender)
				{
				case PT_INST:
					if (receiver == PT_NSCN)
						goto conduct;
					continue;
				case PT_SWCH:
#ifndef NOMOD
				case PT_BUTN:
#endif
					if (receiver==PT_PSCN || receiver==PT_NSCN || receiver==PT_WATR || receiver==PT_SLTW || receiver==PT_NTCT || receiver==PT_PTCT || receiver==PT_INWR)
						continue;
					break;
				case PT_ETRD:
					if (receiver==PT_METL || receiver==PT_BMTL || receiver==PT_BRMT || receiver==PT_LRBD || receiver==PT_RBDM || receiver==PT_PSCN || receiver==PT_NSCN)
						goto conduct;
					continue;
				case PT_NTCT:
					if (receiver==PT_PSCN || (receiver==PT_NSCN && parts[i].temp>373.0f))
						goto conduct;
					continue;
				case PT_PTCT:
					if (receiver==PT_PSCN || (receiver==PT_NSCN && parts[i].temp<373.0f))
						goto conduct;
					continue;
				case PT_INWR:
					if (receiver==PT_NSCN || receiver==PT_PSCN)
						goto conduct;
					continue;
				default:
					break;
				}

				//Receiving cases, where elements can have specific inputs
				switch (receiver)
				{
				case PT_QRTZ:
					if ((sender==PT_NSCN||sender==PT_METL||sender==PT_PSCN||sender==PT_QRTZ) && (parts[ID(r)].temp<173.15||sim->air->pv[(y+ry)/CELL][(x+rx)/CELL]>8))
						goto conduct;
					continue;
				case PT_NTCT:
					if (sender==PT_NSCN || (sender==PT_PSCN && parts[ID(r)].temp>373.0f))
						goto conduct;
					continue;
				case PT_PTCT:
					if (sender==PT_NSCN || (sender==PT_PSCN && parts[ID(r)].temp<373.0f))
						goto conduct;
					continue;
				case PT_INWR:
					if (sender==PT_NSCN || sender==PT_PSCN)
						goto conduct;
					continue;
				case PT_INST:
					if (sender == PT_PSCN)
						goto conduct;
					continue;
				case PT_NBLE:
					if (!(parts[i].tmp&0x1))
						goto conduct;
					continue;
				case PT_PSCN:
					if (sender != PT_NSCN)
						goto conduct;
					continue;
				default:
					break;
				}

conduct:
				//Passed normal conduction rules, check a few last things and change receiver to spark
				if (receiver==PT_WATR || receiver==PT_SLTW)
				{
					if (parts[ID(r)].life==0 && parts[i].life<3)
					{
						part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
						if (receiver == PT_WATR)
							parts[ID(r)].life = 6;
						else
							parts[ID(r)].life = 5;
						parts[ID(r)].ctype = receiver;
					}
				}
				else if (receiver == PT_INST)
				{
					if (parts[ID(r)].life==0 && parts[i].life<4)
					{
						INST_flood_spark(sim, x+rx, y+ry);
					}
				}
				else if (receiver == PT_RSST)
				{
					if (parts[ID(r)].life==0 && parts[i].life<4)
					{
						sim->part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
						parts[ID(r)].life = 5;
						parts[ID(r)].ctype = receiver;
					}
				}
				else if (parts[ID(r)].life==0 && parts[i].life<4)
				{
					sim->spark_conductive(ID(r), x+rx, y+ry);
				}
				else if (!parts[ID(r)].life && sender==PT_ETRD && parts[i].life==5) //ETRD is odd and conducts to others only at life 5, this could probably be somewhere else
				{
					part_change_type(i,x,y,sender);
					parts[i].ctype = PT_NONE;
					parts[i].life = 20;
					sim->spark_conductive(ID(r), x+rx, y+ry);
				}
			}
	return 0;
}

int SPRK_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 60;
	
	*firer = *colr/2;
	*fireg = *colg/2;
	*fireb = *colb/2;
	*pixel_mode |= FIRE_SPARK;
	return 1;
}

void SPRK_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SPRK";
	elem->Name = "SPRK";
	elem->Colour = COLPACK(0xFFFF80);
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
	elem->HotAir = 0.001f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;
	elem->PhotonReflectWavelengths = 0x00000000;

	elem->Weight = 100;

	elem->HeatConduct = 251;
	elem->Latent = 0;
	elem->Description = "Electricity. The basis of all electronics in TPT, travels along conductive elements.";

	elem->Properties = TYPE_SOLID|PROP_LIFE_DEC;
	elem->CarriesTypeIn = 1U << FIELD_CTYPE;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &SPRK_update;
	elem->Graphics = &SPRK_graphics;
	elem->Init = &SPRK_init_element;
}
