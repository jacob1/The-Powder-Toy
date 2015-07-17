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

#include <cmath>
#include "powder.h"
#include "gravity.h"
#include "misc.h"
#include "interface.h" //for framenum, try and remove this later
#include "luaconsole.h" //for lua_el_mode
#include "game/Brush.h"

//Simulation stuff
#include "Element.h"
#include "ElementDataContainer.h"
#include "simulation/elements/MOVS.h"
#include "Tool.h"
#include "Simulation.h"
#include "CoordStack.h"

// Declare the element initialisation functions
#define ElementNumbers_Include_Decl
#define DEFINE_ELEMENT(name, id) void name ## _init_element(ELEMENT_INIT_FUNC_ARGS);
#include "ElementNumbers.h"
#include "WallNumbers.h"
#include "ToolNumbers.h"


Simulation *globalSim = NULL; // TODO: remove this global variable

Simulation::Simulation():
	currentTick(0),
	pfree(-1),
	parts_lastActiveIndex(NPART-1),
	debug_currentParticle(0),
	forceStackingCheck(false),
	msRotation(true),
	maxFrames(25),
	instantActivation(true),
	lightningRecreate(0)
{
	std::fill(&elementData[0], &elementData[PT_NUM], static_cast<ElementDataContainer*>(NULL));
	Clear();
}

Simulation::~Simulation()
{
	int t;
	for (t=0; t<PT_NUM; t++)
	{
		if (elementData[t])
		{
			delete elementData[t];
			elementData[t] = NULL;
		}
	}
}

void Simulation::InitElements()
{
	#define DEFINE_ELEMENT(name, id) if (id>=0 && id<PT_NUM) { name ## _init_element(this, &elements[id], id); };
	#define ElementNumbers_Include_Call
	#include "simulation/ElementNumbers.h"

	Simulation_Compat_CopyData(this);
}

void Simulation::Clear()
{
	int t;
	for (t=0; t<PT_NUM; t++)
	{
		if (elementData[t])
		{
			elementData[t]->Simulation_Cleared(this);
		}
	}
	std::fill(&elementCount[0], &elementCount[PT_NUM], 0);
	pfree = 0;
	parts_lastActiveIndex = NPART-1;

	instantActivation = true;
}

void Simulation::RecountElements()
{
	std::fill(&elementCount[0], &elementCount[PT_NUM], 0);
	for (int i = 0; i < NPART; i++)
		if (parts[i].type)
			elementCount[parts[i].type]++;
}

// the function for creating a particle
// p=-1 for normal creation (checks whether the particle is allowed to be in that location first)
// p=-3 to create without checking whether the particle is allowed to be in that location
// or p = a particle number, to replace a particle
int Simulation::part_create(int p, int x, int y, int t, int v)
{
	// This function is only for actually creating particles.
	// Not for tools, or changing things into spark, or special brush things like setting clone ctype.

	int i, oldType = PT_NONE;
	if (x<0 || y<0 || x>=XRES || y>=YRES || t<=0 || t>=PT_NUM || !elements[t].Enabled)
	{
		return -1;
	}

	// Spark Checks here
	if (p == -2 && (pmap[y][x]&0xFF) == PT_BUTN && parts[pmap[y][x]>>8].life == 10)
	{
		spark_conductive(pmap[y][x]>>8, x, y);
		return pmap[y][x]>>8;
	}
	if (t==PT_SPRK)
	{
		int type = pmap[y][x]&0xFF;
		int index = pmap[y][x]>>8;
		if(type == PT_WIRE)
		{
			parts[index].ctype = PT_DUST;
			return index;
		}
		if (p==-2 && ((elements[type].Properties & PROP_DRAWONCTYPE) || type==PT_CRAY))
		{
			parts[index].ctype = PT_SPRK;
			return index;
		}
		if (!(type == PT_INST || (elements[type].Properties&PROP_CONDUCTS)))
			return -1;
		if (parts[index].life!=0)
			return -1;
		if (type == PT_INST)
		{
			if (p == -2)
				INST_flood_spark(this, x, y);
			else
				spark_conductive(index, x, y);
			return index;
		}

		spark_conductive_attempt(index, x, y);
		return index;
	}
	// End Spark checks

	//Brush Creation
	if (p == -2)
	{
		if (pmap[y][x])
		{
			int drawOn = pmap[y][x]&0xFF;
			//If an element has the PROP_DRAWONCTYPE property, and the element being drawn to it does not have PROP_NOCTYPEDRAW (Also some special cases), set the element's ctype
			if (((elements[drawOn].Properties & PROP_DRAWONCTYPE) ||
				 (drawOn == PT_STOR && !(elements[t].Properties & TYPE_SOLID)) ||
				 (drawOn == PT_PCLN && t != PT_PSCN && t != PT_NSCN) ||
				 (drawOn == PT_PBCN && t != PT_PSCN && t != PT_NSCN))
				&& (!(elements[t].Properties & PROP_NOCTYPEDRAW)))
			{
				parts[pmap[y][x]>>8].ctype = t;
				if (t == PT_LIFE && v >= 0 && v < NGOL && drawOn != PT_STOR)
					parts[pmap[y][x]>>8].tmp = v;
			}
			else if ((drawOn == PT_DTEC || (drawOn == PT_PSTN && t != PT_FRME) || drawOn == PT_DRAY) && drawOn != t)
			{
				parts[pmap[y][x]>>8].ctype = t;
				if (t == PT_LIFE && v >= 0 && v < NGOL)
				{
					if (drawOn == PT_DTEC)
						parts[pmap[y][x]>>8].tmp = v;
					else if (drawOn == PT_DRAY)
						parts[pmap[y][x]>>8].ctype |= v<<8;
				}
			}
			else if (drawOn == PT_CRAY && drawOn != t)
			{
				parts[pmap[y][x]>>8].ctype = t;
				if (t==PT_LIFE && v<NGOL)
					parts[pmap[y][x]>>8].tmp2 = v;
				parts[pmap[y][x]>>8].temp = elements[t].DefaultProperties.temp;
			}
			return -1;
		}
		else if (IsWallBlocking(x, y, t))
			return -1;
		if (photons[y][x] && (elements[t].Properties & TYPE_ENERGY))
			return -1;
	}
	// End Brush Creation

	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, p, x, y, t))
			return -1;
	}

	if (p == -1)
	{
		// Check whether the particle can be created here

		// If there is a particle, only allow creation if the new particle can occupy the same space as the existing particle
		// If there isn't a particle but there is a wall, check whether the new particle is allowed to be in it
		//   (not "!=2" for wall check because eval_move returns 1 for moving into empty space)
		// If there's no particle and no wall, assume creation is allowed
		if (pmap[y][x] ? (EvalMove(t, x, y)!=2) : (bmap[y/CELL][x/CELL] && EvalMove(t, x, y)==0))
		{
			return -1;
		}
		i = part_alloc();
	}
	else if (p == -3) // skip pmap checks, e.g. for sing explosion
	{
		i = part_alloc();
	}
	else if (p >= 0) // Replace existing particle
	{
		int oldX = (int)(parts[p].x+0.5f);
		int oldY = (int)(parts[p].y+0.5f);
		oldType = parts[p].type;
		if (elements[oldType].Func_ChangeType)
		{
			(*(elements[oldType].Func_ChangeType))(this, p, oldX, oldY, oldType, t);
		}
		if (oldType) elementCount[oldType]--;
		pmap_remove(p, oldX, oldY);
		i = p;
	}
	else // Dunno, act like it was p=-3
	{
		i = part_alloc();
	}

	// Check whether a particle was successfully allocated
	if (i<0)
		return -1;

	// Set some properties
	parts[i] = elements[t].DefaultProperties;
	parts[i].type = t;
	parts[i].x = (float)x;
	parts[i].y = (float)y;
#ifdef OGLR
	parts[i].lastX = (float)x;
	parts[i].lastY = (float)y;
#endif

	// Fancy dust effects for powder types
	if ((elements[t].Properties & TYPE_PART) && pretty_powder)
	{
		int sandcolour = (int)(20.0f*sin((float)(currentTick%360)*(M_PI/180.0f)));
		int colr = (int)(COLR(elements[t].Colour)+sandcolour*1.3f+(rand()%40)-20+(rand()%30)-15);
		int colg = (int)(COLG(elements[t].Colour)+sandcolour*1.3f+(rand()%40)-20+(rand()%30)-15);
		int colb = (int)(COLB(elements[t].Colour)+sandcolour*1.3f+(rand()%40)-20+(rand()%30)-15);
		colr = std::max(0, std::min(255, colr));
		colg = std::max(0, std::min(255, colg));
		colb = std::max(0, std::min(255, colb));
		parts[i].dcolour = COLARGB(rand()%150, colr, colg, colb);
	}

	// Set non-static properties (such as randomly generated ones)
	if (elements[t].Func_Create)
	{
		(*(elements[t].Func_Create))(this, i, x, y, t, v);
	}

	pmap_add(i, x, y, t);

	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, oldType, t);
	}

	elementCount[t]++;
	return i;
}

// changes the type of particle number i, to t. This also changes pmap at the same time.
bool Simulation::part_change_type(int i, int x, int y, int t)
{
	if (x<0 || y<0 || x>=XRES || y>=YRES || i>=NPART || t<0 || t>=PT_NUM)
		return false;

	if (t == parts[i].type)
		return true;
	if (!elements[t].Enabled)
	{
		part_kill(i);
		return true;
	}
	if (elements[parts[i].type].Properties&PROP_INDESTRUCTIBLE)
		return false;
	if (elements[t].Func_Create_Allowed)
	{
		if (!(*(elements[t].Func_Create_Allowed))(this, i, x, y, t))
			return false;
	}


	int oldType = parts[i].type;
	if (oldType)
		elementCount[oldType]--;

	parts[i].type = t;
	pmap_remove(i, x, y);
	if (t)
	{
		pmap_add(i, x, y, t);
		elementCount[t]++;
	}
	if (elements[oldType].Func_ChangeType)
	{
		(*(elements[oldType].Func_ChangeType))(this, i, x, y, oldType, t);
	}
	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, oldType, t);
	}
	return true;
}

// used by lua to change type while deleting any particle specific info, and also keep pmap / elementCount up to date
void Simulation::part_change_type_force(int i, int t)
{
	int x = (int)(parts[i].x), y = (int)(parts[i].y);
	if (t<0 || t>=PT_NUM)
		return;

	int oldType = parts[i].type;
	if (oldType)
		elementCount[oldType]--;
	parts[i].type = t;
	pmap_remove(i, x, y);
	if (t)
	{
		pmap_add(i, x, y, t);
		elementCount[t]++;
	}

	if (elements[oldType].Func_ChangeType)
	{
		(*(elements[oldType].Func_ChangeType))(this, i, x, y, oldType, t);
	}
	if (elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, oldType, t);
	}
}

// kills particle ID #i
void Simulation::part_kill(int i)
{
	int x = (int)(parts[i].x+0.5f);
	int y = (int)(parts[i].y+0.5f);

	int t = parts[i].type;
	if (t && elements[t].Func_ChangeType)
	{
		(*(elements[t].Func_ChangeType))(this, i, x, y, t, PT_NONE);
	}

	if (x>=0 && y>=0 && x<XRES && y<YRES)
		pmap_remove(i, x, y);
	if (t == PT_NONE) // TODO: remove this? (//This shouldn't happen anymore, but it's here just in case)
		return;
	elementCount[t]--;
	part_free(i);
}

// calls kill_part with the particle located at (x,y)
void Simulation::part_delete(int x, int y)
{
	if (x<0 || y<0 || x>=XRES || y>=YRES)
		return;

	if (photons[y][x])
		part_kill(photons[y][x]>>8);
	else if (pmap[y][x])
		part_kill(pmap[y][x]>>8);
}

/* Recalculates the pfree/parts[].life linked list for particles with ID <= parts_lastActiveIndex.
 * This ensures that future particle allocations are done near the start of the parts array, to keep parts_lastActiveIndex low.
 * parts_lastActiveIndex is also decreased if appropriate.
 * Does not modify or even read any particles beyond parts_lastActiveIndex */
void Simulation::RecalcFreeParticles()
{
	int x, y, t;
	int lastPartUsed = 0;
	int lastPartUnused = -1;

	std::fill_n(&pmap[0][0], XRES*YRES, 0);
	std::fill_n(&pmap_count[0][0], XRES*YRES, 0);
	std::fill_n(&photons[0][0], XRES*YRES, 0);

	NUM_PARTS = 0;
	for (int i = 0; i <= parts_lastActiveIndex; i++)//the particle loop that resets the pmap/photon maps every frame, to update them.
	{
		if (parts[i].type)
		{
			t = parts[i].type;
			x = (int)(parts[i].x+0.5f);
			y = (int)(parts[i].y+0.5f);
			if (parts[i].flags&FLAG_SKIPMOVE)
				parts[i].flags &= ~FLAG_SKIPMOVE;
			if (x>=0 && y>=0 && x<XRES && y<YRES)
			{
				if (t == PT_PINV && (parts[i].tmp2>>8) >= i)
					parts[i].tmp2 = 0;
				if (elements[t].Properties & TYPE_ENERGY)
					photons[y][x] = t|(i<<8);
				else
				{
					// Particles are sometimes allowed to go inside INVS and FILT
					// To make particles collide correctly when inside these elements, these elements must not overwrite an existing pmap entry from particles inside them
					if (!pmap[y][x] || (t!=PT_INVIS && t!=PT_FILT && (t!=PT_MOVS || (pmap[y][x]&0xFF)==PT_MOVS) && (pmap[y][x]&0xFF)!=PT_PINV))
						pmap[y][x] = t|(i<<8);
					else if ((pmap[y][x]&0xFF) == PT_PINV)
						parts[pmap[y][x]>>8].tmp2 = t|(i<<8);
					// Count number of particles at each location, for excess stacking check
					// (does not include energy particles or THDR - currently no limit on stacking those)
					if (t!=PT_THDR && t!=PT_EMBR && t!=PT_FIGH && t!=PT_PLSM && t!=PT_MOVS)
						pmap_count[y][x]++;
				}
			}
			lastPartUsed = i;
			NUM_PARTS++;
			if (!sys_pause || framerender)
				decrease_life(i); //decrease the life of certain elements by 1 every frame
		}
		else
		{
			if (lastPartUnused < 0)
				pfree = i;
			else
				parts[lastPartUnused].life = i;
			lastPartUnused = i;
		}
	}

	if (lastPartUnused == -1)
	{
		if (parts_lastActiveIndex >= NPART-1)
			pfree = -1;
		else
			pfree = parts_lastActiveIndex+1;
	}
	else
	{
		if (parts_lastActiveIndex >= NPART-1)
			parts[lastPartUnused].life = -1;
		else
			parts[lastPartUnused].life = parts_lastActiveIndex+1;
	}
	parts_lastActiveIndex = lastPartUsed;
}

void Simulation::UpdateBefore()
{
	//update wallmaps
	for (int y = 0; y < YRES/CELL; y++)
	{
		for (int x = 0; x < XRES/CELL; x++)
		{
			if (emap[y][x])
				emap[y][x]--;
			bmap_blockair[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_BLOCKAIR || (bmap[y][x]==WL_EWALL && !emap[y][x]));
			bmap_blockairh[y][x] = (bmap[y][x]==WL_WALL || bmap[y][x]==WL_WALLELEC || bmap[y][x]==WL_BLOCKAIR || bmap[y][x]==WL_GRAV || (bmap[y][x]==WL_EWALL && !emap[y][x])) ? 0x8:0;
		}
	}

	//create stickmen if the current one has been deleted
	if (elementCount[PT_STKM] <= 0 && player.spawnID >= 0)
		part_create(-1, (int)parts[player.spawnID].x, (int)parts[player.spawnID].y, PT_STKM);
	else if (elementCount[PT_STKM2] <= 0 && player2.spawnID >= 0)
		part_create(-1, (int)parts[player2.spawnID].x, (int)parts[player2.spawnID].y, PT_STKM2);

	//check for excessive stacked particles, create BHOL if found
	if (forceStackingCheck || !(rand()%10))
	{
		bool excessiveStackingFound = false;
		forceStackingCheck = 0;
		for (int y = 0; y < YRES; y++)
		{
			for (int x = 0; x < XRES; x++)
			{
				//Use a threshold, since some particle stacking can be normal (e.g. BIZR + FILT)
				//Setting pmap_count[y][x] > NPART means BHOL will form in that spot
				if (pmap_count[y][x] > 5)
				{
					if (bmap[y/CELL][x/CELL] == WL_EHOLE)
					{
						//Allow more stacking in E-hole
						if (pmap_count[y][x] > 1500)
						{
							pmap_count[y][x] = pmap_count[y][x] + NPART;
							excessiveStackingFound = true;
						}
					}
					//Random chance to turn into BHOL that increases with the amount of stacking, up to a threshold where it is certain to turn into BHOL
					else if (pmap_count[y][x] > 1500 || (rand()%1600) <= pmap_count[y][x]+100)
					{
						pmap_count[y][x] = pmap_count[y][x] + NPART;
						excessiveStackingFound = true;
					}
				}
			}
		}
		if (excessiveStackingFound)
		{
			for (int i = 0; i <= parts_lastActiveIndex; i++)
			{
				if (parts[i].type)
				{
					int t = parts[i].type;
					int x = (int)(parts[i].x+0.5f);
					int y = (int)(parts[i].y+0.5f);
					if (x >= 0 && y >= 0 && x < XRES && y < YRES && !(elements[t].Properties&TYPE_ENERGY))
					{
						if (pmap_count[y][x] >= NPART)
						{
							if (pmap_count[y][x] > NPART)
							{
								part_create(i, x, y, PT_NBHL);
								parts[i].temp = MAX_TEMP;
								parts[i].tmp = pmap_count[y][x] - NPART;//strength of grav field
								if (parts[i].tmp > 51200)
									parts[i].tmp = 51200;
								pmap_count[y][x] = NPART;
							}
							else
							{
								part_kill(i);
							}
						}
					}
				}
			}
		}
	}

	// For elements with extra data, run special update functions
	// This does things like LIFE recalculation and LOLZ patterns
	for (int t = 1; t < PT_NUM; t++)
	{
		if (elementData[t])
		{
			elementData[t]->Simulation_BeforeUpdate(this);
		}
	}

	// lightning recreation time (TODO: move to elementData)
	if (lightningRecreate)
		lightningRecreate--;
}

void Simulation::UpdateParticles(int start, int end)
{
	// The main particle loop function, goes over all particles.
	for (int i = start; i <= end && i <= parts_lastActiveIndex; i++)
		if (parts[i].type)
		{
			UpdateParticle(i);
		}
}

void Simulation::UpdateAfter()
{
	// For elements with extra data, run special update functions
	// Used only for moving solids
	for (int t = 1; t < PT_NUM; t++)
	{
		if (elementData[t])
		{
			elementData[t]->Simulation_AfterUpdate(this);
		}
	}
}

void Simulation::Tick()
{
	RecalcFreeParticles();
	if (!sys_pause || framerender)
	{
		UpdateBefore();
		UpdateParticles(0, NPART);
		UpdateAfter();
	}
	// In automatic heat mode, calculate highest and lowest temperature points (maybe could be moved)
	if (heatmode == 1)
	{
		highesttemp = MIN_TEMP;
		lowesttemp = MAX_TEMP;
		for (int i = 0; i <= parts_lastActiveIndex; i++)
		{
			if (parts[i].type)
			{
				if (parts[i].temp > highesttemp)
					highesttemp = (int)parts[i].temp;
				if (parts[i].temp < lowesttemp)
					lowesttemp = (int)parts[i].temp;
			}
		}
	}
	currentTick++;
}

int PCLN_update(UPDATE_FUNC_ARGS);
int CLNE_update(UPDATE_FUNC_ARGS);
int PBCN_update(UPDATE_FUNC_ARGS);
int BCLN_update(UPDATE_FUNC_ARGS);
int MOVS_update(UPDATE_FUNC_ARGS);

bool Simulation::UpdateParticle(int i)
{
	int t = parts[i].type;
	int x = (int)(parts[i].x+0.5f);
	int y = (int)(parts[i].y+0.5f);
	float pGravX, pGravY, pGravD;
	bool transitionOccurred = false;

	//this kills any particle out of the screen, or in a wall where it isn't supposed to go
	if (OutOfBounds(x, y) ||
		( bmap[y/CELL][x/CELL] &&
		  ( bmap[y/CELL][x/CELL] == WL_WALL ||
		   (bmap[y/CELL][x/CELL] == WL_WALLELEC) ||
		   (bmap[y/CELL][x/CELL] == WL_ALLOWAIR) ||
		   (bmap[y/CELL][x/CELL] == WL_DESTROYALL) ||
		   (bmap[y/CELL][x/CELL] == WL_ALLOWLIQUID && elements[t].Falldown != 2) ||
		   (bmap[y/CELL][x/CELL] == WL_ALLOWSOLID && elements[t].Falldown != 1) ||
		   (bmap[y/CELL][x/CELL] == WL_ALLOWGAS && !(elements[t].Properties&TYPE_GAS)) || //&&  elements[t].Falldown!=0 && t!=PT_FIRE && t!=PT_SMKE && t!=PT_HFLM) ||
		   (bmap[y/CELL][x/CELL] == WL_ALLOWENERGY && !(elements[t].Properties&TYPE_ENERGY)) ||
		   (bmap[y/CELL][x/CELL] == WL_DETECT && (t==PT_METL || t==PT_SPRK)) ||
		   (bmap[y/CELL][x/CELL] == WL_EWALL && !emap[y/CELL][x/CELL])
		  ) && t!=PT_STKM && t!=PT_STKM2 && t!=PT_FIGH && t != PT_MOVS))
	{
		part_kill(i);
		return true;
	}
	if (bmap[y/CELL][x/CELL]==WL_DETECT && emap[y/CELL][x/CELL]<8)
		set_emap(x/CELL, y/CELL);

	if (parts[i].flags&FLAG_SKIPMOVE)
		return false;

	//adding to velocity from the particle's velocity
	vx[y/CELL][x/CELL] = vx[y/CELL][x/CELL]*elements[t].AirLoss + elements[t].AirDrag*parts[i].vx;
	vy[y/CELL][x/CELL] = vy[y/CELL][x/CELL]*elements[t].AirLoss + elements[t].AirDrag*parts[i].vy;

	if (elements[t].HotAir)
	{
		if (t == PT_GAS || t == PT_NBLE)
		{
			if (pv[y/CELL][x/CELL] < 3.5f)
				pv[y/CELL][x/CELL] += elements[t].HotAir*(3.5f - pv[y/CELL][x/CELL]);
			if (y+CELL < YRES && pv[y/CELL+1][x/CELL] < 3.5f)
				pv[y/CELL+1][x/CELL] += elements[t].HotAir*(3.5f - pv[y/CELL+1][x/CELL]);
			if (x+CELL < XRES)
			{
				if (pv[y/CELL][x/CELL+1] < 3.5f)
					pv[y/CELL][x/CELL+1] += elements[t].HotAir*(3.5f - pv[y/CELL][x/CELL+1]);
				if (y+CELL<YRES && pv[y/CELL+1][x/CELL+1] < 3.5f)
					pv[y/CELL+1][x/CELL+1] += elements[t].HotAir*(3.5f - pv[y/CELL+1][x/CELL+1]);
			}
		}
		//add the hotair variable to the pressure map, like black hole, or white hole.
		else
		{
			float value = elements[t].HotAir;
			value = restrict_flt(value, -256.0f, 256.0f);
			pv[y/CELL][x/CELL] += value;
			if (y+CELL < YRES)
				pv[y/CELL+1][x/CELL] += value;
			if (x+CELL < XRES)
			{
				pv[y/CELL][x/CELL+1] += value;
				if (y+CELL < YRES)
					pv[y/CELL+1][x/CELL+1] += value;
			}
		}
	}
	if (elements[t].Gravity || !(elements[t].Properties & TYPE_SOLID))
	{

		//Gravity mode by Moach
		switch (gravityMode)
		{
		default:
		case 0:
			pGravX = 0.0f;
			pGravY = elements[t].Gravity;
			break;
		case 1:
			pGravX = pGravY = 0.0f;
			break;
		case 2:
			pGravD = 0.01f - hypotf(((float)x - XCNTR), ((float)y - YCNTR));
			pGravX = elements[t].Gravity * ((float)(x - XCNTR) / pGravD);
			pGravY = elements[t].Gravity * ((float)(y - YCNTR) / pGravD);
		}
		//Get some gravity from the gravity map
		if (t == PT_ANAR)
		{
			pGravX -= gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
			pGravY -= gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
		}
		else if (t != PT_STKM && t != PT_STKM2 && t != PT_FIGH && !(elements[t].Properties & TYPE_SOLID))
		{
			pGravX += gravx[(y/CELL)*(XRES/CELL)+(x/CELL)];
			pGravY += gravy[(y/CELL)*(XRES/CELL)+(x/CELL)];
		}
	}
	else
		pGravX = pGravY = 0;

	//velocity updates for the particle
	parts[i].vx *= elements[t].Loss;
	parts[i].vy *= elements[t].Loss;
	//particle gets velocity from the vx and vy maps
	parts[i].vx += elements[t].Advection*vx[y/CELL][x/CELL] + pGravX;
	parts[i].vy += elements[t].Advection*vy[y/CELL][x/CELL] + pGravY;


	if (elements[t].Diffusion)//the random diffusion that gases have
	{
		if (realistic)
		{
			//The magic number controls diffusion speed
			parts[i].vx += 0.05f*sqrtf(parts[i].temp)*elements[t].Diffusion*(rand()/(0.5f*RAND_MAX)-1.0f);
			parts[i].vy += 0.05f*sqrtf(parts[i].temp)*elements[t].Diffusion*(rand()/(0.5f*RAND_MAX)-1.0f);
		}
		else
		{
			parts[i].vx += elements[t].Diffusion*(rand()/(0.5f*RAND_MAX)-1.0f);
			parts[i].vy += elements[t].Diffusion*(rand()/(0.5f*RAND_MAX)-1.0f);
		}
	}

	//surround_space stores the number of empty spaces around a particle, nt stores the number of empty spaces + the number of particles of a different type
	int surround_space = 0, surround_particle = 0, nt = 0;
	//surround stores the 8 particle types around the current one, used in heat transfer
	int surround[8];
	for (int nx = -1; nx <= 1; nx++)
		for (int ny = -1; ny <= 1; ny++)
		{
			if (nx || ny)
			{
				int r;
				if (!OutOfBounds(x+nx, y+ny))
				{
					surround[surround_particle] = r = pmap[y+ny][x+nx];
					surround_particle++;

					//there is empty space
					if (!(r&0xFF))
						surround_space++;

					//there is nothing or a different particle
					if ((r&0xFF) != t)
						nt++;
				}
				else
				{
					surround[surround_particle] = 0;
					surround_particle++;
					surround_space++;
					nt++;
				}
			}
		}

	if (!legacy_enable)
	{
		if (transfer_heat(i, t, surround))
		{
			transitionOccurred = true;
			t = parts[i].type;
		}
		if (!t)
			return true;
	}

	//spark updates from walls
	if ((elements[t].Properties&PROP_CONDUCTS) || t == PT_SPRK)
	{
		int nx = x%CELL;
		if (nx == 0)
			nx = x/CELL - 1;
		else if (nx == CELL-1)
			nx = x/CELL + 1;
		else
			nx = x/CELL;

		int ny = y%CELL;
		if (ny == 0)
			ny = y/CELL - 1;
		else if (ny == CELL-1)
			ny = y/CELL + 1;
		else
			ny = y/CELL;

		if (nx >= 0 && ny >= 0 && nx < XRES/CELL && ny < YRES/CELL)
		{
			if (t != PT_SPRK)
			{
				if (emap[ny][nx] == 12 && !parts[i].life)
				{
					spark_conductive(i, x, y);
					parts[i].life = 4;
					t = PT_SPRK;
				}
			}
			else if (bmap[ny][nx] == WL_DETECT || bmap[ny][nx] == WL_EWALL || bmap[ny][nx] == WL_ALLOWLIQUID || bmap[ny][nx] == WL_WALLELEC || bmap[ny][nx] == WL_ALLOWALLELEC || bmap[ny][nx] == WL_EHOLE)
				set_emap(nx, ny);
		}
	}

	//the basic explosion, from the .explosive variable
	if (!(elements[t].Properties&PROP_INDESTRUCTIBLE) && (elements[t].Explosive&2) && pv[y/CELL][x/CELL] > 2.5f)
	{
		parts[i].life = rand()%80 + 180;
		parts[i].temp = restrict_flt(elements[PT_FIRE].DefaultProperties.temp + (elements[t].Flammable/2), MIN_TEMP, MAX_TEMP);
		t = PT_FIRE;
		part_change_type(i, x, y, t);
		pv[y/CELL][x/CELL] += 0.25f * CFDS;
	}

	if (!(elements[t].Properties&PROP_INDESTRUCTIBLE) && (elements[t].HighPressureTransitionThreshold != -1 || elements[t].HighPressureTransitionElement != -1))
	{
		if (particle_transitions(i, t))
		{
			transitionOccurred = true;
			t = parts[i].type;
		}
		if (!t)
			return true;
	}

	//call the particle update function, if there is one
#ifdef LUACONSOLE
	if (lua_el_mode[parts[i].type] == 3)
	{
		if (luacon_part_update(t, i, x, y, surround_space, nt) || t != parts[i].type)
			return true;
		// Need to update variables, in case they've been changed by Lua
		x = (int)(parts[i].x+0.5f);
		y = (int)(parts[i].y+0.5f);
	}

	if (lua_el_mode[t] != 2)
	{
#endif
		if (elements[t].Properties&PROP_POWERED)
		{
			if (update_POWERED(this, i, x, y, surround_space, nt))
				return true;
		}
		if (elements[t].Properties&PROP_CLONE)
		{
			if (elements[t].Properties&PROP_POWERED)
				PCLN_update(this, i, x, y, surround_space, nt);
			else
				CLNE_update(this, i, x, y, surround_space, nt);
		}
		else if (elements[t].Properties&PROP_BREAKABLECLONE)
		{
			if (elements[t].Properties&PROP_POWERED)
			{
				if (PBCN_update(this, i, x, y, surround_space, nt))
					return true;
			}
			else
			{
				if (BCLN_update(this, i, x, y, surround_space, nt))
					return true;
			}
		}
		if (elements[t].Update)
		{
			if ((*(elements[t].Update))(this, i, x, y, surround_space, nt))
				return true;
			else if (t == PT_WARP)
			{
				// Warp does some movement in its update func, update variables to avoid incorrect data in pmap
				x = (int)(parts[i].x+0.5f);
				y = (int)(parts[i].y+0.5f);
			}
		}
#ifdef LUACONSOLE
	}

	if (lua_el_mode[parts[i].type] && lua_el_mode[parts[i].type] != 3)
	{
		if (luacon_part_update(t, i, x, y, surround_space, nt) || t != parts[i].type)
			return true;
		// Need to update variables, in case they've been changed by Lua
		x = (int)(parts[i].x+0.5f);
		y = (int)(parts[i].y+0.5f);
	}
#endif
	if (legacy_enable)//if heat sim is off
		update_legacy_all(this, i, x, y,surround_space, nt);

	//if its dead, skip to next particle
	if (parts[i].type == PT_NONE)
		return true;

	if (parts[i].flags&FLAG_EXPLODE)
	{
		if (!(rand()%10))
		{
			parts[i].flags &= ~FLAG_EXPLODE;
			pv[y/CELL][x/CELL] += 5.0f;
			if(!(rand()%3))
			{
				if(!(rand()%2))
				{
					part_create(i, x, y, PT_BOMB);
					parts[i].temp = MAX_TEMP;
				}
				else
				{
					part_create(i, x, y, PT_PLSM);
					parts[i].temp = MAX_TEMP;
				}
			}
			else
			{
				part_create(i, x, y, PT_EMBR);
				parts[i].temp = MAX_TEMP;
				parts[i].vx = rand()%20-10.0f;
				parts[i].vy = rand()%20-10.0f;
			}
			return true;
		}
	}

	if (transitionOccurred)
		return false;

	if (!parts[i].vx && !parts[i].vy)//if its not moving, skip to next particle, movement code is next
		return false;

	float mv = std::max(fabsf(parts[i].vx), fabsf(parts[i].vy));
	int fin_x, fin_y, clear_x, clear_y;
	float fin_xf, fin_yf, clear_xf, clear_yf;
	if (mv < ISTP)
	{
		clear_x = x;
		clear_y = y;
		clear_xf = parts[i].x;
		clear_yf = parts[i].y;
		fin_xf = clear_xf + parts[i].vx;
		fin_yf = clear_yf + parts[i].vy;
		fin_x = (int)(fin_xf+0.5f);
		fin_y = (int)(fin_yf+0.5f);
	}
	else
	{
		if (mv > SIM_MAXVELOCITY)
		{
			parts[i].vx *= SIM_MAXVELOCITY/mv;
			parts[i].vy *= SIM_MAXVELOCITY/mv;
			mv = SIM_MAXVELOCITY;
		}
		// interpolate to see if there is anything in the way
		float dx = parts[i].vx*ISTP/mv;
		float dy = parts[i].vy*ISTP/mv;
		fin_xf = parts[i].x;
		fin_yf = parts[i].y;
		while (1)
		{
			mv -= ISTP;
			fin_xf += dx;
			fin_yf += dy;
			fin_x = (int)(fin_xf+0.5f);
			fin_y = (int)(fin_yf+0.5f);
			if (edgeMode == 2)
			{
				if (fin_x < CELL)
					fin_xf += XRES-CELL*2;
				if (fin_x >= XRES-CELL)
					fin_xf -= XRES-CELL*2;
				if (fin_y < CELL)
					fin_yf += YRES-CELL*2;
				if (fin_y >= YRES-CELL)
					fin_yf -= YRES-CELL*2;
				fin_x = (int)(fin_xf+0.5f);
				fin_y = (int)(fin_yf+0.5f);
			}
			if (mv <= 0.0f)
			{
				// nothing found
				fin_xf = parts[i].x + parts[i].vx;
				fin_yf = parts[i].y + parts[i].vy;
				fin_x = (int)(fin_xf+0.5f);
				fin_y = (int)(fin_yf+0.5f);
				clear_xf = fin_xf-dx;
				clear_yf = fin_yf-dy;
				clear_x = (int)(clear_xf+0.5f);
				clear_y = (int)(clear_yf+0.5f);
				break;
			}
			//block if particle can't move (0), or some special cases where it returns 1 (can_move = 3 but returns 1 meaning particle will be eaten)
			//also photons are still blocked (slowed down) by any particle (even ones it can move through), and absorb wall also blocks particles
			int eval = EvalMove(t, fin_x, fin_y);
			if (!eval || (can_move[t][pmap[fin_y][fin_x]&0xFF] == 3 && eval == 1) || (t == PT_PHOT && pmap[fin_y][fin_x]) || bmap[fin_y/CELL][fin_x/CELL]==WL_DESTROYALL)
			{
				// found an obstacle
				clear_xf = fin_xf-dx;
				clear_yf = fin_yf-dy;
				clear_x = (int)(clear_xf+0.5f);
				clear_y = (int)(clear_yf+0.5f);
				break;
			}
			if (bmap[fin_y/CELL][fin_x/CELL] == WL_DETECT && emap[fin_y/CELL][fin_x/CELL] < 8)
				set_emap(fin_x/CELL, fin_y/CELL);
		}
	}

	bool stagnant = parts[i].flags & FLAG_STAGNANT;
	parts[i].flags &= ~FLAG_STAGNANT;

	if (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH)
	{
		//head movement, let head pass through anything
		Move(i, x, y, parts[i].x+parts[i].vx, parts[i].y+parts[i].vy);
	}
	else if (elements[t].Properties & TYPE_ENERGY)
	{
		if (t == PT_PHOT)
		{
			if (EvalMove(PT_PHOT, fin_x, fin_y))
			{
				int rt = pmap[fin_y][fin_x] & 0xFF;
				int lt = pmap[y][x] & 0xFF;

				if ((rt == PT_GLAS && lt != PT_GLAS) || (rt != PT_GLAS && lt == PT_GLAS))
				{
					float nrx, nry;
					if (!GetNormalInterp(REFRACT|t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy, &nrx, &nry))
					{
						part_kill(i);
						return true;
					}

					int r = get_wavelength_bin(&parts[i].ctype);
					if (r == -1 || !(parts[i].ctype&0x3FFFFFFF))
					{
						part_kill(i);
						return true;
					}
					float nn = GLASS_IOR - GLASS_DISP*(r-15)/15.0f;
					nn *= nn;
					nrx = -nrx;
					nry = -nry;
					if (rt == PT_GLAS && lt != PT_GLAS)
						nn = 1.0f/nn;
					float ct1 = parts[i].vx*nrx + parts[i].vy*nry;
					float ct2 = 1.0f - (nn*nn)*(1.0f-(ct1*ct1));
					if (ct2 < 0.0f)
					{
						//total internal reflection
						parts[i].vx -= 2.0f*ct1*nrx;
						parts[i].vy -= 2.0f*ct1*nry;
						fin_xf = parts[i].x;
						fin_yf = parts[i].y;
						fin_x = x;
						fin_y = y;
					}
					else
					{
						// refraction
						ct2 = sqrtf(ct2);
						ct2 = ct2 - nn*ct1;
						parts[i].vx = nn*parts[i].vx + ct2*nrx;
						parts[i].vy = nn*parts[i].vy + ct2*nry;
					}
				}
			}
		}
		//FLAG_STAGNANT set, was reflected on previous frame
		if (stagnant)
		{
			// cast coords as int then back to float for compatibility with existing saves
			if (!DoMove(i, x, y, (float)fin_x, (float)fin_y) && parts[i].type)
			{
				part_kill(i);
				return true;
			}
		}
		else if (!DoMove(i, x, y, fin_xf, fin_yf))
		{
			float nrx, nry;
			if (parts[i].type == PT_NONE)
				return true;
			//reflection
			parts[i].flags |= FLAG_STAGNANT;
			if (t == PT_NEUT && !(rand()%10))
			{
				part_kill(i);
				return true;
			}
			int r = pmap[fin_y][fin_x];

			if (((r&0xFF)==PT_PIPE || (r&0xFF) == PT_PPIP) && !(parts[r>>8].tmp&0xFF))
			{
				parts[r>>8].tmp =  (parts[r>>8].tmp&~0xFF) | parts[i].type;
				parts[r>>8].temp = parts[i].temp;
				parts[r>>8].tmp2 = parts[i].life;
				parts[r>>8].pavg[0] = (float)parts[i].tmp;
				parts[r>>8].pavg[1] = (float)parts[i].ctype;
				part_kill(i);
				return true;
			}

			if (r&0xFF)
				parts[i].ctype &= elements[r&0xFF].PhotonReflectWavelengths;

			if (GetNormalInterp(t, parts[i].x, parts[i].y, parts[i].vx, parts[i].vy, &nrx, &nry))
			{
				float dp = nrx*parts[i].vx + nry*parts[i].vy;
				parts[i].vx -= 2.0f*dp*nrx;
				parts[i].vy -= 2.0f*dp*nry;
				// leave the actual movement until next frame so that reflection of fast particles and refraction happen correctly
			}
			else
			{
				if (t != PT_NEUT)
				{
					part_kill(i);
					return true;
				}
				return false;
			}
			if (!(parts[i].ctype&0x3FFFFFFF) && t == PT_PHOT)
			{
				part_kill(i);
				return true;
			}
		}
	}
	// gases and solids (but not powders)
	else if (elements[t].Falldown == 0)
	{
		if (!DoMove(i, x, y, fin_xf, fin_yf))
		{
			if (parts[i].type == PT_NONE)
				return true;
			// can't move there, so bounce off
			// TODO
			if (fin_x > x+ISTP) fin_x = x+ISTP;
			if (fin_x < x-ISTP) fin_x = x-ISTP;
			if (fin_y > y+ISTP) fin_y = y+ISTP;
			if (fin_y < y-ISTP) fin_y = y-ISTP;
			if (DoMove(i, x, y, 0.25f+(float)(2*x-fin_x), 0.25f+fin_y))
			{
				parts[i].vx *= elements[t].Collision;
			}
			else if (DoMove(i, x, y, 0.25f+fin_x, 0.25f+(float)(2*y-fin_y)))
			{
				parts[i].vy *= elements[t].Collision;
			}
			else
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
		}
	}
	//liquids and powders
	else
	{
		//checking stagnant is cool, but then it doesn't update when you change it later.
		if (water_equal_test && elements[t].Falldown == 2 && !(rand()%400))
		{
			if (!flood_water(x, y, i, y, parts[i].flags&FLAG_WATEREQUAL))
				return false;
		}
		if (!DoMove(i, x, y, fin_xf, fin_yf))
		{
			if (parts[i].type == PT_NONE)
				return true;
			if (fin_x != x && DoMove(i, x, y, fin_xf, clear_yf))
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
			else if (fin_y != y && DoMove(i, x, y, clear_xf, fin_yf))
			{
				parts[i].vx *= elements[t].Collision;
				parts[i].vy *= elements[t].Collision;
			}
			else
			{
				int nx, ny, s = 1;
				int r = (rand()%2)*2-1; // position search direction (left/right first)
				if ((clear_x!=x || clear_y!=y || nt || surround_space) &&
					(fabsf(parts[i].vx)>0.01f || fabsf(parts[i].vy)>0.01f))
				{
					// allow diagonal movement if target position is blocked
					// but no point trying this if particle is stuck in a block of identical particles
					float dx = parts[i].vx - parts[i].vy*r;
					float dy = parts[i].vy + parts[i].vx*r;
					if (fabsf(dy) > fabsf(dx))
						mv = fabsf(dy);
					else
						mv = fabsf(dx);
					dx /= mv;
					dy /= mv;
					if (DoMove(i, x, y, clear_xf+dx, clear_yf+dy))
					{
						parts[i].vx *= elements[t].Collision;
						parts[i].vy *= elements[t].Collision;
						return false;
					}
					float swappage = dx;
					dx = dy*r;
					dy = -swappage*r;
					if (DoMove(i, x, y, clear_xf+dx, clear_yf+dy))
					{
						parts[i].vx *= elements[t].Collision;
						parts[i].vy *= elements[t].Collision;
						return false;
					}
				}
				if (elements[t].Falldown>1 && !ngrav_enable && gravityMode==0 && parts[i].vy>fabsf(parts[i].vx))
				{
					int rt;
					s = 0;
					// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
					if (!stagnant || nt) //nt is if there is an something else besides the current particle type, around the particle
						rt = 30;//slight less water lag, although it changes how it moves a lot
					else
						rt = 10;

					if (t == PT_GEL)
						rt = (int)(parts[i].tmp*0.20f+5.0f);

					for (int j=clear_x+r; j>=0 && j>=clear_x-rt && j<clear_x+rt && j<XRES; j+=r)
					{
						if (((pmap[fin_y][j]&0xFF)!=t || bmap[fin_y/CELL][j/CELL])
							&& (s=DoMove(i, x, y, (float)j, fin_yf)))
						{
							nx = (int)(parts[i].x+0.5f);
							ny = (int)(parts[i].y+0.5f);
							break;
						}
						if (fin_y!=clear_y && ((pmap[clear_y][j]&0xFF)!=t || bmap[clear_y/CELL][j/CELL])
							&& (s=DoMove(i, x, y, (float)j, clear_yf)))
						{
							nx = (int)(parts[i].x+0.5f);
							ny = (int)(parts[i].y+0.5f);
							break;
						}
						if ((pmap[clear_y][j]&0xFF)!=t || (bmap[clear_y/CELL][j/CELL] && bmap[clear_y/CELL][j/CELL]!=WL_STREAM))
							break;
					}
					if (parts[i].vy > 0)
						r = 1;
					else
						r = -1;
					if (s == 1)
						for (int j=ny+r; j>=0 && j<YRES && j>=ny-rt && j<ny+rt; j+=r)
						{
							if (((pmap[j][nx]&0xFF)!=t || bmap[j/CELL][nx/CELL]) && DoMove(i, nx, ny, (float)nx, (float)j))
								break;
							if ((pmap[j][nx]&255)!=t || (bmap[j/CELL][nx/CELL] && bmap[j/CELL][nx/CELL]!=WL_STREAM))
								break;
						}
					else if (s==-1) {} // particle is out of bounds
					else if ((clear_x!=x||clear_y!=y) && DoMove(i, x, y, clear_xf, clear_yf)) {}
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
				else if (elements[t].Falldown>1 && fabsf(pGravX*parts[i].vx+pGravY*parts[i].vy)>fabsf(pGravY*parts[i].vx-pGravX*parts[i].vy))
				{
					float nxf, nyf, prev_pGravX, prev_pGravY, ptGrav = elements[t].Gravity;
					int rt;
					s = 0;
					// stagnant is true if FLAG_STAGNANT was set for this particle in previous frame
					if (!stagnant || nt) //nt is if there is an something else besides the current particle type, around the particle
						rt = 30;//slight less water lag, although it changes how it moves a lot
					else
						rt = 10;
					// clear_xf, clear_yf is the last known position that the particle should almost certainly be able to move to
					nxf = clear_xf;
					nyf = clear_yf;
					nx = clear_x;
					ny = clear_y;
					// Look for spaces to move horizontally (perpendicular to gravity direction), keep going until a space is found or the number of positions examined = rt
					for (int j = 0; j < rt; j++)
					{
						// Calculate overall gravity direction
						switch (gravityMode)
						{
							default:
							case 0:
								pGravX = 0.0f;
								pGravY = ptGrav;
								break;
							case 1:
								pGravX = pGravY = 0.0f;
								break;
							case 2:
								pGravD = 0.01f - hypotf(((float)nx - XCNTR), ((float)ny - YCNTR));
								pGravX = ptGrav * ((float)(nx - XCNTR) / pGravD);
								pGravY = ptGrav * ((float)(ny - YCNTR) / pGravD);
						}
						pGravX += gravx[(ny/CELL)*(XRES/CELL)+(nx/CELL)];
						pGravY += gravy[(ny/CELL)*(XRES/CELL)+(nx/CELL)];
						// Scale gravity vector so that the largest component is 1 pixel
						if (fabsf(pGravY)>fabsf(pGravX))
							mv = fabsf(pGravY);
						else
							mv = fabsf(pGravX);
						if (mv<0.0001f) break;
						pGravX /= mv;
						pGravY /= mv;
						// Move 1 pixel perpendicularly to gravity
						// r is +1/-1, to try moving left or right at random
						if (j)
						{
							// Not quite the gravity direction
							// Gravity direction + last change in gravity direction
							// This makes liquid movement a bit less frothy, particularly for balls of liquid in radial gravity. With radial gravity, instead of just moving along a tangent, the attempted movement will follow the curvature a bit better.
							nxf += r*(pGravY*2.0f-prev_pGravY);
							nyf += -r*(pGravX*2.0f-prev_pGravX);
						}
						else
						{
							nxf += r*pGravY;
							nyf += -r*pGravX;
						}
						prev_pGravX = pGravX;
						prev_pGravY = pGravY;
						// Check whether movement is allowed
						nx = (int)(nxf+0.5f);
						ny = (int)(nyf+0.5f);
						if (nx<0 || ny<0 || nx>=XRES || ny >=YRES)
							break;
						if ((pmap[ny][nx]&0xFF)!=t || bmap[ny/CELL][nx/CELL])
						{
							s = DoMove(i, x, y, nxf, nyf);
							if (s)
							{
								// Movement was successful
								nx = (int)(parts[i].x+0.5f);
								ny = (int)(parts[i].y+0.5f);
								break;
							}
							// A particle of a different type, or a wall, was found. Stop trying to move any further horizontally unless the wall should be completely invisible to particles.
							if (bmap[ny/CELL][nx/CELL]!=WL_STREAM)
								break;
						}
					}
					if (s == 1)
					{
						// The particle managed to move horizontally, now try to move vertically (parallel to gravity direction)
						// Keep going until the particle is blocked (by something that isn't the same element) or the number of positions examined = rt
						clear_x = nx;
						clear_y = ny;
						for (int j = 0; j < rt; j++)
						{
							// Calculate overall gravity direction
							switch (gravityMode)
							{
								default:
								case 0:
									pGravX = 0.0f;
									pGravY = ptGrav;
									break;
								case 1:
									pGravX = pGravY = 0.0f;
									break;
								case 2:
									pGravD = 0.01f - hypotf(((float)nx - XCNTR), ((float)ny - YCNTR));
									pGravX = ptGrav * ((float)(nx - XCNTR) / pGravD);
									pGravY = ptGrav * ((float)(ny - YCNTR) / pGravD);
							}
							pGravX += gravx[(ny/CELL)*(XRES/CELL)+(nx/CELL)];
							pGravY += gravy[(ny/CELL)*(XRES/CELL)+(nx/CELL)];
							// Scale gravity vector so that the largest component is 1 pixel
							if (fabsf(pGravY)>fabsf(pGravX))
								mv = fabsf(pGravY);
							else
								mv = fabsf(pGravX);
							if (mv<0.0001f) break;
							pGravX /= mv;
							pGravY /= mv;
							// Move 1 pixel in the direction of gravity
							nxf += pGravX;
							nyf += pGravY;
							nx = (int)(nxf+0.5f);
							ny = (int)(nyf+0.5f);
							if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
								break;
							// If the space is anything except the same element (a wall, empty space, or occupied by a particle of a different element), try to move into it
							if ((pmap[ny][nx]&0xFF)!=t || bmap[ny/CELL][nx/CELL])
							{
								s = DoMove(i, clear_x, clear_y, nxf, nyf);
								if (s || bmap[ny/CELL][nx/CELL]!=WL_STREAM)
									break; // found the edge of the liquid and movement into it succeeded, so stop moving down
							}
						}
					}
					else if (s==-1) {} // particle is out of bounds
					else if ((clear_x!=x || clear_y!=y) && DoMove(i, x, y, clear_xf, clear_yf)) {} // try moving to the last clear position
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
				else
				{
					// if interpolation was done, try moving to last clear position
					if ((clear_x!=x || clear_y!=y) && DoMove(i, x, y, clear_xf, clear_yf)) {}
					else parts[i].flags |= FLAG_STAGNANT;
					parts[i].vx *= elements[t].Collision;
					parts[i].vy *= elements[t].Collision;
				}
			}
		}
	}
	return false;
}

/* spark_conductive turns a particle into SPRK and sets ctype, life, and temperature.
 * spark_all does something similar, but behaves correctly for WIRE and INST
 *
 * spark_conductive_attempt and spark_all_attempt do the same thing, except they check whether the particle can actually be sparked (is conductive, has life of zero) first. Remember to check for INSL though. 
 * They return true if the particle was successfully sparked.
*/

void Simulation::spark_all(int i, int x, int y)
{
	if (parts[i].type==PT_WIRE)
		parts[i].ctype = PT_DUST;
	//else if (parts[i].type==PT_INST)
	//	INST_flood_spark(this, x, y);
	else
		spark_conductive(i, x, y);
}
void Simulation::spark_conductive(int i, int x, int y)
{
	int type = parts[i].type;
	part_change_type(i, x, y, PT_SPRK);
	parts[i].ctype = type;
	if (type==PT_WATR)
		parts[i].life = 6;
	else if (type==PT_SLTW)
		parts[i].life = 5;
	else
		parts[i].life = 4;
	if (parts[i].temp < 673.0f && !legacy_enable && (type==PT_METL || type == PT_BMTL || type == PT_BRMT || type == PT_PSCN || type == PT_NSCN || type == PT_ETRD || type == PT_NBLE || type == PT_IRON))
	{
		parts[i].temp = parts[i].temp+10.0f;
		if (parts[i].temp > 673.0f)
			parts[i].temp = 673.0f;
	}
}
bool Simulation::spark_all_attempt(int i, int x, int y)
{
	if ((parts[i].type==PT_WIRE && parts[i].ctype<=0) || (parts[i].type==PT_INST && parts[i].life<=0))
	{
		spark_all(i, x, y);
		return true;
	}
	else if (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS))
	{
		spark_conductive(i, x, y);
		return true;
	}
	return false;
}
bool Simulation::spark_conductive_attempt(int i, int x, int y)
{
	if (!parts[i].life && (elements[parts[i].type].Properties & PROP_CONDUCTS))
	{
		spark_conductive(i, x, y);
		return true;
	}
	return false;
}

/*
 *
 *Functions for creating parts, walls, tools, and deco
 *
 */

int Simulation::CreateParts(int x, int y, int c, int flags, bool fill, Brush* brush)
{
	int f = 0, rx = 0, ry = 0, shape = CIRCLE_BRUSH;
	if (brush)
	{
		rx = brush->GetRadius().X, ry = brush->GetRadius().Y, shape = brush->GetShape();
	}

	if (c == PT_LIGH)
	{
		if (lightningRecreate > 0)
			return 0;
		int newlife = rx + ry;
		if (newlife > 55)
			newlife = 55;
		c = c|newlife<<8;
		lightningRecreate = newlife/4;
		rx = ry = 0;
	}
	else if (c == PT_STKM || c == PT_STKM2 || c == PT_FIGH)
		rx = ry = 0;
	else if (c == PT_TESC)
	{
		int newtmp = (rx*4+ry*4+7);
		if (newtmp > 300)
			newtmp = 300;
		c = c|newtmp<<8;
	}
	else if (c == PT_MOVS)
	{
		if (CreatePartFlags(x, y, c|1<<8, flags) && !((MOVS_ElementDataContainer*)this->elementData[PT_MOVS])->IsCreatingSolid())
			return 1;
		c = c|2<<8;
	}

	if (rx<=0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (int j = y - ry; j <= y + ry; j++)
			if (CreatePartFlags(x, j, c, flags))
				f = 1;
	}
	else
	{
		int tempy = y, i, j, jmax, oldy;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush

		//For triangle brush, start at the very bottom
		if (brush->GetShape() == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			oldy = tempy;
			while (brush->IsInside(i-x,tempy-y))
				tempy = tempy - 1;
			tempy = tempy + 1;
			if (fill)
			{
				//If triangle brush, create parts down to the bottom always; if not go down to the bottom border
				if (brush->GetShape() == TRI_BRUSH)
					jmax = y + ry;
				else
					jmax = 2*y - tempy;

				for (j = jmax; j >= tempy; j--)
				{
					if (CreatePartFlags(i, j, c, flags))
						f = 1;
					//don't create twice in the vertical center line
					if (i!=x && CreatePartFlags(2*x-i, j, c, flags))
						f = 1;
				}
			}
			else
			{
				if ((oldy != tempy && brush->GetShape() != SQUARE_BRUSH) || i == x-rx)
					oldy--;
				for (j = oldy+1; j >= tempy; j--)
				{
					int i2 = 2*x-i, j2 = 2*y-j;
					if (brush->GetShape() == TRI_BRUSH)
						j2 = y+ry;
					if (CreatePartFlags(i, j, c, flags))
						f = 1;
					if (i2 != i && CreatePartFlags(i2, j, c, flags))
						f = 1;
					if (j2 != j && CreatePartFlags(i, j2, c, flags))
						f = 1;
					if (i2 != i && j2 != j && CreatePartFlags(i2, j2, c, flags))
						f = 1;
				}
			}
		}
	}
	return !f;
}

int Simulation::CreatePartFlags(int x, int y, int c, int flags)
{
	//specific delete
	if ((flags&BRUSH_SPECIFIC_DELETE) && !(flags&BRUSH_REPLACEMODE))
	{
		if ((pmap[y][x]&0xFF)== ((ElementTool*)activeTools[2])->GetID() || ((ElementTool*)activeTools[2])->GetID() <= 0)//specific deletion
			part_delete(x, y);
	}
	//delete
	else if (c == 0 && !(flags&BRUSH_REPLACEMODE))
	{
		part_delete(x, y);
	}
	//replace mode
	else if (flags&BRUSH_REPLACEMODE)
	{
		if (x<0 || y<0 || x>=XRES || y>=YRES)
			return 0;
		if (((ElementTool*)activeTools[2])->GetID() > 0 && (pmap[y][x]&0xFF) != ((ElementTool*)activeTools[2])->GetID() && (photons[y][x]&0xFF) != ((ElementTool*)activeTools[2])->GetID())
			return 0;
		if (pmap[y][x])
		{
			part_delete(x, y);
			if (c != 0)
				part_create(-2, x, y, c&0xFF, c>>8);
		}
	}
	//normal draw
	else
		if (part_create(-2, x, y, c&0xFF, c>>8) == -1)
			return 1;
	return 0;
}

void Simulation::CreateLine(int x1, int y1, int x2, int y2, int c, int flags, Brush* brush)
{
	int x, y, dx, dy, sy;
	bool reverseXY = abs(y2-y1) > abs(x2-x1), fill = true;
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			CreateParts(y, x, c, flags, fill, brush);
		else
			CreateParts(x, y, c, flags, fill, brush);
		e += de;
		fill = false;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(brush && brush->GetRadius().X+brush->GetRadius().Y) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					CreateParts(y, x, c, flags, fill, brush);
				else
					CreateParts(x, y, c, flags, fill, brush);
			}
			e -= 1.0f;
		}
	}
}

void Simulation::CreateBox(int x1, int y1, int x2, int y2, int c, int flags)
{
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y2; j >= y1; j--)
		for (int i = x1; i <= x2; i++)
			CreateParts(i, j, c, flags, false);
}

//used for element and prop tool floodfills
int Simulation::FloodFillPmapCheck(int x, int y, unsigned int type)
{
	if (type == 0)
		return !pmap[y][x] && !photons[y][x];
	if (elements[type].Properties&TYPE_ENERGY)
		return (photons[y][x] & 0xFF) == type;
	else
		return (pmap[y][x] & 0xFF) == type;
}

int Simulation::FloodParts(int x, int y, int fullc, int replace, int flags)
{
	unsigned int c = fullc&0xFF;
	int x1, x2;
	int created_something = 0;

	if (replace == -1)
	{
		//if initial flood point is out of bounds, do nothing
		if (c != 0 && (x < CELL || x >= XRES-CELL || y < CELL || y >= YRES-CELL || c == PT_SPRK))
			return 1;
		if (c == 0)
		{
			replace = pmap[y][x]&0xFF;
			if (!replace && !(replace = photons[y][x]&0xFF))
			{
				if (bmap[y/CELL][x/CELL])
					return FloodWalls(x/CELL, y/CELL, WL_ERASE, -1);
				else
					return 0;
				//if ((flags&BRUSH_REPLACEMODE) && ((ElementTool*)activeTools[2])->GetID() != replace)
				//	return 0;
			}
		}
		else
			replace = 0;
	}
	if (c != 0 && IsWallBlocking(x, y, c))
		return 0;

	if (!FloodFillPmapCheck(x, y, replace) || ((flags&BRUSH_SPECIFIC_DELETE) && ((ElementTool*)activeTools[2])->GetID() != replace))
		return 0;

	try
	{
		CoordStack cs;
		cs.push(x, y);

		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			// go left as far as possible
			while (c?x1>CELL:x1>0)
			{
				if (!FloodFillPmapCheck(x1-1, y, replace) || (c != 0 && IsWallBlocking(x1-1, y, c)))
				{
					break;
				}
				x1--;
			}
			// go right as far as possible
			while (c?x2<XRES-CELL-1:x2<XRES-1)
			{
				if (!FloodFillPmapCheck(x2+1, y, replace) || (c != 0 && IsWallBlocking(x2+1, y, c)))
				{
					break;
				}
				x2++;
			}
			// fill span
			for (x=x1; x<=x2; x++)
			{
				if (CreateParts(x, y, fullc, flags, true))
					created_something = 1;
			}

			if (c ? y>CELL : y>0)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y-1, replace) && (c == 0 || !IsWallBlocking(x, y-1, c)))
					{
						cs.push(x, y-1);
					}

			if (c ? y<YRES-CELL-1 : y<YRES-1)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y+1, replace) && (c == 0 || !IsWallBlocking(x, y+1, c)))
					{
						cs.push(x, y+1);
					}
		}
		while (cs.getSize() > 0);
	}
	catch (const CoordStackOverflowException& e)
	{
		(void)e; //ignore compiler warning
		return -1;
	}
	return created_something;
}

void Simulation::CreateWall(int x, int y, int wall)
{
	if (x < 0 || y < 0 || x >= XRES/CELL || y >= YRES/CELL)
		return;

	//reset fan velocity, as if new fan walls had been created
	if (wall == WL_FAN)
	{
		fvx[y][x] = 0.0f;
		fvy[y][x] = 0.0f;
	}
	//streamlines can't be drawn next to each other
	else if (wall == WL_STREAM)
	{
		for (int tempY = y-1; tempY <= y+1; tempY++)
			for (int tempX = x-1; tempX <= x+1; tempX++)
			{
				if (tempX >= 0 && tempX < XRES/CELL && tempY >= 0 && tempY < YRES/CELL && bmap[tempY][tempX] == WL_STREAM)
					return;
			}
	}
	else if (wall == WL_ERASEALL)
	{
		for (int i = 0; i < CELL; i++)
			for (int j = 0; j < CELL; j++)
			{
				part_delete(x*CELL+i, y*CELL+j);
			}
		for (int i = 0; i < MAXSIGNS; i++)
			if (signs[i].text[0])
			{
				if (signs[i].x >= x*CELL && signs[i].y >= y*CELL && signs[i].x <= (x+1)*CELL && signs[i].y <= (y+1)*CELL)
					signs[i].text[0] = 0;
			}
		wall = 0;
	}
	if (wall == WL_GRAV || bmap[y][x] == WL_GRAV)
		gravwl_timeout = 60;
	bmap[y][x] = (unsigned char)wall;
}

void Simulation::CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall)
{
	int x, y, dx, dy, sy;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			CreateWallBox(y-rx, x-ry, y+rx, x+ry, wall);
		else
			CreateWallBox(x-rx, y-ry, x+rx, y+ry, wall);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if ((y1<y2) ? (y<=y2) : (y>=y2))
			{
				if (reverseXY)
					CreateWallBox(y-rx, x-ry, y+rx, x+ry, wall);
				else
					CreateWallBox(x-rx, y-ry, x+rx, y+ry, wall);
			}
			e -= 1.0f;
		}
	}
}

void Simulation::CreateWallBox(int x1, int y1, int x2, int y2, int wall)
{
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y1; j <= y2; j++)
		for (int i = x1; i <= x2; i++)
			CreateWall(i, j, wall);
}

int Simulation::FloodWalls(int x, int y, int wall, int replace)
{
	int x1, x2;
	if (replace == -1)
	{
		if (wall==WL_ERASE || wall==WL_ERASEALL)
		{
			replace = bmap[y][x];
			if (!replace)
				return 0;
		}
		else
			replace = 0;
	}

	if (bmap[y][x] != replace)
		return 1;

	// go left as far as possible
	x1 = x2 = x;
	while (x1 >= 1)
	{
		if (bmap[y][x1-1] != replace)
		{
			break;
		}
		x1--;
	}
	while (x2 < XRES/CELL-1)
	{
		if (bmap[y][x2+1] != replace)
		{
			break;
		}
		x2++;
	}

	// fill span
	for (x=x1; x<=x2; x++)
	{
		CreateWall(x, y, wall);
	}
	// fill children
	if (y >= 1)
		for (x=x1; x<=x2; x++)
			if (bmap[y-1][x] == replace)
				if (!FloodWalls(x, y-1, wall, replace))
					return 0;
	if (y < YRES/CELL-1)
		for (x=x1; x<=x2; x++)
			if (bmap[y+1][x] == replace)
				if (!FloodWalls(x, y+1, wall, replace))
					return 0;
	return 1;
}

int Simulation::CreateTool(int x, int y, int tool, float strength)
{
	if (!InBounds(x, y))
		return -2;
	if (tool == TOOL_HEAT || tool == TOOL_COOL)
	{
		int r = pmap[y][x];
		if (!(r&0xFF))
			r = photons[y][x];
		if (r&0xFF)
		{
			float heatchange;
			if ((r&0xFF) == PT_PUMP || (r&0xFF) == PT_GPMP)
				heatchange = strength*.1f;
			else if ((r&0xFF) == PT_ANIM)
				heatchange = strength;
			else
				heatchange = strength*2.0f;

			if (tool == TOOL_HEAT)
			{
				parts[r>>8].temp = restrict_flt(parts[r>>8].temp + heatchange, MIN_TEMP, MAX_TEMP);
			}
			else if (tool == TOOL_COOL)
			{
				parts[r>>8].temp = restrict_flt(parts[r>>8].temp - heatchange, MIN_TEMP, MAX_TEMP);
			}
			return r>>8;
		}
		else
		{
			return -1;
		}
	}
	else if (tool == TOOL_AIR)
	{
		pv[y/CELL][x/CELL] += strength*.05f;
		return -1;
	}
	else if (tool == TOOL_VAC)
	{
		pv[y/CELL][x/CELL] -= strength*.05f;
		return -1;
	}
	else if (tool == TOOL_PGRV)
	{
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = strength*5.0f;
		return -1;
	}
	else if (tool == TOOL_NGRV)
	{
		gravmap[(y/CELL)*(XRES/CELL)+(x/CELL)] = strength*-5.0f;
		return -1;
	}
	return -1;
}

void Simulation::CreateToolBrush(int x, int y, int tool, float strength, Brush* brush)
{
	int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
	if (rx <= 0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (int j = y - ry; j <= y + ry; j++)
			CreateTool(x, j, tool, strength);
	}
	else
	{
		int tempy = y, i, j, jmax;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush (bottom border of brush)

		//For triangle brush, start at the very bottom
		if (brush->GetShape() == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			//loop up until it finds a point not in the brush
			while (brush->IsInside(i-x,tempy-y))
				tempy = tempy - 1;
			tempy = tempy + 1;

			//If triangle brush, create parts down to the bottom always; if not go down to the bottom border
			if (brush->GetShape() == TRI_BRUSH)
				jmax = y + ry;
			else
				jmax = 2*y - tempy;
			for (j = tempy; j <= jmax; j++)
			{
				CreateTool(i, j, tool, strength);
				//don't create twice in the vertical center line
				if (i != x)
					CreateTool(2*x-i, j, tool, strength);
			}
		}
	}
}

void Simulation::CreateToolLine(int x1, int y1, int x2, int y2, int tool, float strength, Brush* brush)
{
	if (tool == TOOL_WIND)
	{
		int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
		for (int j = -ry; j <= ry; j++)
			for (int i = -rx; i <= rx; i++)
				if (x2+i>0 && y2+j>0 && x2+i<XRES && y2+j<YRES && brush->IsInside(i, j))
				{
					vx[(y2+j)/CELL][(x2+i)/CELL] += (x2-x1)*0.01f;
					vy[(y2+j)/CELL][(x2+i)/CELL] += (y2-y1)*0.01f;
				}
		return;
	}
	int x, y, dx, dy, sy;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			CreateToolBrush(y, x, tool, strength, brush);
		else
			CreateToolBrush(x, y, tool, strength, brush);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(brush->GetRadius().X+brush->GetRadius().Y) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					CreateToolBrush(y, x, tool, strength, brush);
				else
					CreateToolBrush(x, y, tool, strength, brush);
			}
			e -= 1.0f;
		}
	}
}

void Simulation::CreateToolBox(int x1, int y1, int x2, int y2, int tool, float strength)
{
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y1; j <= y2; j++)
		for (int i = x1; i <= x2; i++)
			CreateTool(i, j, tool, strength);
}

int Simulation::CreateProp(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset)
{
	if (!InBounds(x, y))
		return -2;

	int i = pmap[y][x];
	if (!i)
		i = photons[y][x];

	if (i & 0xFF)
	{
		if (propType == Integer)
			*((int*)(((char*)&parts[i>>8]) + propOffset)) = propValue.Integer;
		else if (propType == UInteger)
			*((unsigned int*)(((char*)&parts[i>>8]) + propOffset)) = propValue.UInteger;
		else if (propType == Float)
			*((float*)(((char*)&parts[i>>8]) + propOffset)) = propValue.Float;
		return i >> 8;
	}
	return -1;
}

void Simulation::CreatePropBrush(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset, Brush* brush)
{
	int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
	if (rx <= 0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (int j = y - ry; j <= y + ry; j++)
			CreateProp(x, j, propType, propValue, propOffset);
	}
	else
	{
		int tempy = y, i, j, jmax;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush (bottom border of brush)

		//For triangle brush, start at the very bottom
		if (brush->GetShape() == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			//loop up until it finds a point not in the brush
			while (brush->IsInside(i - x, tempy - y))
				tempy = tempy - 1;
			tempy = tempy + 1;

			//If triangle brush, create parts down to the bottom always; if not go down to the bottom border
			if (brush->GetShape() == TRI_BRUSH)
				jmax = y + ry;
			else
				jmax = 2 * y - tempy;
			for (j = tempy; j <= jmax; j++)
			{
				CreateProp(i, j, propType, propValue, propOffset);
				//don't create twice in the vertical center line
				if (i != x)
					CreateProp(2 * x - i, j, propType, propValue, propOffset);
			}
			//TODO: should use fill argument like creating elements does (but all the repetitiveness here needs to be removed first)
		}
	}
}

void Simulation::CreatePropLine(int x1, int y1, int x2, int y2, PropertyType propType, PropertyValue propValue, size_t propOffset, Brush* brush)
{
	int x, y, dx, dy, sy;
	bool reverseXY = abs(y2 - y1) > abs(x2 - x1);
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy / (float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x = x1; x <= x2; x++)
	{
		if (reverseXY)
			CreatePropBrush(y, x, propType, propValue, propOffset, brush);
		else
			CreatePropBrush(x, y, propType, propValue, propOffset, brush);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(brush->GetRadius().X +  brush->GetRadius().Y) && ((y1<y2) ? (y <= y2) : (y >= y2)))
			{
				if (reverseXY)
					CreatePropBrush(y, x, propType, propValue, propOffset, brush);
				else
					CreatePropBrush(x, y, propType, propValue, propOffset, brush);
			}
			e -= 1.0f;
		}
	}
}

void Simulation::CreatePropBox(int x1, int y1, int x2, int y2, PropertyType propType, PropertyValue propValue, size_t propOffset)
{
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y1; j <= y2; j++)
		for (int i = x1; i <= x2; i++)
			CreateProp(i, j, propType, propValue, propOffset);
}

int Simulation::FloodProp(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset)
{
	int i, x1, x2, dy = 1;
	int did_something = 0;
	int r = pmap[y][x];
	if (!r)
		r = photons[y][x];
	if (!r)
		return 0;
	int parttype = (r&0xFF);
	char *bitmap = new char[XRES*YRES]; //Bitmap for checking
	if (!bitmap)
		return -1;
	std::fill_n(&bitmap[0], XRES*YRES, 0);
	try
	{
		CoordStack cs;
		cs.push(x, y);
		do
		{
			cs.pop(x, y);
			x1 = x2 = x;
			x1 = x2 = x;
			while (x1>=CELL)
			{
				if (!FloodFillPmapCheck(x1-1, y, parttype) || bitmap[(y*XRES)+x1-1])
					break;
				x1--;
			}
			while (x2<XRES-CELL)
			{
				if (!FloodFillPmapCheck(x2+1, y, parttype) || bitmap[(y*XRES)+x2+1])
					break;
				x2++;
			}
			for (x=x1; x<=x2; x++)
			{
				i = pmap[y][x];
				if (!i)
					i = photons[y][x];
				if (!i)
					continue;
				switch (propType) {
					case Float:
						*((float*)(((char*)&parts[i>>8])+propOffset)) = propValue.Float;
						break;

					case ParticleType:
					case Integer:
						*((int*)(((char*)&parts[i>>8])+propOffset)) = propValue.Integer;
						break;

					case UInteger:
						*((unsigned int*)(((char*)&parts[i>>8])+propOffset)) = propValue.UInteger;
						break;

					default:
						break;
				}
				bitmap[(y*XRES)+x] = 1;
				did_something = 1;
			}
			if (y>=CELL+dy)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y-dy, parttype) && !bitmap[((y-dy)*XRES)+x])
						cs.push(x, y-dy);
			if (y<YRES-CELL-dy)
				for (x=x1; x<=x2; x++)
					if (FloodFillPmapCheck(x, y+dy, parttype) && !bitmap[((y+dy)*XRES)+x])
						cs.push(x, y+dy);
		} while (cs.getSize()>0);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		delete[] bitmap;
		return -1;
	}
	delete[] bitmap;
	return did_something;
}

void Simulation::CreateDeco(int x, int y, int tool, ARGBColour colour)
{
	int rp, tr = 0, tg = 0, tb = 0, ta = 0;
	float strength = 0.01f, colr, colg, colb, cola;
	if (!InBounds(x, y))
		return;

	rp = pmap[y][x];
	if (!rp)
		rp = photons[y][x];
	if (!rp)
		return;

	switch (tool)
	{
	case DECO_DRAW:
		parts[rp>>8].dcolour = colour;
		break;
	case DECO_CLEAR:
		parts[rp>>8].dcolour = COLARGB(0, 0, 0, 0);
		break;
	case DECO_ADD:
	case DECO_SUBTRACT:
	case DECO_MULTIPLY:
	case DECO_DIVIDE:
		if (!parts[rp>>8].dcolour)
			return;
		cola = COLA(colour)/255.0f;
		colr = (float)COLR(parts[rp>>8].dcolour);
		colg = (float)COLG(parts[rp>>8].dcolour);
		colb = (float)COLB(parts[rp>>8].dcolour);

		if (tool == DECO_ADD)
		{
			colr += (COLR(colour)*strength)*cola;
			colg += (COLG(colour)*strength)*cola;
			colb += (COLB(colour)*strength)*cola;
		}
		else if (tool == DECO_SUBTRACT)
		{
			colr -= (COLR(colour)*strength)*cola;
			colg -= (COLG(colour)*strength)*cola;
			colb -= (COLB(colour)*strength)*cola;
		}
		else if (tool == DECO_MULTIPLY)
		{
			colr *= 1.0f+(COLR(colour)/255.0f*strength)*cola;
			colg *= 1.0f+(COLG(colour)/255.0f*strength)*cola;
			colb *= 1.0f+(COLB(colour)/255.0f*strength)*cola;
		}
		else if (tool == DECO_DIVIDE)
		{
			colr /= 1.0f+(COLR(colour)/255.0f*strength)*cola;
			colg /= 1.0f+(COLG(colour)/255.0f*strength)*cola;
			colb /= 1.0f+(COLB(colour)/255.0f*strength)*cola;
		}

		tr = int(colr+.5f); tg = int(colg+.5f); tb = int(colb+.5f);
		if (tr > 255) tr = 255;
		else if (tr < 0) tr = 0;
		if (tg > 255) tg = 255;
		else if (tg < 0) tg = 0;
		if (tb > 255) tb = 255;
		else if (tb < 0) tb = 0;

		parts[rp>>8].dcolour = COLRGB(tr, tg, tb);
		break;
	case DECO_LIGHTEN:
		if (!parts[rp>>8].dcolour)
			return;
		tr = COLR(parts[rp>>8].dcolour);
		tg = COLG(parts[rp>>8].dcolour);
		tb = COLB(parts[rp>>8].dcolour);
		ta = COLA(parts[rp>>8].dcolour);
		parts[rp>>8].dcolour = COLARGB(ta, clamp_flt(tr+(255-tr)*0.02f+1, 0, 255), clamp_flt(tg+(255-tg)*0.02f+1, 0, 255), clamp_flt(tb+(255-tb)*0.02f+1, 0, 255));
		break;
	case DECO_DARKEN:
		if (!parts[rp>>8].dcolour)
			return;
		tr = COLR(parts[rp>>8].dcolour);
		tg = COLG(parts[rp>>8].dcolour);
		tb = COLB(parts[rp>>8].dcolour);
		ta = COLA(parts[rp>>8].dcolour);
		parts[rp>>8].dcolour = COLARGB(ta, clamp_flt(tr-(tr)*0.02f, 0, 255), clamp_flt(tg-(tg)*0.02f, 0, 255), clamp_flt(tb-(tb)*0.02f, 0, 255));
		break;
	case DECO_SMUDGE:
		if (x >= CELL && x < XRES-CELL && y >= CELL && y < YRES-CELL)
		{
			int rx, ry, num = 0, ta = 0;
			for (rx=-2; rx<3; rx++)
				for (ry=-2; ry<3; ry++)
				{
					if (abs(rx)+abs(ry) > 2 && (pmap[y+ry][x+rx]&0xFF) && parts[pmap[y+ry][x+rx]>>8].dcolour)
					{
						num++;
						ta += COLA(parts[pmap[y+ry][x+rx]>>8].dcolour);
						tr += COLR(parts[pmap[y+ry][x+rx]>>8].dcolour);
						tg += COLG(parts[pmap[y+ry][x+rx]>>8].dcolour);
						tb += COLB(parts[pmap[y+ry][x+rx]>>8].dcolour);
					}
				}
			if (num == 0)
				return;
			ta = std::min(255,(int)((float)ta/num+.5));
			tr = std::min(255,(int)((float)tr/num+.5));
			tg = std::min(255,(int)((float)tg/num+.5));
			tb = std::min(255,(int)((float)tb/num+.5));
			if (!parts[rp>>8].dcolour)
				ta = std::max(0, ta-3);
			parts[rp>>8].dcolour = COLARGB(ta, tr, tg, tb);
		}
		break;
	}

	if (parts[rp>>8].type == PT_ANIM)
	{
		if (parts[rp>>8].tmp2 >= 0 && parts[rp>>8].tmp2 < maxFrames)
			parts[rp>>8].animations[parts[rp>>8].tmp2] = parts[rp>>8].dcolour;
	}
}

void Simulation::CreateDecoBrush(int x, int y, int tool, ARGBColour colour, Brush* brush)
{
	int rx = brush->GetRadius().X, ry = brush->GetRadius().Y;
	if (rx <= 0) //workaround for rx == 0 crashing. todo: find a better fix later.
	{
		for (int j = y - ry; j <= y + ry; j++)
			CreateDeco(x, j, tool, colour);
	}
	else
	{
		int tempy = y, i, j, jmax;
		// tempy is the smallest y value that is still inside the brush
		// jmax is the largest y value that is still inside the brush (bottom border of brush)

		//For triangle brush, start at the very bottom
		if (brush->GetShape() == TRI_BRUSH)
			tempy = y + ry;
		for (i = x - rx; i <= x; i++)
		{
			//loop up until it finds a point not in the brush
			while (brush->IsInside(i-x,tempy-y))
				tempy = tempy - 1;
			tempy = tempy + 1;

			//If triangle brush, create parts down to the bottom always; if not go down to the bottom border
			if (brush->GetShape() == TRI_BRUSH)
				jmax = y + ry;
			else
				jmax = 2*y - tempy;

			for (j = tempy; j <= jmax; j++)
			{
				CreateDeco(i, j, tool, colour);
				//don't create twice in the vertical center line
				if (i != x)
					CreateDeco(2*x-i, j, tool, colour);
			}
		}
	}
}

void Simulation::CreateDecoLine(int x1, int y1, int x2, int y2, int tool, ARGBColour colour, Brush* brush)
{
	int x, y, dx, dy, sy;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			CreateDecoBrush(y, x, tool, colour, brush);
		else
			CreateDecoBrush(x, y, tool, colour, brush);
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(brush->GetRadius().X+brush->GetRadius().Y) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					CreateDecoBrush(y, x, tool, colour, brush);
				else
					CreateDecoBrush(x, y, tool, colour, brush);
			}
			e -= 1.0f;
		}
	}
}

void Simulation::CreateDecoBox(int x1, int y1, int x2, int y2, int tool, ARGBColour colour)
{
	if (x1 > x2)
	{
		int temp = x2;
		x2 = x1;
		x1 = temp;
	}
	if (y1 > y2)
	{
		int temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (int j = y1; j <= y2; j++)
		for (int i = x1; i <= x2; i++)
			CreateDeco(i, j, tool, colour);
}

void Simulation::FloodDeco(int x, int y, ARGBColour colour, ARGBColour replace)
{
	//TODO: implement
}

/*
 *
 *End of tool creation section
 *
 */

//This is not part of the simulation class
void Simulation_Compat_CopyData(Simulation* sim)
{
	// TODO: this can be removed once all the code uses Simulation instead of global variables
	parts = sim->parts;


	for (int t=0; t<PT_NUM; t++)
	{
		ptypes[t].name = mystrdup(sim->elements[t].Name.c_str());
		ptypes[t].enabled = sim->elements[t].Enabled;
		ptypes[t].gravity = sim->elements[t].Gravity;
		ptypes[t].falldown = sim->elements[t].Falldown;
		ptypes[t].flammable = sim->elements[t].Flammable;
		ptypes[t].explosive = sim->elements[t].Explosive;
		ptypes[t].meltable = sim->elements[t].Meltable;
		ptypes[t].hardness = sim->elements[t].Hardness;
		ptypes[t].weight = sim->elements[t].Weight;
		ptypes[t].heat = sim->elements[t].DefaultProperties.temp;
		ptypes[t].hconduct = sim->elements[t].HeatConduct;
		ptypes[t].descs = mystrdup(sim->elements[t].Description.c_str());
		ptypes[t].state = sim->elements[t].State;
		ptypes[t].properties = sim->elements[t].Properties;
		ptypes[t].graphics_func = sim->elements[t].Graphics;

		ptransitions[t].plt = sim->elements[t].LowPressureTransitionElement;
		ptransitions[t].plv = sim->elements[t].LowPressureTransitionThreshold;
		ptransitions[t].pht = sim->elements[t].HighPressureTransitionElement;
		ptransitions[t].phv = sim->elements[t].HighPressureTransitionThreshold;
		ptransitions[t].tlt = sim->elements[t].LowTemperatureTransitionElement;
		ptransitions[t].tlv = sim->elements[t].LowTemperatureTransitionThreshold;
		ptransitions[t].tht = sim->elements[t].HighTemperatureTransitionElement;
		ptransitions[t].thv = sim->elements[t].HighTemperatureTransitionThreshold;

		platent[t] = sim->elements[t].Latent;
	}
}
