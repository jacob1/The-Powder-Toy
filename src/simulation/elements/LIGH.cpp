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

int LIGH_update(UPDATE_FUNC_ARGS)
{
	/*
	 *
	 * tmp2:
	 * -1 - part will be removed
	 * 0 - "branches" of the lightning
	 * 1 - bending
	 * 2 - branching
	 * 3 - transfer spark or make destruction
	 * 4 - first pixel
	 *
	 * life - "thickness" of lighting (but anyway one pixel)
	 *
	 * tmp - angle of lighting, measured in degrees anticlockwise from the positive x direction
	 *
	*/
	int r,rx,ry, multipler, powderful=(int)(parts[i].temp*(1+parts[i].life/40)*LIGHTING_POWER), near, voidnearby = 0;
	float angle, angle2=-1;
	update_PYRO(UPDATE_FUNC_SUBCALL_ARGS);
	if (aheat_enable)
	{
		hv[y/CELL][x/CELL]+=powderful/50;
		if (hv[y/CELL][x/CELL]>MAX_TEMP)
			hv[y/CELL][x/CELL]=MAX_TEMP;
	}

	for (rx=-2; rx<3; rx++)
		for (ry=-2; ry<3; ry++)
			if (BOUNDS_CHECK && (rx || ry))
			{
				r = ((pmap[y+ry][x+rx]&0xFF)==PT_PINV&&parts[pmap[y+ry][x+rx]>>8].life==10)?0:pmap[y+ry][x+rx];
				if (!r)
					continue;
				if ((r&0xFF)!=PT_LIGH && (r&0xFF)!=PT_TESC)
				{
					if ((r&0xFF)!=PT_THDR&&!(ptypes[r&0xFF].properties&PROP_INDESTRUCTIBLE)&&(r&0xFF)!=PT_FIRE&&(r&0xFF)!=PT_NEUT&&(r&0xFF)!=PT_PHOT)
					{
						if ((ptypes[r&0xFF].properties&PROP_CONDUCTS) && parts[r>>8].life==0)
						{
							create_part(r>>8,x+rx,y+ry,PT_SPRK);
						}
						pv[y/CELL][x/CELL] += powderful/400;
						if (ptypes[r&0xFF].hconduct) parts[r>>8].temp = restrict_flt(parts[r>>8].temp+powderful/1.5, MIN_TEMP, MAX_TEMP);
					}
					if ((r&0xFF)==PT_DEUT || (r&0xFF)==PT_PLUT) // start nuclear reactions
					{
						parts[r>>8].temp = restrict_flt(parts[r>>8].temp+powderful, MIN_TEMP, MAX_TEMP);
						pv[y/CELL][x/CELL] +=powderful/35;
						if (rand()%3==0)
						{
							part_change_type(r>>8,x+rx,y+ry,PT_NEUT);
							parts[r>>8].life = rand()%480+480;
							parts[r>>8].vx=rand()%10-5.0f;
							parts[r>>8].vy=rand()%10-5.0f;
						}
					}
					if ((r&0xFF)==PT_COAL || (r&0xFF)==PT_BCOL) // ignite coal
					{
						if (parts[r>>8].life>100) {
							parts[r>>8].life = 99;
						}
					}
					if (ptypes[r&0xFF].hconduct)
						parts[r>>8].temp = restrict_flt(parts[r>>8].temp+powderful/10, MIN_TEMP, MAX_TEMP);
					if (((r&0xFF)==PT_STKM && player.elem!=PT_LIGH) || ((r&0xFF)==PT_STKM2 && player2.elem!=PT_LIGH))
					{
						parts[r>>8].life-=powderful/100;
					}
					if ((((r&0xFF)==PT_VOID || ((r&0xFF)==PT_PVOD && parts[r>>8].life >= 10)) && (!parts[r>>8].ctype || (parts[r>>8].ctype==PT_LIGH)!=(parts[r>>8].tmp&1))) || (r&0xFF)==PT_BHOL || (r&0xFF)==PT_NBHL) // VOID, PVOD, VACU, and BHOL eat LIGH here
					{
						voidnearby = 1;
					}
				}
			}
	if (parts[i].tmp2==3)
	{
		parts[i].tmp2=0;
		return 1;
	}

	if (parts[i].tmp2==-1)
	{
		kill_part(i);
		return 1;
	}
	if (parts[i].tmp2<=0 || parts[i].life<=1)
	{
		if (parts[i].tmp2>0)
			parts[i].tmp2=0;
		parts[i].tmp2--;
		return 1;
	}
	if (parts[i].tmp2<=-2 || voidnearby)
	{
		kill_part(i);
		return 1;
	}

	near = LIGH_nearest_part(i, (int)(parts[i].life*2.5));
	if (near!=-1)
	{
		int t=parts[near].type;
		float n_angle; // angle to nearest part
		float angle_diff;
		rx=(int)parts[near].x-x;
		ry=(int)parts[near].y-y;
		if (rx!=0 || ry!=0)
			n_angle = atan2f(-ry, rx);
		else
			n_angle = 0;
		if (n_angle<0)
			n_angle+=M_PI*2;
		angle_diff = fabsf(n_angle-parts[i].tmp*M_PI/180);
		if (angle_diff>M_PI)
			angle_diff = M_PI*2 - angle_diff;
		if (parts[i].life<5 || angle_diff<M_PI*0.8) // lightning strike
		{
			create_line_par(x, y, x+rx, y+ry, PT_LIGH, (int)parts[i].temp, parts[i].life, parts[i].tmp-90, 0);

			if (t!=PT_TESC)
			{
				near=contact_part(near, PT_LIGH);
				if (near!=-1)
				{
					parts[near].tmp2=3;
					parts[near].life=(int)(1.0*parts[i].life/2-1);
					parts[near].tmp=parts[i].tmp-180;
					parts[near].temp=parts[i].temp;
				}
			}
		}
		else near=-1;
	}

	//if (parts[i].tmp2==1/* || near!=-1*/)
	//angle=0;//parts[i].tmp-30+rand()%60;
	angle = parts[i].tmp-30.0f+rand()%60;
	if (angle<0)
		angle+=360;
	if (angle>=360)
		angle-=360;
	if (parts[i].tmp2==2 && near==-1)
	{
		angle2=angle+100-rand()%200;
		if (angle2<0)
			angle2+=360;
		if (angle2>=360)
			angle-=360;
	}

	multipler=(int)(parts[i].life*1.5+rand()%((int)(parts[i].life+1)));
	rx=(int)(cos(angle*M_PI/180)*multipler);
	ry=(int)(-sin(angle*M_PI/180)*multipler);
	create_line_par(x, y, x+rx, y+ry, PT_LIGH, (int)parts[i].temp, parts[i].life, (int)angle, 0);

	if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry))
	{
		r = pmap[y+ry][x+rx];
		if ((r&0xFF)==PT_LIGH)
		{
			parts[r>>8].tmp2=1+(rand()%200>parts[i].tmp2*parts[i].tmp2/10+60);
			parts[r>>8].life=(int)(1.0*parts[i].life/1.5-rand()%2);
			parts[r>>8].tmp=(int)angle;
			parts[r>>8].temp=parts[i].temp;
		}
	}

	if (angle2!=-1)
	{
		multipler=(int)(parts[i].life*1.5+rand()%((int)(parts[i].life+1)));
		rx=(int)(cos(angle2*M_PI/180)*multipler);
		ry=(int)(-sin(angle2*M_PI/180)*multipler);
		create_line_par(x, y, x+rx, y+ry, PT_LIGH, (int)parts[i].temp, parts[i].life, (int)angle2, 0);

		if (BOUNDS_CHECK && (rx || ry))
		{
			r = pmap[y+ry][x+rx];
			if ((r&0xFF)==PT_LIGH)
			{
				parts[r>>8].tmp2=1+(rand()%200>parts[i].tmp2*parts[i].tmp2/10+40);
				parts[r>>8].life=(int)(1.0*parts[i].life/1.5-rand()%2);
				parts[r>>8].tmp=(int)angle;
				parts[r>>8].temp=parts[i].temp;
			}
		}
	}

	parts[i].tmp2=-1;
	return 1;
}

int LIGH_graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 120;
	*firer = *colr = 235;
	*fireg = *colg = 245;
	*fireb = *colb = 255;
	*pixel_mode |= PMODE_GLOW | FIRE_ADD;
	return 1;
}

void LIGH_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_LIGH";
	elem->Name = "LIGH";
	elem->Colour = COLPACK(0xFFFFC0);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_EXPLOSIVE;
	elem->Enabled = 1;

	elem->Advection = 0.0f;
	elem->AirDrag = 0.00f * CFDS;
	elem->AirLoss = 0.90f;
	elem->Loss = 0.00f;
	elem->Collision = 0.0f;
	elem->Gravity = 0.0f;
	elem->Diffusion = 0.00f;
	elem->PressureAdd_NoAmbHeat = 0.000f	* CFDS;
	elem->Falldown = 0;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 1;

	elem->Weight = 100;

	elem->CreationTemperature = R_TEMP+0.0f	+273.15f;
	elem->HeatConduct = 0;
	elem->Latent = 0;
	elem->Description = "More realistic lightning. Set pen size to set the size of the lightning.";

	elem->State = ST_SOLID;
	elem->Properties = TYPE_SOLID;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = ITH;
	elem->HighTemperatureTransitionElement = NT;

	elem->Update = &LIGH_update;
	elem->Graphics = &LIGH_graphics;
}