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
#include "PLNT.h"

// Fields used (see also PLNT.cpp):
// ctype, tmp-tmp4 have the same meaning as in PLNT,
// but the first bit of ctype is used to determine whether a seed bred with another seed or not.
// life is used as a timer.

int SEED_update(UPDATE_FUNC_ARGS)
{
	auto temp = parts[i].temp;

	// Turn into liquid wax under high temperature and pressure
	if (temp > 320.0f && sim->air->pv[y/CELL][x/CELL] > 50.0f && RNG::Ref().chance(1, 150))
	{
		sim->part_create(i, x, y, PT_MWAX);

		parts[i].ctype = 0;
		parts[i].life = 0;
		parts[i].tmp = 0;
		parts[i].tmp2 = 0;
		parts[i].tmp3 = 0;
		parts[i].tmp4 = 0;

		return 0;
	}

	// Don't do anything at bad temperatures
	if (temp > 343.15f || temp < 278.15f)
	{
		parts[i].ctype &= ~(0xFF << PLNT_LIFE); // Set water to zero
		parts[i].life = 0;

		return 0;
	}

	int water = (parts[i].ctype >> PLNT_LIFE) & 0xFF;

	// Check if we can grow
	if (water > 3)
	{
		int down = detectDown(sim, x, y); // Down gravity direction

		// Set a default direction if can't decide using gravity
		if (down < 0 || down > 7)
			down = 4;

		int up = (down+4)%8;

		// Check if there's SAND/SPNG under the seed particle and empty space above
		unsigned int downPart = pmap[y+dir3x3[down][1]][x+dir3x3[down][0]];
		if ((TYP(downPart) == PT_SAND || TYP(downPart) == PT_SPNG) && !TYP(pmap[y+dir3x3[up][1]][x+dir3x3[up][0]]))
		{
			if (parts[i].life > 200)
			{
				// Start growth
				sim->part_change_type(i, x, y, PT_PLNT);

				parts[i].ctype &= ~((7 << PLNT_DIR) | (3 << PLNT_PHASE)); // Clear direction and phase
				parts[i].ctype |= ((up & 7) << PLNT_DIR) | 1; // Set initial growth direction and growth bit
				parts[i].life = 15*water; // Make the first branch longer

				return 1;
			}
			else
			{
				parts[i].life++;
			}
		}
		else
			parts[i].life = 0; // Reset growth counter if we lost contact with ground
	}

	for (auto rx = -1; rx <= 1; rx++)
		for (auto ry = -1; ry <= 1; ry++)
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];

				switch (TYP(r))
				{
				case PT_WATR: // Drink water
					if (water < 31)
					{
						sim->part_kill(ID(r));
						water++;
						parts[i].life = 0;
					}
					break;
				case PT_DEUT: // Heavy water allows extra growth
					if (water < 255)
					{
						sim->part_kill(ID(r));
						water++;
						parts[i].life = 0;
					}
					break;
				case PT_CBNW:
				case PT_DSTW: // Distilled or carbonated water is allowed only in very limited amount
					if (water < 3)
					{
						sim->part_kill(ID(r));
						water++;
						parts[i].life = 0;
					}
					break;
				case PT_SLTW: // Salty water is harmful
					if (water > 0)
					{
						sim->part_kill(ID(r));
						water--;
						parts[i].life = 0;
					}
					break;
				case PT_SPNG: // Allow absorbing some water from SPNG
					if (water < 15 && parts[ID(r)].life > 0)
					{
						parts[ID(r)].life--;
						water++;
						parts[i].life = 0;
					}
					break;
				case PT_SEED: // Breed with another seed
					if (water > 0 && !(parts[i].ctype & 1) && !(parts[ID(r)].ctype & 1) && RNG::Ref().chance(1, 10))
					{
						auto ac = parts[i].ctype;
						auto bc = parts[ID(r)].ctype;

						std::array<int, 4> old_tmpsi = {{parts[i].tmp, parts[i].tmp2, parts[i].tmp3, parts[i].tmp4}};
						std::array<int, 4> old_tmpsr = {{parts[ID(r)].tmp, parts[ID(r)].tmp2, parts[ID(r)].tmp3, parts[ID(r)].tmp4}};
						std::array<int*, 4> new_tmpsi = {{&parts[i].tmp, &parts[i].tmp2, &parts[i].tmp3, &parts[i].tmp4}};
						std::array<int*, 4> new_tmpsr = {{&parts[ID(r)].tmp, &parts[ID(r)].tmp2, &parts[ID(r)].tmp3, &parts[ID(r)].tmp4}};

						// We set breeding bits, so these seeds will no longer breed with others
						parts[i].ctype = 1;
						parts[ID(r)].ctype = 1;

						// Inherit colour
						for(int ind = 0; ind < 3; ind++)
						{
							std::array<std::array<int, 2>, 2> bits;

							for(int kid = 0; kid < 2; kid++)
							{
								// Decide which allele is chosen from the first parent
								if (RNG::Ref().chance(1, 2))
									bits[kid][0] = (ac & (1 << (2*ind + PLNT_COLOUR))) != 0;
								else
									bits[kid][0] = (ac & (1 << (1 + 2*ind + PLNT_COLOUR))) != 0;

								// ... from the second parent
								if (RNG::Ref().chance(1, 2))
									bits[kid][1] = (bc & (1 << (2*ind + PLNT_COLOUR))) != 0;
								else
									bits[kid][1] = (bc & (1 << (1 + 2*ind + PLNT_COLOUR))) != 0;
							}

							// Set the corresponding genes
							parts[i].ctype |= bits[0][0] << (2*ind + PLNT_COLOUR);
							parts[i].ctype |= bits[0][1] << (1 + 2*ind + PLNT_COLOUR);

							parts[ID(r)].ctype |= bits[1][0] << (2*ind + PLNT_COLOUR);
							parts[ID(r)].ctype |= bits[1][1] << (1 + 2*ind + PLNT_COLOUR);
						}

						// Inherit the branching part of tmp - tmp4
						for(int ind = 0; ind < 5; ind++)
						{
							// Loop over tmp - tmp4
							for(int field = 0; field < 4; field++)
							{
								*(new_tmpsi[field]) |= RNG::Ref().chance(1, 2) ?
									(old_tmpsi[field] & (1 << ind)) : (old_tmpsr[field] & (1 << ind));
								*(new_tmpsr[field]) |= RNG::Ref().chance(1, 2) ?
									(old_tmpsi[field] & (1 << ind)) : (old_tmpsr[field] & (1 << ind));
							}
						}

						// Inherit the phases part of tmp - tmp4
						for(int ind = 5; ind < PLNT_TOTAL_TMP; ind += 2)
						{
							// Loop over tmp - tmp4
							for(int field = 0; field < 4; field++)
							{
								*(new_tmpsi[field]) |= RNG::Ref().chance(1, 2) ?
									(old_tmpsi[field] & (3 << ind)) : (old_tmpsr[field] & (3 << ind));
								*(new_tmpsr[field]) |= RNG::Ref().chance(1, 2) ?
									(old_tmpsi[field] & (3 << ind)) : (old_tmpsr[field] & (3 << ind));
							}
						}
					}
					break;
				default:
					continue;
				}
			}

	// Update water counter
	parts[i].ctype &= ~(0xFF << PLNT_LIFE);
	parts[i].ctype |= (water & 0xFF) << PLNT_LIFE;

	return 0;
}

void SEED_create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].ctype = 0b111011'000'00'0;

	int down = detectDown(sim, x, y); // Down gravity direction

	// Use random down direction if can't decide using gravity
	if (down < 0 || down > 7)
		down = RNG::Ref().between(0, 7);

	int up = (down+4)%8; // Up gravity direction

	sim->parts[i].ctype |= (up & 7) << PLNT_DIR;
	sim->parts[i].tmp = 0;
	sim->parts[i].tmp2 = 0;
	sim->parts[i].tmp3 = 0;
	sim->parts[i].tmp4 = 0;

	for (int ind = 0; ind < PLNT_TOTAL_TMP; ind++)
	{
		if (RNG::Ref().chance(1, 2))
			sim->parts[i].tmp |= (1<<ind);
		if (RNG::Ref().chance(1, 2))
			sim->parts[i].tmp2 |= (1<<ind);
		if (RNG::Ref().chance(1, 2))
			sim->parts[i].tmp3 |= (1<<ind);
		if (RNG::Ref().chance(1, 2))
			sim->parts[i].tmp4 |= (1<<ind);
	}
}

int SEED_graphics(GRAPHICS_FUNC_ARGS)
{
	int water = (cpart->ctype >> PLNT_LIFE) & 0xFF;

	if (water > 3) // Ready for growth
	{
		*colr -= 50;
		*colg -= 50;
		*colb -= 50;
	}
	else if (water > 0) // Had a sip of water
	{
		*colr -= 25;
		*colg -= 25;
		*colb -= 25;
	}

	return 0;
}

void SEED_init_element(ELEMENT_INIT_FUNC_ARGS)
{
	elem->Identifier = "DEFAULT_PT_SEED";
	elem->Name = "SEED";
	elem->Colour = COLPACK(0x88E788);
	elem->MenuVisible = 1;
	elem->MenuSection = SC_POWDERS;
	elem->Enabled = 1;

	elem->Advection = 0.95f;
	elem->AirDrag = 0.005f * CFDS;
	elem->AirLoss = 0.98f;
	elem->Loss = 0.80;
	elem->Collision = -0.01f;
	elem->Gravity = 0.05f;
	elem->Diffusion = 0.00f;
	elem->HotAir = 0.000f	* CFDS;
	elem->Falldown = 1;

	elem->Flammable = 0;
	elem->Explosive = 0;
	elem->Meltable = 0;
	elem->Hardness = 19;
	elem->PhotonReflectWavelengths = 0xE00;

	elem->Weight = 36;

	elem->HeatConduct = 32;
	elem->Latent = 0;
	elem->Description = "Seeds. Put on sand and add water to grow a tree.";

	elem->Properties = TYPE_PART | PROP_NEUTPASS;

	elem->LowPressureTransitionThreshold = IPL;
	elem->LowPressureTransitionElement = NT;
	elem->HighPressureTransitionThreshold = IPH;
	elem->HighPressureTransitionElement = NT;
	elem->LowTemperatureTransitionThreshold = ITL;
	elem->LowTemperatureTransitionElement = NT;
	elem->HighTemperatureTransitionThreshold = 673.15f;
	elem->HighTemperatureTransitionElement = PT_FIRE;

	elem->Update = &SEED_update;
	elem->Graphics = &SEED_graphics;
	elem->Func_Create = &SEED_create;
	elem->Init = &SEED_init_element;
}
