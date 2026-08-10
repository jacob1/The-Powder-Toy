/**
 * Powder Toy - air simulation (header)
 *
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

#ifndef AIR_H
#define AIR_H

#include "defines.h"
#include "SimulationData.h"

class Simulation;

class Air
{
	// used to calculate & store new air maps off of the old ones
	float opv[YRES/CELL][XRES/CELL];
	float ovx[YRES/CELL][XRES/CELL];
	float ovy[YRES/CELL][XRES/CELL];

	float ambientAirTemp;
	float ambientAirTempPref;
	float edgePressure = 0.0f;
	float edgeVelocityX = 0.0f;
	float edgeVelocityY = 0.0f;
	float edgePressurePref = 0.0f;
	float edgeVelocityXPref = 0.0f;
	float edgeVelocityYPref = 0.0f;

public:
	int airMode = AIR_ON;
	float vorticityCoeff = 0.0f;
	float vorticityCoeffPref = 0.1f;
	int convectionMode = AIRC_BOUSSINESQ;
	int saveConvectionMode = -1;

	float pv[YRES/CELL][XRES/CELL];
	float vx[YRES/CELL][XRES/CELL];
	float vy[YRES/CELL][XRES/CELL];
	unsigned char blockair[YRES/CELL][XRES/CELL];
	unsigned char blockairh[YRES/CELL][XRES/CELL];

	// Fan velocity
	float fvx[YRES/CELL][XRES/CELL], fvy[YRES/CELL][XRES/CELL];

	// Ambient Heat
	float hv[YRES/CELL][XRES/CELL], ohv[YRES/CELL][XRES/CELL];

	float kernel[9];

	Air();
	void MakeKernel();
	
	void Clear();
	void ClearPresVel();
	void ClearAirH();
	void ClearTemporarySettings();

	void UpdateAirHeat(Simulation * sim);
	void UpdateAir();

	void RecalculateBlockAirMaps(Simulation * sim);

	void SetAmbientAirTemp(float ambientAirTemp);
	void SetAmbientAirTempPref(float ambientAirTemp);
	float GetAmbientAirTemp();
	float GetAmbientAirTempPref();

	void SetEdgePressure(float edgePressure);
	void SetEdgePressurePref(float edgePressure);
	float GetEdgePressure();
	float GetEdgePressurePref();

	void SetEdgeVelocity(float edgeVelocityX, float edgeVelocityY);
	void SetEdgeVelocityPref(float edgeVelocityX, float edgeVelocityY);
	float GetEdgeVelocityX();
	float GetEdgeVelocityPrefX();
	float GetEdgeVelocityY();
	float GetEdgeVelocityPrefY();

	static float vorticity(const Air * air, int y, int x);
	float GetVorticityCoeff();
	float GetVorticityCoeffPref();
	void SetVorticityCoeff(float vorticityCoeff);
	void SetVorticityCoeffPref(float vorticityCoeff);

	int GetConvectionMode();
	void SetConvectionMode(int convectionMode);
	void SetTempConvectionMode(int convectionMode);

	bool InCellBounds(int x, int y)
	{
		return (x >= 0 && y >= 0 && x < XRES / CELL && y < YRES / CELL);
	}
};

#endif
