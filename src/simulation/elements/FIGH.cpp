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

int FIGH_update(UPDATE_FUNC_ARGS)
{
	playerst* figh = &fighters[(unsigned char)parts[i].tmp];

	unsigned int tarx, tary;

	parts[i].tmp2 = 0; //0 - stay in place, 1 - seek a stick man

	//Set target coords
	if (player.spwn && player2.spwn)
	{
			if ((pow(player.legs[2]-x, 2) + pow(player.legs[3]-y, 2))<=
					(pow(player2.legs[2]-x, 2) + pow(player2.legs[3]-y, 2)))
			{
				tarx = (unsigned int)player.legs[2];
				tary = (unsigned int)player.legs[3];
			}
			else
			{
				tarx = (unsigned int)player2.legs[2];
				tary = (unsigned int)player2.legs[3];
			}
			parts[i].tmp2 = 1;
	}
	else
	{
		if (player.spwn)
		{
			tarx = (unsigned int)player.legs[2];
			tary = (unsigned int)player.legs[3];
			parts[i].tmp2 = 1;
		}
		if (player2.spwn)
		{
			tarx = (unsigned int)player2.legs[2];
			tary = (unsigned int)player2.legs[3];
			parts[i].tmp2 = 1;
		}
	}

	switch (parts[i].tmp2)
	{
		case 1:
			if ((pow(tarx-x, 2) + pow(tary-y, 2))<600)
			{
				if (figh->elem == PT_LIGH || figh->elem == PT_NEUT 
						|| ptypes[figh->elem].properties&(PROP_DEADLY|PROP_RADIOACTIVE) 
						|| ptypes[figh->elem].heat>=323 || ptypes[figh->elem].heat<=243)
					figh->comm = (int)figh->comm | 0x08;
			}
			else
				if (tarx<x)
				{
					if(figh->rocketBoots || !(eval_move(PT_FIGH, (int)figh->legs[4]-10, (int)figh->legs[5]+6, NULL) 
					  && eval_move(PT_FIGH, (int)figh->legs[4]-10, (int)figh->legs[5]+3, NULL)))
						figh->comm = 0x01;
					else
						figh->comm = 0x02;

					if (figh->rocketBoots)
					{
						if (tary<y)
							figh->comm = (int)figh->comm | 0x04;
					}
					else if (!eval_move(PT_FIGH, (int)figh->legs[4]-4, (int)figh->legs[5]-1, NULL) 
							|| !eval_move(PT_FIGH, (int)figh->legs[12]-4, (int)figh->legs[13]-1, NULL)
							|| eval_move(PT_FIGH, (int)(2*figh->legs[4]-figh->legs[6]), (int)figh->legs[5]+5, NULL))
						figh->comm = (int)figh->comm | 0x04;
				}
				else
				{
					if (figh->rocketBoots || !(eval_move(PT_FIGH, (int)figh->legs[12]+10, (int)figh->legs[13]+6, NULL)
					   && eval_move(PT_FIGH, (int)figh->legs[12]+10, (int)figh->legs[13]+3, NULL)))
						figh->comm = 0x02;
					else
						figh->comm = 0x01;

					if (figh->rocketBoots)
					{
						if (tary<y)
							figh->comm = (int)figh->comm | 0x04;
					}
					else if (!eval_move(PT_FIGH, (int)figh->legs[4]+4, (int)figh->legs[5]-1, NULL) 
							|| !eval_move(PT_FIGH, (int)figh->legs[4]+4, (int)figh->legs[5]-1, NULL)
							|| eval_move(PT_FIGH, (int)(2*figh->legs[12]-figh->legs[14]), (int)figh->legs[13]+5, NULL))
						figh->comm = (int)figh->comm | 0x04;
				}
			break;
		default:
			figh->comm = 0;
			break;
	}

	figh->pcomm = figh->comm;

	run_stickman(figh, UPDATE_FUNC_SUBCALL_ARGS);
	return 0;
}

void FIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_FIGH";
	elem->Name = "FIGH";
	elem->Colour = COLPACK(0xFFE0A0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_SPECIAL;
	elem->Enabled = 1;

	elem->Advection = 0.5f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.2f;
	elem->Loss = 1.0f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.0f;
	elem->PressureAdd_NoAmbHeat = 0.00f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 0;

	elem->Weight = 50;

	elem->CreationTemperature = R_TEMP+14.6f+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "Fighter. Tries to kill stickmen. You must first give it an element to kill him with.";

	elem->State = ST_NONE;
	elem->Properties = PROP_NOCTYPEDRAW;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 620.0f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &FIGH_update;
	elem->Graphics = &graphics_STKM;
}