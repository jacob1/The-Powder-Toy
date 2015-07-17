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

#ifndef Simulation_h
#define Simulation_h

#include <cstddef> // offsetof, for FloodProp
#include "graphics/ARGBColour.h"
#include "simulation/Element.h"
#include "powder.h"

// Defines for element transitions
#define IPL -257.0f
#define IPH 257.0f
#define ITL MIN_TEMP-1
#define ITH MAX_TEMP+1
// no transition (PT_NONE means kill part)
#define NT -1
// special transition - lava ctypes etc need extra code, which is only found and run if ST is given
#define ST PT_NUM

class ElementDataContainer;
class Brush;

class Simulation
{
public:
	unsigned int currentTick;

	particle parts[NPART];
	int elementCount[PT_NUM];
	Element elements[PT_NUM];
	ElementDataContainer *elementData[PT_NUM];
	int pfree;
	int parts_lastActiveIndex;
	int debug_currentParticle;
	bool forceStackingCheck;

	// settings
	bool msRotation; //for moving solids
	int maxFrames;   //for animated LCRY
	bool instantActivation; //electronics are instantly activated

	// misc Simulation variables
	unsigned int lightningRecreate; //timer for when LIGH can be created again
	
	Simulation();
	~Simulation();
	void InitElements();
	void InitElement(char* name, int id);
	void Clear();
	void RecountElements();
	int part_create(int p, int x, int y, int t, int v = -1);
	void part_kill(int i);
	void part_delete(int x, int y);
	bool part_change_type(int i, int x, int y, int t);
	void part_change_type_force(int i, int t);

	void RecalcFreeParticles();
	void UpdateBefore();
	void UpdateParticles(int start, int end);
	void UpdateAfter();
	bool UpdateParticle(int i); // called by UpdateParticles
	void Tick();

	void spark_all(int i, int x, int y);
	bool spark_all_attempt(int i, int x, int y);
	void spark_conductive(int i, int x, int y);
	bool spark_conductive_attempt(int i, int x, int y);

	// Element drawing functions
	int CreateParts(int x, int y, int c, int flags, bool fill, Brush* brush = NULL);
	int CreatePartFlags(int x, int y, int c, int flags);
	void CreateLine(int x1, int y1, int x2, int y2, int c, int flags, Brush* brush = NULL);
	void CreateBox(int x1, int y1, int x2, int y2, int c, int flags);
	int FloodFillPmapCheck(int x, int y, unsigned int type);
	int FloodParts(int x, int y, int fullc, int replace, int flags);

	void CreateWall(int x, int y, int wall);
	void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall);
	void CreateWallBox(int x1, int y1, int x2, int y2, int wall);
	int FloodWalls(int x, int y, int wall, int replace);

	int CreateTool(int x, int y, int tool, float strength);
	void CreateToolBrush(int x, int y, int tool, float strength, Brush* brush);
	void CreateToolLine(int x1, int y1, int x2, int y2, int tool, float strength, Brush* brush);
	void CreateToolBox(int x1, int y1, int x2, int y2, int tool, float strength);

	int CreateProp(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset);
	void CreatePropBrush(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset, Brush* brush);
	void CreatePropLine(int x1, int y1, int x2, int y2, PropertyType propType, PropertyValue propValue, size_t propOffset, Brush* brush);
	void CreatePropBox(int x1, int y1, int x2, int y2, PropertyType propType, PropertyValue propValue, size_t propOffset);
	int FloodProp(int x, int y, PropertyType propType, PropertyValue propValue, size_t propOffset);

	void CreateDeco(int x, int y, int tool, ARGBColour colour);
	void CreateDecoBrush(int x, int y, int tool, ARGBColour colour, Brush* brush);
	void CreateDecoLine(int x1, int y1, int x2, int y2, int tool, ARGBColour colour, Brush* brush);
	void CreateDecoBox(int x1, int y1, int x2, int y2, int tool, ARGBColour colour);
	void FloodDeco(int x, int y, ARGBColour colour, ARGBColour replace);

	// movement, functions implemented in Movement.cpp
	unsigned char can_move[PT_NUM][PT_NUM];
	bool OutOfBounds(int x, int y);
	bool IsWallBlocking(int x, int y, int type);
	bool GetNormalInterp(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);
	void InitCanMove();
	unsigned char EvalMove(int pt, int nx, int ny, unsigned *rr = NULL);
	int DoMove(int i, int x, int y, float nxf, float nyf);
	int Move(int i, int x, int y, float nxf, float nyf);

	// Functions defined here should hopefully be inlined
	// Don't put anything that will change often here, since changes cause a lot of recompiling
	bool IsElement(int t) const
	{
		return (t>0 && t<PT_NUM && elements[t].Enabled);
	}
	bool InBounds(int x, int y)
	{
		return (x>=0 && y>=0 && x<XRES && y<YRES);
	}

	// Most of the time, part_alloc and part_free should not be used directly unless you really know what you're doing. 
	// Use part_create and part_kill instead.
	int part_alloc()
	{
		if (pfree == -1)
			return -1;
		int i = pfree;
		pfree = parts[i].life;
		if (i>parts_lastActiveIndex)
			parts_lastActiveIndex = i;
		return i;
	}
	void part_free(int i)
	{
		parts[i].type = 0;
		parts[i].life = pfree;
		pfree = i;
	}
	void pmap_add(int i, int x, int y, int t)
	{
		// NB: all arguments are assumed to be within bounds
		if (elements[t].Properties & TYPE_ENERGY)
			photons[y][x] = t|(i<<8);
		else if ((!pmap[y][x] || (t!=PT_INVIS && t!= PT_FILT)))// && (pmap[y][x]&0xFF) != PT_PINV)
			pmap[y][x] = t|(i<<8);
	}
	void pmap_remove(unsigned int i, int x, int y)
	{
		// NB: all arguments are assumed to be within bounds
		if ((pmap[y][x]>>8)==i)
			pmap[y][x] = 0;
		else if ((pmap[y][x]&0xFF)==PT_PINV && (parts[pmap[y][x]>>8].tmp2>>8)==i)
			parts[pmap[y][x]>>8].tmp2 = 0;
		else if ((photons[y][x]>>8)==i)
			photons[y][x] = 0;
	}

private:
	// some movement functions are private
	void CreateGainPhoton(int pp);
	void CreateCherenkovPhoton(int pp);
	void PhotoelectricEffect(int nx, int ny);
	unsigned DirectionToMap(float dx, float dy, int t);
	bool IsBlocking(int t, int x, int y);
	bool IsBoundary(int pt, int x, int y);
	bool FindNextBoundary(int pt, int *x, int *y, int dm, int *em);
	bool GetNormal(int pt, int x, int y, float dx, float dy, float *nx, float *ny);

	int TryMove(int i, int x, int y, int nx, int ny);
};

void Simulation_Compat_CopyData(Simulation *sim);

extern Simulation *globalSim; // TODO: remove this

#endif
