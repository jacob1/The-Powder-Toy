/**
 * Powder Toy - air simulation
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

#include <cmath>
#include <cstring> // memcpy
#include "simulation/Air.h"
#include "defines.h"
#include "simulation/Simulation.h"
#include "simulation/WallNumbers.h"

// Used when updating temp or velocity from far away
const float advDistanceMult = 0.7f;

Air::Air()
{
	MakeKernel();
	ambientAirTemp = R_TEMP + 273.15;
	ambientAirTempPref = R_TEMP + 273.15;

	Clear();
}

// Kernel, used for velocity
void Air::MakeKernel()
{
	float s = 0.0f;
	for (int j = -1; j <= 1; j++)
		for (int i = -1; i <= 1; i++)
		{
			kernel[(i+1)+3*(j+1)] = expf(-2.0f*(i*i+j*j));
			s += kernel[(i+1)+3*(j+1)];
		}
	s = 1.0f / s;
	for (int j = -1; j <= 1; j++)
		for (int i = -1; i <= 1; i++)
			kernel[(i+1)+3*(j+1)] *= s;
}

void Air::Clear()
{
	ClearPresVel();
	std::fill(&fvx[0][0], &fvx[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&fvy[0][0], &fvy[0][0]+((XRES/CELL)*(YRES/CELL)), 0.0f);
	std::fill(&blockair[0][0], &blockair[0][0]+((XRES/CELL)*(YRES/CELL)), 0);
	std::fill(&blockairh[0][0], &blockairh[0][0]+((XRES/CELL)*(YRES/CELL)), 0);

	float airTemp = GetAmbientAirTemp();
	for (int x = 0; x < XRES/CELL; x++)
	{
		for (int y = 0; y < YRES/CELL; y++)
		{
			hv[y][x] = airTemp;
		}
	}

	saveConvectionMode = -1;
}

void Air::ClearPresVel()
{
	std::fill(&pv[0][0], &pv[0][0]+((XRES/CELL)*(YRES/CELL)), edgePressure);
	std::fill(&vx[0][0], &vx[0][0]+((XRES/CELL)*(YRES/CELL)), edgeVelocityX);
	std::fill(&vy[0][0], &vy[0][0]+((XRES/CELL)*(YRES/CELL)), edgeVelocityY);
}

void Air::ClearAirH()
{
	std::fill(&hv[0][0], &hv[0][0]+((XRES/CELL)*(YRES/CELL)), GetAmbientAirTemp());
}

void Air::ClearTemporarySettings()
{
	// Reset settings from loaded saves back to their user-set defaults
	this->ambientAirTemp = this->ambientAirTempPref;
	this->edgePressure = this->edgePressurePref;
	this->edgeVelocityX = this->edgeVelocityXPref;
	this->edgeVelocityY = this->edgeVelocityYPref;
	this->vorticityCoeff = this->vorticityCoeffPref;
}

void Air::UpdateAirHeat(Simulation *sim)
{
	if (!aheat_enable)
		return;

	float ambientAirTemp = GetAmbientAirTemp();

	// Set ambient heat temp on the edges every frame
	for (int i = 0; i < YRES/CELL; i++)
	{
		hv[i][0] = ambientAirTemp;
		hv[i][1] = ambientAirTemp;
		hv[i][XRES/CELL-2] = ambientAirTemp;
		hv[i][XRES/CELL-1] = ambientAirTemp;
	}

	// Set ambient heat temp on the edges every frame
	for (int i = 0; i < XRES/CELL; i++)
	{
		hv[0][i] = ambientAirTemp;
		hv[1][i] = ambientAirTemp;
		hv[YRES/CELL-2][i] = ambientAirTemp;
		hv[YRES/CELL-1][i] = ambientAirTemp;
	}

	float dh, dx, dy;
	float f;
	// Update ambient heat
	for (int y = 0; y < YRES/CELL; y++)
	{
		for (int x = 0; x < XRES/CELL; x++)
		{
			dh = 0.0f;
			dx = 0.0f;
			dy = 0.0f;
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					if (y + j > 0 && y + j < YRES / CELL - 1 && x + i > 0 && x + i < XRES / CELL - 1 && !(blockairh[y + j][x + i] & 0x8))
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y+j][x+i]*f;
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dh += hv[y][x]*f;
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
					}
				}
			}

			// Trying to take air temp from far away.
			// The code is almost identical to the "far away" velocity code from update_air
			auto tx = x - dx*advDistanceMult;
			auto ty = y - dy*advDistanceMult;
			if ((std::abs(dx*advDistanceMult) > 1.0f || std::abs(dy*advDistanceMult) > 1.0f) && (tx>=2 && tx<XCELLS-2 && ty>=2 && ty<YCELLS-2))
			{
				float stepX, stepY;
				int stepLimit;
				if (std::abs(dx)>std::abs(dy))
				{
					stepX = (dx<0.0f) ? 1.f : -1.f;
					stepY = -dy/fabsf(dx);
					stepLimit = (int)(fabsf(dx*advDistanceMult));
				}
				else
				{
					stepY = (dy<0.0f) ? 1.f : -1.f;
					stepX = -dx/fabsf(dy);
					stepLimit = (int)(fabsf(dy*advDistanceMult));
				}
				tx = float(x);
				ty = float(y);
				auto step = 0;
				for (; step<stepLimit; ++step)
				{
					tx += stepX;
					ty += stepY;
					if (!InCellBounds(int(tx+0.5f), int(ty+0.5f)) || blockairh[(int)(ty+0.5f)][(int)(tx+0.5f)]&0x8)
					{
						tx -= stepX;
						ty -= stepY;
						break;
					}
				}
				if (step==stepLimit)
				{
					// No wall found
					tx = x - dx*advDistanceMult;
					ty = y - dy*advDistanceMult;
				}
			}
			auto i = (int)tx;
			auto j = (int)ty;
			tx -= i;
			ty -= j;
			if (!(blockairh[y][x]&0x8) && i>=0 && i<XCELLS-1 && j>=0 && j<YCELLS-1 && tx >= 0.0f && ty >= 0.0f)
			{
				auto odh = dh;
				dh *= 1.0f - AIR_VADV;
				dh += AIR_VADV*(1.0f-tx)*(1.0f-ty)*((blockairh[j][i]&0x8) ? odh : hv[j][i]);
				dh += AIR_VADV*tx*(1.0f-ty)*((blockairh[j][i+1]&0x8) ? odh : hv[j][i+1]);
				dh += AIR_VADV*(1.0f-tx)*ty*((blockairh[j+1][i]&0x8) ? odh : hv[j+1][i]);
				dh += AIR_VADV*tx*ty*((blockairh[j+1][i+1]&0x8) ? odh : hv[j+1][i+1]);
			}

			// Don't update if the current cell blocks ambient heat
			if (blockairh[y][x] & 0x8)
				dh = hv[y][x];

			// Temp caps
			if (dh > MAX_TEMP) dh = MAX_TEMP;
			if (dh < MIN_TEMP) dh = MIN_TEMP;

			ohv[y][x] = dh;

			// Air convection.
			float dvx = vx[y][x];
			float dvy = vy[y][x];

			if (x>=2 && x<XCELLS-2 && y>=2 && y<YCELLS-2)
			{
				float convGravX, convGravY;
				sim->GetGravityField(x*CELL, y*CELL, -1.0f, -1.0f, convGravX, convGravY);

				switch (GetConvectionMode())
				{
				case AIRC_LEGACY:
				{
					// Air convection pre 99.0
					auto weight = ((hv[y][x] - hv[y][x-1]) * convGravX + (hv[y][x] - hv[y-1][x]) * convGravY) / 5000.0f;
					if (weight > 0 && !(blockairh[y-1][x]&0x8))
					{
						dvx += weight * convGravX;
						dvy += weight * convGravY;
					}

					break;
				}
				case AIRC_BOUSSINESQ:
				{
					// Boussinesq approximation, i.e. we assume density to be nonconstant only
					// near the gravity term of the fluid equation, and we suppose that it depends linearly on the
					// difference between the current temperature (hv[y][x]) and some "stationary" temperature (ambientAirTemp).

					// Cap the gravity field
					float gravMagn = std::sqrt(convGravX*convGravX + convGravY*convGravY);
					if (gravMagn > 10.0f)
					{
						convGravX /= 0.1f*gravMagn;
						convGravY /= 0.1f*gravMagn;
					}

					auto weight = (hv[y][x] - ambientAirTemp) / 10000.0f;

					// Our approximation works best when the temperature difference is small, so we cap it from above.
					if (weight > 0.01f) weight = 0.01f;

					dvx += weight * convGravX;
					dvy += weight * convGravY;

					break;
				}
				default:
					break;
				}
			}

			// Velocity cap
			if (dvx > MAX_PRESSURE) dvx = MAX_PRESSURE;
			if (dvx < MIN_PRESSURE) dvx = MIN_PRESSURE;
			if (dvy > MAX_PRESSURE) dvy = MAX_PRESSURE;
			if (dvy < MIN_PRESSURE) dvy = MIN_PRESSURE;

			vx[y][x] = dvx;
			vy[y][x] = dvy;
		}
	}
	memcpy(hv, ohv, sizeof(hv));
}

static float Mix(float a, float b, float f)
{
	return a + (b - a) * f;
}

void Air::UpdateAir()
{
	// "No Update"
	if (airMode == AIR_NOUPDATE)
		return;

	// Reduces pressure/velocity on the edges every frame
	for (int i = 0; i < YRES/CELL; i++)
	{
		pv[i][       0] = Mix(edgePressure , pv[i][       0], 0.8f);
		pv[i][       1] = Mix(edgePressure , pv[i][       1], 0.8f);
		pv[i][XCELLS-2] = Mix(edgePressure , pv[i][XCELLS-2], 0.8f);
		pv[i][XCELLS-1] = Mix(edgePressure , pv[i][XCELLS-1], 0.8f);
		vx[i][       0] = Mix(edgeVelocityX, vx[i][       0], 0.9f);
		vx[i][       1] = Mix(edgeVelocityX, vx[i][       1], 0.9f);
		vx[i][XCELLS-2] = Mix(edgeVelocityX, vx[i][XCELLS-2], 0.9f);
		vx[i][XCELLS-1] = Mix(edgeVelocityX, vx[i][XCELLS-1], 0.9f);
		vy[i][       0] = Mix(edgeVelocityY, vy[i][       0], 0.9f);
		vy[i][       1] = Mix(edgeVelocityY, vy[i][       1], 0.9f);
		vy[i][XCELLS-2] = Mix(edgeVelocityY, vy[i][XCELLS-2], 0.9f);
		vy[i][XCELLS-1] = Mix(edgeVelocityY, vy[i][XCELLS-1], 0.9f);
	}

	// Reduces pressure/velocity on the edges every frame
	for (int i = 0; i < XRES/CELL; i++)
	{
		pv[       0][i] = Mix(edgePressure , pv[       0][i], 0.8f);
		pv[       1][i] = Mix(edgePressure , pv[       1][i], 0.8f);
		pv[YCELLS-2][i] = Mix(edgePressure , pv[YCELLS-2][i], 0.8f);
		pv[YCELLS-1][i] = Mix(edgePressure , pv[YCELLS-1][i], 0.8f);
		vx[       0][i] = Mix(edgeVelocityX, vx[       0][i], 0.9f);
		vx[       1][i] = Mix(edgeVelocityX, vx[       1][i], 0.9f);
		vx[YCELLS-2][i] = Mix(edgeVelocityX, vx[YCELLS-2][i], 0.9f);
		vx[YCELLS-1][i] = Mix(edgeVelocityX, vx[YCELLS-1][i], 0.9f);
		vy[       0][i] = Mix(edgeVelocityY, vy[       0][i], 0.9f);
		vy[       1][i] = Mix(edgeVelocityY, vy[       1][i], 0.9f);
		vy[YCELLS-2][i] = Mix(edgeVelocityY, vy[YCELLS-2][i], 0.9f);
		vy[YCELLS-1][i] = Mix(edgeVelocityY, vy[YCELLS-1][i], 0.9f);
	}

	// Clear some velocities near walls
	for (int j = 1; j < YRES / CELL - 1; j++)
	{
		for (int i = 1; i < XRES / CELL - 1; i++)
		{
			if (blockair[j][i])
			{
				vx[j][i] = 0.0f;
				vx[j][i-1] = 0.0f;
				vx[j][i+1] = 0.0f;
				vy[j][i] = 0.0f;
				vy[j-1][i] = 0.0f;
				vy[j+1][i] = 0.0f;
			}
		}
	}

	// Pressure adjustments from velocity
	for (int y = 1; y < YRES / CELL - 1; y++)
		for (int x = 1; x < XRES / CELL - 1; x++)
		{
			float dp = (vx[y][x-1] - vx[y][x+1]) + (vy[y-1][x] - vy[y+1][x]);
			pv[y][x] = Mix(edgePressure, pv[y][x], AIR_PLOSS);
			pv[y][x] += dp * AIR_TSTEPP * 0.5f;
		}

	// Velocity adjustments from pressure
	for (int y = 1; y < YRES/CELL-1; y++)
		for (int x = 1; x < XRES/CELL-1; x++)
		{
			float dx = pv[y][x-1] - pv[y][x+1];
			float dy = pv[y-1][x] - pv[y+1][x];
			vx[y][x] = Mix(edgeVelocityX, vx[y][x], AIR_VLOSS);
			vy[y][x] = Mix(edgeVelocityY, vy[y][x], AIR_VLOSS);
			vx[y][x] += dx * AIR_TSTEPV * 0.5f;
			vy[y][x] += dy * AIR_TSTEPV * 0.5f;
			if (blockair[y][x-1] || blockair[y][x] || blockair[y][x+1])
				vx[y][x] = 0;
			if (blockair[y-1][x] || blockair[y][x] || blockair[y+1][x])
				vy[y][x] = 0;
		}

	const float advDistanceMult = 0.7f;
	float dp, dx, dy;
	float f;
	float txf, tyf;
	int txi, tyi;
	float stepX, stepY;
	int stepLimit, step;
	// Update velocity and pressure
	for (int y = 0; y < YRES/CELL; y++)
		for (int x = 0; x < XRES/CELL; x++)
		{
			dx = 0.0f;
			dy = 0.0f;
			dp = 0.0f;
			for (int j = -1; j <= 1; j++)
				for (int i = -1; i <= 1; i++)
					if (y+j>0 && y+j<YRES/CELL-1 &&
							x+i>0 && x+i<XRES/CELL-1 &&
							!blockair[y+j][x+i])
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
						dp += pv[y+j][x+i]*f;
					}
					else
					{
						f = kernel[i+1+(j+1)*3];
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
						dp += pv[y][x]*f;
					}


			txf = x - dx * advDistanceMult;
			tyf = y - dy * advDistanceMult;
			if ((std::abs(dx * advDistanceMult) > 1.0f || std::abs(dy * advDistanceMult) > 1.0f) && (txf >= 2 && txf < XRES/CELL-2 && tyf >= 2 && tyf < YRES/CELL-2))
			{
				// Trying to take velocity from far away, check whether there is an intervening wall. Step from current position to desired source location, looking for walls, with either the x or y step size being 1 cell
				if (std::abs(dx) > std::abs(dy))
				{
					stepX = (dx < 0.0f) ? 1.0f : -1.0f;
					stepY = -dy / std::abs(dx);
					stepLimit = (int)(std::abs(dx * advDistanceMult));
				}
				else
				{
					stepY = (dy < 0.0f) ? 1.0f : -1.0f;
					stepX = -dx / std::abs(dy);
					stepLimit = (int)(std::abs(dy * advDistanceMult));
				}
				txf = (float)x;
				tyf = (float)y;
				for (step = 0; step < stepLimit; ++step)
				{
					txf += stepX;
					tyf += stepY;
					if (!InCellBounds((int)(tyf+0.5f), (int)(txf+0.5f)) || blockair[(int)(tyf+0.5f)][(int)(txf+0.5f)])
					{
						txf -= stepX;
						tyf -= stepY;
						break;
					}
				}
				if (step == stepLimit)
				{
					// No wall found
					txf = x - dx * advDistanceMult;
					tyf = y - dy * advDistanceMult;
				}
			}
			txi = (int)txf;
			tyi = (int)tyf;
			txf -= txi;
			tyf -= tyi;
			if (!blockair[y][x] && txi >= 2 && txi < XRES/CELL-3 && tyi >= 2 && tyi < YRES/CELL-3 && txf >= 0.0f && tyf >= 0.0f)
			{
				dx *= 1.0f - AIR_VADV;
				dy *= 1.0f - AIR_VADV;

				dx += AIR_VADV * (1.0f-txf) * (1.0f-tyf) * vx[tyi][txi];
				dy += AIR_VADV * (1.0f-txf) * (1.0f-tyf) * vy[tyi][txi];

				dx += AIR_VADV * txf * (1.0f-tyf) * vx[tyi][txi+1];
				dy += AIR_VADV * txf * (1.0f-tyf) * vy[tyi][txi+1];

				dx += AIR_VADV * (1.0f-txf) * tyf * vx[tyi+1][txi];
				dy += AIR_VADV * (1.0f-txf) * tyf * vy[tyi+1][txi];

				dx += AIR_VADV * txf * tyf * vx[tyi+1][txi+1];
				dy += AIR_VADV * txf * tyf * vy[tyi+1][txi+1];
			}

			// Vorticity confinement
			if (vorticityCoeff > 0.0f && x > 1 && x < XCELLS - 2 && y > 1 && y < YCELLS - 2)
			{
				auto dwx = (std::abs(vorticity(this, y, x + 1)) - std::abs(vorticity(this, y, x - 1))) * 0.5f;
				auto dwy = (std::abs(vorticity(this, y + 1, x)) - std::abs(vorticity(this, y - 1, x))) * 0.5f;
				auto norm = std::hypot(dwx, dwy);
				auto w = vorticity(this, y, x);

				dx += vorticityCoeff / 5.0f * dwy / (norm + 0.001f) * w;
				dy += vorticityCoeff / 5.0f * (-dwx) / (norm + 0.001f) * w;
			}

			if (bmap[y][x] == WL_FAN)
			{
				dx += fvx[y][x];
				dy += fvy[y][x];
			}

			// pressure/velocity caps
			if (dp > 256.0f)
				dp = 256.0f;
			else if (dp < -256.0f)
				dp = -256.0f;

			if (dx > 256.0f)
				dx = 256.0f;
			else if (dx < -256.0f)
				dx = -256.0f;

			if (dy > 256.0f)
				dy = 256.0f;
			else if (dy < -256.0f)
				dy = -256.0f;

			switch (airMode)
			{
			// Default
			default:
			case AIR_ON:
				break;
			// "Pressure off"
			case AIR_PRESSUREOFF:
				dp = 0.0f;
				break;
			// "Velocity off"
			case AIR_VELOCITYOFF:
				dx = 0.0f;
				dy = 0.0f;
				break;
			// "Off"
			case AIR_OFF:
				dx = 0.0f;
				dy = 0.0f;
				dp = 0.0f;
				break;
			}

			ovx[y][x] = dx;
			ovy[y][x] = dy;
			opv[y][x] = dp;
		}
	memcpy(vx, ovx, sizeof(vx));
	memcpy(vy, ovy, sizeof(vy));
	memcpy(pv, opv, sizeof(pv));
}

// called when loading saves / stamps to ensure nothing "leaks" the first frame
// copied from tpt++
// turns out ... it was only a tpt++ bug. This mod updates air after the simulation, so TTAN sets wallmap blocking properly on save loads
// commented out for now, but left in for consistency in case air updates are moved later
void Air::RecalculateBlockAirMaps(Simulation * sim)
{
	/*for (int i = 0; i <= sim->parts_lastActiveIndex; i++)
	{
		int type = sim->parts[i].type;
		if (!type)
			continue;
		// Real TTAN would only block if there was enough TTAN
		// but it would be more expensive and complicated to actually check that
		// so just block for a frame, if it wasn't supposed to block it will continue allowing air next frame
		if (type == PT_TTAN)
		{
			int x = ((int)(sim->parts[i].x+0.5f))/CELL, y = ((int)(sim->parts[i].y+0.5f))/CELL;
			if (sim->InBounds(x, y))
			{
				bmap_blockair[y][x] = 1;
				bmap_blockairh[y][x] = 0x8;
			}
		}
		// mostly accurate insulator blocking, besides checking GEL
		else if (sim->IsHeatInsulator(parts[i]) || sim->elements[type].HeatConduct <= (rand()%250))
		{
			int x = ((int)(sim->parts[i].x+0.5f))/CELL, y = ((int)(sim->parts[i].y+0.5f))/CELL;
			if (sim->InBounds(x, y) && !(bmap_blockairh[y][x]&0x8))
				bmap_blockairh[y][x]++;
		}
	}*/
}

void Air::SetAmbientAirTemp(float ambientAirTemp)
{
	this->ambientAirTemp = ambientAirTemp;
}

void Air::SetAmbientAirTempPref(float ambientAirTemp)
{
	this->ambientAirTemp = ambientAirTemp;
	this->ambientAirTempPref = ambientAirTemp;
}

float Air::GetAmbientAirTemp()
{
	return ambientAirTemp;
}

float Air::GetAmbientAirTempPref()
{
	return ambientAirTempPref;
}

void Air::SetEdgePressure(float edgePressure)
{
	this->edgePressure = edgePressure;
}

void Air::SetEdgePressurePref(float edgePressure)
{
	this->edgePressure = edgePressure;
	this->edgePressurePref = edgePressure;
}

float Air::GetEdgePressure()
{
	return edgePressure;
}

float Air::GetEdgePressurePref()
{
	return edgePressurePref;
}

void Air::SetEdgeVelocity(float edgeVelocityX, float edgeVelocityY)
{
	this->edgeVelocityX = edgeVelocityX;
	this->edgeVelocityY = edgeVelocityY;
}

void Air::SetEdgeVelocityPref(float edgeVelocityX, float edgeVelocityY)
{

	this->edgeVelocityX = edgeVelocityX;
	this->edgeVelocityY = edgeVelocityY;
	this->edgeVelocityXPref = edgeVelocityX;
	this->edgeVelocityYPref = edgeVelocityY;
}

float Air::GetEdgeVelocityX()
{
	return edgeVelocityX;
}

float Air::GetEdgeVelocityPrefX()
{
	return edgeVelocityXPref;
}

float Air::GetEdgeVelocityY()
{
	return edgeVelocityY;
}

float Air::GetEdgeVelocityPrefY()
{
	return edgeVelocityYPref;
}

float Air::vorticity(const Air * air, int y, int x)
{
	if (x > 1 && x < XCELLS - 2 && y > 1 && y < YCELLS - 2)
	{
		// dvy/dx - dvx/dy
		auto dvydx = (air->blockair[y][x] || air->blockair[y][x+1] || air->blockair[y][x-1]) ? 0.0f : air->vy[y][x+1] - air->vy[y][x-1];
		auto dvxdy = (air->blockair[y][x] || air->blockair[y+1][x] || air->blockair[y-1][x]) ? 0.0f : air->vx[y+1][x] - air->vx[y-1][x];
		return (dvydx - dvxdy)*0.5f;
	}
	else
	{
		return 0.0f;
	}
}

float Air::GetVorticityCoeff()
{
	return vorticityCoeff;
}

float Air::GetVorticityCoeffPref()
{
	return vorticityCoeffPref;
}

void Air::SetVorticityCoeff(float vorticityCoeff)
{
	this->vorticityCoeff = vorticityCoeff;
}

void Air::SetVorticityCoeffPref(float vorticityCoeff)
{
	this->vorticityCoeff = vorticityCoeff;
	this->vorticityCoeffPref = vorticityCoeff;
}

int Air::GetConvectionMode()
{
	return saveConvectionMode == -1 ? convectionMode : saveConvectionMode;
}

void Air::SetConvectionMode(int convectionMode)
{
	this->convectionMode = convectionMode;
	saveConvectionMode = -1;
}

void Air::SetTempConvectionMode(int convectionMode)
{
	saveConvectionMode = convectionMode;
}
