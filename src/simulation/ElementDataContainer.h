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

#ifndef ElementDataContainer_h
#define ElementDataContainer_h

class Simulation;
class ElementDataContainer
{
public:
	ElementDataContainer() {}
	virtual ElementDataContainer * Clone() { return new ElementDataContainer(*this); }
	virtual ~ElementDataContainer() {}
	virtual void Simulation_Cleared(Simulation *sim) {}
	virtual void Simulation_BeforeUpdate(Simulation *sim) {}
	virtual void Simulation_AfterUpdate(Simulation *sim) {}
};

#endif
