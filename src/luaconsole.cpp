/**
 * Powder Toy - Lua console
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

#ifdef LUACONSOLE
#include <math.h>

#if defined(LIN) || defined(MACOSX)
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef WIN
#include <direct.h>
#endif
extern "C"
{
#include "socket/luasocket.h"
#include "socket/socket.lua.h"
}

#include "defines.h"
#include "powder.h"
#include "graphics.h"
#include "gravity.h"
#include "http.h"
#include "console.h"
#include "interface.h"
#include "luaconsole.h"
#include "luascriptinterface.h"
#include "save.h"

#include "game/Brush.h"
#include "game/Menus.h"
#include "simulation/Simulation.h"
#include "simulation/Tool.h"
#include "simulation/WallNumbers.h"

int *lua_el_func, *lua_el_mode, *lua_gr_func;
char* log_history[20];
int log_history_times[20];
int getPartIndex_curIdx;
lua_State *l;
int tptProperties; //Table for some TPT properties
int tptPropertiesVersion;
int tptElements; //Table for TPT element names
int tptParts, tptPartsMeta, tptElementTransitions, tptPartsCData, tptPartMeta, tptPart, cIndex;

void luacon_open()
{
	int i = 0;
	char tmpname[12];
	int currentElementMeta, currentElement;
	const static struct luaL_Reg tptluaapi [] = {
		{"test", &luatpt_test},
		{"drawtext", &luatpt_drawtext},
		{"create", &luatpt_create},
		{"set_pause", &luatpt_setpause},
		{"toggle_pause", &luatpt_togglepause},
		{"set_console", &luatpt_setconsole},
		{"log", &luatpt_log},
		{"set_pressure", &luatpt_set_pressure},
		{"set_aheat", &luatpt_set_aheat},
		{"set_velocity", &luatpt_set_velocity},
		{"set_gravity", &luatpt_set_gravity},
		{"reset_gravity_field", &luatpt_reset_gravity_field},
		{"reset_velocity", &luatpt_reset_velocity},
		{"reset_spark", &luatpt_reset_spark},
		{"set_property", &luatpt_set_property},
		{"get_property", &luatpt_get_property},
		{"set_wallmap",&luatpt_createwall},
		{"get_wallmap",&luatpt_getwall},
		{"set_elecmap",&luatpt_set_elecmap},
		{"get_elecmap",&luatpt_get_elecmap},
		{"drawpixel", &luatpt_drawpixel},
		{"drawrect", &luatpt_drawrect},
		{"fillrect", &luatpt_fillrect},
		{"drawcircle", &luatpt_drawcircle},
		{"fillcircle", &luatpt_fillcircle},
		{"drawline", &luatpt_drawline},
		{"textwidth", &luatpt_textwidth},
		{"get_name", &luatpt_get_name},
		{"set_shortcuts", &luatpt_set_shortcuts},
		{"delete", &luatpt_delete},
		{"register_step", &luatpt_register_step},
		{"unregister_step", &luatpt_unregister_step},
		{"register_mouseclick", &luatpt_register_mouseclick},
		{"unregister_mouseclick", &luatpt_unregister_mouseclick},
		{"register_keypress", &luatpt_register_keypress},
		{"unregister_keypress", &luatpt_unregister_keypress},
		{"register_mouseevent", &luatpt_register_mouseclick},
		{"unregister_mouseevent", &luatpt_unregister_mouseclick},
		{"register_keyevent", &luatpt_register_keypress},
		{"unregister_keyevent", &luatpt_unregister_keypress},
		{"input", &luatpt_input},
		{"message_box", &luatpt_message_box},
		{"get_numOfParts", &luatpt_get_numOfParts},
		{"start_getPartIndex", &luatpt_start_getPartIndex},
		{"next_getPartIndex", &luatpt_next_getPartIndex},
		{"getPartIndex", &luatpt_getPartIndex},
		{"hud", &luatpt_hud},
		{"newtonian_gravity", &luatpt_gravity},
		{"ambient_heat", &luatpt_airheat},
		{"active_menu", &luatpt_active_menu},
		{"decorations_enable", &luatpt_decorations_enable},
		{"display_mode", &luatpt_cmode_set},
		{"throw_error", &luatpt_error},
		{"heat", &luatpt_heat},
		{"setfire", &luatpt_setfire},
		{"setdebug", &luatpt_setdebug},
		{"setfpscap",&luatpt_setfpscap},
		{"getscript",&luatpt_getscript},
		{"setwindowsize",&luatpt_setwindowsize},
		{"watertest",&luatpt_togglewater},
		{"screenshot",&luatpt_screenshot},
		{"get_clipboard",&luatpt_getclip},
		{"set_clipboard",&luatpt_setclip},
		{"element",&luatpt_getelement},
		{"element_func",&luatpt_element_func},
		{"graphics_func",&luatpt_graphics_func},
		{"load",&simulation_loadSave},
		{"bubble",&luatpt_bubble},
		{"maxframes",&luatpt_maxframes},
		{"indestructible",&luatpt_indestructible},
		{"oldmenu", &luatpt_oldmenu},
		{NULL,NULL}
	};

	l = luaL_newstate();
	luaL_openlibs(l);
	luaopen_bit(l);
	luaopen_socket_core(l);
	lua_getglobal(l, "package");
	lua_pushstring(l, "loaded");
	lua_rawget(l, -2);
	lua_pushstring(l, "socket");
	lua_rawget(l, -2);
	lua_pushstring(l, "socket.core");
	lua_pushvalue(l, -2);
	lua_rawset(l, -4);
	lua_pop(l, 3);
	luaopen_socket(l);
	luaL_register(l, "tpt", tptluaapi);
	
	initSimulationAPI(l);
	initRendererAPI(l);
	initFileSystemAPI(l);
	initGraphicsAPI(l);
	initElementsAPI(l);
	lua_getglobal(l, "tpt");

	tptProperties = lua_gettop(l);
	
	//Replace print function with our screen/console logging function
	lua_pushcfunction(l, luatpt_log);
	lua_setglobal(l, "print");

	lua_pushinteger(l, 0);
	lua_setfield(l, tptProperties, "mousex");
	lua_pushinteger(l, 0);
	lua_setfield(l, tptProperties, "mousey");
	
	lua_newtable(l);
	tptPropertiesVersion = lua_gettop(l);
	lua_pushinteger(l, SAVE_VERSION);
	lua_setfield(l, tptPropertiesVersion, "major");
	lua_pushinteger(l, MINOR_VERSION);
	lua_setfield(l, tptPropertiesVersion, "minor");
	lua_pushinteger(l, BUILD_NUM);
	lua_setfield(l, tptPropertiesVersion, "build");
	lua_pushinteger(l, MOD_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod");
	lua_pushinteger(l, MOD_MINOR_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_minor");
	lua_pushinteger(l, MOD_SAVE_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_save");
	lua_pushinteger(l, MOD_BUILD_VERSION);
	lua_setfield(l, tptPropertiesVersion, "jacob1s_mod_build");
	lua_setfield(l, tptProperties, "version");
	
#ifdef FFI
	//LuaJIT's ffi gives us direct access to parts data, no need for nested metatables. HOWEVER, this is in no way safe, it's entirely possible for someone to try to read parts[-10]
	lua_pushlightuserdata(l, parts);
	lua_setfield(l, tptProperties, "partsdata");
	
	luaL_dostring (l, "ffi = require(\"ffi\")\n\
ffi.cdef[[\n\
typedef struct { int type; int life, ctype; float x, y, vx, vy; float temp; float pavg[2]; int flags; int tmp; int tmp2; unsigned int dcolour; } particle;\n\
]]\n\
tpt.parts = ffi.cast(\"particle *\", tpt.partsdata)\n\
ffi = nil\n\
tpt.partsdata = nil");
	//Since ffi is REALLY REALLY dangrous, we'll remove it from the environment completely (TODO)
	//lua_pushstring(l, "parts");
	//tptPartsCData = lua_gettable(l, tptProperties);
#else
	lua_newtable(l);
	tptParts = lua_gettop(l);
	lua_newtable(l);
	tptPartsMeta = lua_gettop(l);
	lua_pushcfunction(l, luacon_partswrite);
	lua_setfield(l, tptPartsMeta, "__newindex");
	lua_pushcfunction(l, luacon_partsread);
	lua_setfield(l, tptPartsMeta, "__index");
	lua_setmetatable(l, tptParts);
	lua_setfield(l, tptProperties, "parts");
	
	lua_newtable(l);
	tptPart = lua_gettop(l);
	lua_newtable(l);
	tptPartMeta = lua_gettop(l);
	lua_pushcfunction(l, luacon_partwrite);
	lua_setfield(l, tptPartMeta, "__newindex");
	lua_pushcfunction(l, luacon_partread);
	lua_setfield(l, tptPartMeta, "__index");
	lua_setmetatable(l, tptPart);
	
	tptPart = luaL_ref(l, LUA_REGISTRYINDEX);
#endif
	
	lua_newtable(l);
	tptElements = lua_gettop(l);
	for(i = 1; i < PT_NUM; i++)
	{
		if (i == PT_EXPL)
			continue;
		for(unsigned int j = 0; j < strlen(ptypes[i].name); j++)
			tmpname[j] = tolower(ptypes[i].name[j]);
		tmpname[strlen(ptypes[i].name)] = 0;
		
		lua_newtable(l);
		currentElement = lua_gettop(l);
		lua_pushinteger(l, i);
		lua_setfield(l, currentElement, "id");
		
		lua_newtable(l);
		currentElementMeta = lua_gettop(l);
		lua_pushcfunction(l, luacon_elementwrite);
		lua_setfield(l, currentElementMeta, "__newindex");
		lua_pushcfunction(l, luacon_elementread);
		lua_setfield(l, currentElementMeta, "__index");
		lua_setmetatable(l, currentElement);
		
		lua_setfield(l, tptElements, tmpname);
	}
	lua_setfield(l, tptProperties, "el");
	
	lua_newtable(l);
	tptElementTransitions = lua_gettop(l);
	for(i = 1; i < PT_NUM; i++)
	{
		if (i == PT_EXPL)
			continue;
		for(unsigned int j = 0; j < strlen(ptypes[i].name); j++)
			tmpname[j] = tolower(ptypes[i].name[j]);
		tmpname[strlen(ptypes[i].name)] = 0;
		
		lua_newtable(l);
		currentElement = lua_gettop(l);		
		lua_newtable(l);
		currentElementMeta = lua_gettop(l);
		lua_pushinteger(l, i);
		lua_setfield(l, currentElement, "value");
		lua_pushcfunction(l, luacon_transitionwrite);
		lua_setfield(l, currentElementMeta, "__newindex");
		lua_pushcfunction(l, luacon_transitionread);
		lua_setfield(l, currentElementMeta, "__index");
		lua_setmetatable(l, currentElement);
		
		lua_setfield(l, tptElementTransitions, tmpname);
	}
	lua_setfield(l, tptProperties, "eltransition");
	
	lua_el_func = (int*)calloc(PT_NUM, sizeof(int));
	lua_el_mode = (int*)calloc(PT_NUM, sizeof(int));
	lua_gr_func = (int*)calloc(PT_NUM, sizeof(int));
	for(i = 0; i < PT_NUM; i++)
	{
		lua_el_mode[i] = 0;
		lua_gr_func[i] = 0;
	}
	for (i = 0; i < 20; i++)
	{
		log_history[i] = NULL;
		log_history_times[i] = 0;
	}
	lua_sethook(l, &lua_hook, LUA_MASKCOUNT, 4000000);

	//make tpt.* a metatable
	lua_newtable(l);
	lua_pushcfunction(l, luacon_tptIndex);
	lua_setfield(l, -2, "__index");
	lua_pushcfunction(l, luacon_tptNewIndex);
	lua_setfield(l, -2, "__newindex");
	lua_setmetatable(l, -2);
}

void luacon_openmultiplayer()
{
	luaopen_multiplayer(l);
}

void luacon_openscriptmanager()
{
	luaopen_scriptmanager(l);
}

#ifndef FFI
int luacon_partread(lua_State* l)
{
	int format, offset, tempinteger;
	float tempfloat;
	int i;
	const char * key = luaL_optstring(l, 2, "");
	offset = Particle_GetOffset(key, &format);

	i = cIndex;
	
	if (i < 0 || i >= NPART || offset==-1)
	{
		if (i < 0 || i >= NPART)
			return luaL_error(l, "Out of range");
		else if (!strcmp(key, "id"))
		{
			lua_pushnumber(l, i);
			return 1;
		}
		else
			return luaL_error(l, "Invalid property");
	}

	switch(format)
	{
	case 0:
	case 2:
	case 3:
		tempinteger = *((int*)(((char*)&parts[i])+offset));
		lua_pushnumber(l, tempinteger);
		break;
	case 1:
		tempfloat = *((float*)(((char*)&parts[i])+offset));
		lua_pushnumber(l, tempfloat);
		break;
	}
	return 1;
}

int luacon_partwrite(lua_State* l)
{
	int format, offset;
	int i;
	const char * key = luaL_optstring(l, 2, "");
	offset = Particle_GetOffset(key, &format);
	
	i = cIndex;
	
	if (i < 0 || i >= NPART || offset==-1)
	{
		if (i < 0 || i >= NPART)
			return luaL_error(l, "array index out of bounds");
		else
			return luaL_error(l, "Invalid property");
	}

	switch(format)
	{
	case 0:
	case 3:
		*((int*)(((char*)&parts[i])+offset)) = luaL_optinteger(l, 3, 0);
		break;
	case 1:
		*((float*)(((char*)&parts[i])+offset)) = (float)luaL_optnumber(l, 3, 0);
		break;
	case 2:
		globalSim->part_change_type_force(i, luaL_optinteger(l, 3, 0));
	}
	return 1;
}

int luacon_partsread(lua_State* l)
{
	int i = luaL_optinteger(l, 2, 0);
	
	if (i<0 || i>=NPART)
	{
		return luaL_error(l, "array index out of bounds");
	}
	
	lua_rawgeti(l, LUA_REGISTRYINDEX, tptPart);
	cIndex = i;
	return 1;
}

int luacon_partswrite(lua_State* l)
{
	return luaL_error(l, "table readonly");
}
#endif

int luacon_transition_getproperty(const char * key, int * format)
{
	int offset;
	if (!strcmp(key, "presHighValue")) {
		offset = offsetof(Element, HighPressureTransitionThreshold);
		*format = 1;
	} else if (!strcmp(key, "presHighType")) {
		offset = offsetof(Element, HighPressureTransitionElement);
		*format = 0;
	} else if (!strcmp(key, "presLowValue")) {
		offset = offsetof(Element, LowPressureTransitionThreshold);
		*format = 1;
	} else if (!strcmp(key, "presLowType")) {
		offset = offsetof(Element, LowPressureTransitionElement);
		*format = 0;
	} else if (!strcmp(key, "tempHighValue")) {
		offset = offsetof(Element, HighTemperatureTransitionThreshold);
		*format = 1;
	} else if (!strcmp(key, "tempHighType")) {
		offset = offsetof(Element, HighTemperatureTransitionElement);
		*format = 0;
	} else if (!strcmp(key, "tempLowValue")) {
		offset = offsetof(Element, LowTemperatureTransitionThreshold);
		*format = 1;
	} else if (!strcmp(key, "tempLowType")) {
		offset = offsetof(Element, LowTemperatureTransitionElement);
		*format = 0;
	} else {
		offset = -1;
	}
	return offset;
}

int luacon_transitionread(lua_State* l)
{
	int format, offset;
	const char * key = luaL_optstring(l, 2, "");
	offset = luacon_transition_getproperty(key, &format);
	
	//Get Raw Index value for element
	lua_pushstring(l, "value");
	lua_rawget(l, 1);
	
	int i = lua_tointeger(l, lua_gettop(l));
	
	lua_pop(l, 1);
	
	if (i < 0 || i >= PT_NUM || offset==-1)
	{
		return luaL_error(l, "Invalid property");
	}
	elements_writeProperty(l, i, format, offset);
	return 1;
}

int luacon_transitionwrite(lua_State* l)
{
	int format, offset;
	const char * key = luaL_optstring(l, 2, "");
	offset = luacon_transition_getproperty(key, &format);
	
	//Get Raw Index value for element
	lua_pushstring(l, "value");
	lua_rawget(l, 1);
	
	int i = lua_tointeger(l, lua_gettop(l));
	
	lua_pop(l, 1);
	
	if (i < 0 || i >= PT_NUM || offset==-1)
	{
		return luaL_error(l, "Invalid property");
	}
	elements_setProperty(l, i, format, offset);
	Simulation_Compat_CopyData(globalSim);
	return 0;
}

int luacon_element_getproperty(const char * key, int * format, unsigned int * modified_stuff)
{
	int offset;
	if (!strcmp(key, "name"))
	{
		offset = offsetof(Element, Name);
		*format = 2;
	}
	else if (!strcmp(key, "colour") || !strcmp(key, "colour"))
	{
		offset = offsetof(Element, Colour);
		*format = 4;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_GRAPHICS;
	}
	else if (!strcmp(key, "advection"))
	{
		offset = offsetof(Element, Advection);
		*format = 1;
	}
	else if (!strcmp(key, "airdrag"))
	{
		offset = offsetof(Element, AirDrag);
		*format = 1;
	}
	else if (!strcmp(key, "airloss"))
	{
		offset = offsetof(Element, AirLoss);
		*format = 1;
	}
	else if (!strcmp(key, "loss"))
	{
		offset = offsetof(Element, Loss);
		*format = 1;
	}
	else if (!strcmp(key, "collision"))
	{
		offset = offsetof(Element, Collision);
		*format = 1;
	}
	else if (!strcmp(key, "gravity"))
	{
		offset = offsetof(Element, Gravity);
		*format = 1;
	}
	else if (!strcmp(key, "diffusion"))
	{
		offset = offsetof(Element, Diffusion);
		*format = 1;
	}
	else if (!strcmp(key, "hotair"))
	{
		offset = offsetof(Element, HotAir);
		*format = 1;
	}
	else if (!strcmp(key, "falldown"))
	{
		offset = offsetof(Element, Falldown);
		*format = 0;
	}
	else if (!strcmp(key, "flammable"))
	{
		offset = offsetof(Element, Flammable);
		*format = 0;
	}
	else if (!strcmp(key, "explosive"))
	{
		offset = offsetof(Element, Explosive);
		*format = 0;
	}
	else if (!strcmp(key, "meltable"))
	{
		offset = offsetof(Element, Meltable);
		*format = 0;
	}
	else if (!strcmp(key, "hardness"))
	{
		offset = offsetof(Element, Hardness);
		*format = 0;
	}
	else if (!strcmp(key, "photonwavelength"))
	{
		offset = offsetof(Element, PhotonReflectWavelengths);
		*format = 5;
	}
	else if (!strcmp(key, "menu"))
	{
		offset = offsetof(Element, MenuVisible);
		*format = 0;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_MENUS;
	}
	else if (!strcmp(key, "menusection"))
	{
		offset = offsetof(Element, MenuSection);
		*format = 0;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_MENUS;
	}
	else if (!strcmp(key, "enabled"))
	{
		offset = offsetof(Element, Enabled);
		*format = 0;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_MENUS;
	}
	else if (!strcmp(key, "weight"))
	{
		offset = offsetof(Element, Weight);
		*format = 0;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_CANMOVE;
	}
	else if (!strcmp(key, "heat"))
	{
		offset = offsetof(Element, DefaultProperties.temp);
		*format = 1;
	}
	else if (!strcmp(key, "hconduct"))
	{
		offset = offsetof(Element, HeatConduct);
		*format = 3;
	}
	else if (!strcmp(key, "latent"))
	{
		offset = offsetof(Element, Latent);
		*format = 5;
	}
	else if (!strcmp(key, "state"))
	{
		offset = offsetof(Element, State);
		*format = 6;
	}
	else if (!strcmp(key, "properties"))
	{
		offset = offsetof(Element, Properties);
		*format = 5;
		if (modified_stuff)
			*modified_stuff |= LUACON_EL_MODIFIED_GRAPHICS | LUACON_EL_MODIFIED_CANMOVE;
	}
	else if (!strcmp(key, "description"))
	{
		offset = offsetof(Element, Description);
		*format = 2;
	}
	else
	{
		return -1;
	}
	return offset;
}

int luacon_elementread(lua_State* l)
{
	int format, offset;
	int i;
	const char * key = luaL_optstring(l, 2, "");
	offset = luacon_element_getproperty(key, &format, NULL);
	
	//Get Raw Index value for element
	lua_pushstring(l, "id");
	lua_rawget(l, 1);
	
	i = lua_tointeger (l, lua_gettop(l));
	
	lua_pop(l, 1);
	
	if (i < 0 || i >= PT_NUM || offset==-1)
	{
		return luaL_error(l, "Invalid property");
	}
	elements_writeProperty(l, i, format, offset);
	return 1;
}

int luacon_elementwrite(lua_State* l)
{
	int format, offset;
	int i;
	unsigned int modified_stuff = 0;
	const char * key = luaL_optstring(l, 2, "");
	offset = luacon_element_getproperty(key, &format, &modified_stuff);
	
	//Get Raw Index value for element
	lua_pushstring(l, "id");
	lua_rawget(l, 1);
	
	i = lua_tointeger (l, lua_gettop(l));
	
	lua_pop(l, 1);
	
	if (i < 0 || i >= PT_NUM || offset==-1)
	{
		return luaL_error(l, "Invalid property");
	}

	char * tempstring = mystrdup(luaL_optstring(l, 3, ""));
	if (!strcmp(key, "name"))
	{
		//Convert to upper case
		for (unsigned int j = 0; j < strlen(tempstring); j++)
			tempstring[j] = toupper(tempstring[j]);
		if (console_parse_type(tempstring, NULL, NULL))
		{
			free(tempstring);
			return luaL_error(l, "Name in use");
		}
	}
	else
		free(tempstring);
	elements_setProperty(l, i, format, offset);
	Simulation_Compat_CopyData(globalSim);
	if (modified_stuff)
	{
		if (modified_stuff & LUACON_EL_MODIFIED_MENUS)
			FillMenus();
		if (modified_stuff & LUACON_EL_MODIFIED_CANMOVE)
			globalSim->InitCanMove();
		if (modified_stuff & LUACON_EL_MODIFIED_GRAPHICS)
			memset(graphicscache, 0, sizeof(gcache_item)*PT_NUM);
	}
	return 0;
}

int luacon_tptIndex(lua_State *l)
{
	std::string key = luaL_checkstring(l, 2);
	if (!key.compare("selectedl"))
		return lua_pushstring(l, activeTools[0]->GetIdentifier().c_str()), 1;
	if (!key.compare("selectedr"))
		return lua_pushstring(l, activeTools[1]->GetIdentifier().c_str()), 1;
	if (!key.compare("selecteda"))
		return lua_pushstring(l, activeTools[2]->GetIdentifier().c_str()), 1;
	if (!key.compare("selectedreplace"))
		return lua_pushstring(l, activeTools[2]->GetIdentifier().c_str()), 1;
	if (!key.compare("brushx"))
		return lua_pushnumber(l, currentBrush->GetRadius().X), 1;
	if (!key.compare("brushy"))
		return lua_pushnumber(l, currentBrush->GetRadius().Y), 1;
	if (!key.compare("brushID"))
		return lua_pushnumber(l, currentBrush->GetShape()), 1;

	//if not a special key, return the value in the table
	return lua_rawget(l, 1), 1;
}

int luacon_tptNewIndex(lua_State *l)
{
	std::string key = luaL_checkstring(l, 2);
	if (!key.compare("selectedl"))
	{
		Tool* t = GetToolFromIdentifier(luaL_checkstring(l, 3));
		if (t)
			activeTools[0] = t;
		else
			luaL_error(l, "Invalid tool identifier: %s", lua_tostring(l, 3));
	}
	else if (!key.compare("selectedr"))
	{
		Tool* t = GetToolFromIdentifier(luaL_checkstring(l, 3));
		if (t)
			activeTools[1] = t;
		else
			luaL_error(l, "Invalid tool identifier: %s", lua_tostring(l, 3));
	}
	else if (!key.compare("selecteda"))
	{
		Tool* t = GetToolFromIdentifier(luaL_checkstring(l, 3));
		if (t)
			activeTools[2] = t;
		else
			luaL_error(l, "Invalid tool identifier: %s", lua_tostring(l, 3));
	}
	else if (!key.compare("selectedreplace"))
	{
		Tool* t = GetToolFromIdentifier(luaL_checkstring(l, 3));
		if (t)
			activeTools[2] = t;
		else
			luaL_error(l, "Invalid tool identifier: %s", lua_tostring(l, 3));
	}
	else if (!key.compare("brushx"))
		currentBrush->SetRadius(Point(luaL_checkinteger(l, 3), currentBrush->GetRadius().Y));
	else if (!key.compare("brushy"))
		currentBrush->SetRadius(Point(currentBrush->GetRadius().X, luaL_checkinteger(l, 3)));
	else if (!key.compare("brushID"))
		currentBrush->SetShape(luaL_checkinteger(l, 3)%BRUSH_NUM);
	else
	{
		//if not a special key, set a value in the table
		return lua_rawset(l, 1), 1;
	}
	return 0;
}

int luacon_keyevent(int key, int modifier, int event)
{
	int keycontinue = 1;
	lua_pushstring(l, "keyfunctions");
	lua_rawget(l, LUA_REGISTRYINDEX);
	if (!lua_istable(l, -1))
	{
		lua_pop(l, 1);
		lua_newtable(l);
		lua_pushstring(l, "keyfunctions");
		lua_pushvalue(l, -2);
		lua_rawset(l, LUA_REGISTRYINDEX);
	}
	int len = lua_objlen(l, -1);
	for (int i = 1; i <= len && keycontinue; i++)
	{
		loop_time = SDL_GetTicks();
		lua_rawgeti(l, -1, i);
		lua_pushlstring(l, (const char*)&key, 1);
		lua_pushinteger(l, key);
		lua_pushinteger(l, modifier);
		lua_pushinteger(l, event);
		int callret = lua_pcall(l, 4, 1, 0);
		if (callret)
		{
			char *error, *tolog;
			if (!strcmp(luacon_geterror(), "Error: Script not responding"))
			{
				for (int j = i;j <= len-1; j++)
				{
					lua_rawgeti(l, -2, j+1);
					lua_rawseti(l, -3, j);
				}
				lua_pushnil(l);
				lua_rawseti(l, -3, len);
				i--;
			}
			error = (char*)luacon_geterror();
			tolog = (char*)malloc(strlen(error) + 15);
			sprintf(tolog, "In key event: %s", error);
			luacon_log(tolog);
			lua_pop(l, 1);
		}
		else
		{
			if (!lua_isnoneornil(l, -1))
				keycontinue = lua_toboolean(l, -1);
			lua_pop(l, 1);
		}
		len = lua_objlen(l, -1);
	}
	lua_pop(l, 1);
	return keycontinue;
}

int luacon_mouseevent(int mx, int my, int mb, int event, int mouse_wheel)
{
	int mpcontinue = 1;
	lua_pushstring(l, "mousefunctions");
	lua_rawget(l, LUA_REGISTRYINDEX);
	if (!lua_istable(l, -1))
	{
		lua_pop(l, 1);
		lua_newtable(l);
		lua_pushstring(l, "mousefunctions");
		lua_pushvalue(l, -2);
		lua_rawset(l, LUA_REGISTRYINDEX);
	}
	int len = lua_objlen(l, -1);
	for (int i = 1; i <= len && mpcontinue; i++)
	{
		loop_time = SDL_GetTicks();
		lua_rawgeti(l, -1, i);
		lua_pushinteger(l, mx);
		lua_pushinteger(l, my);
		lua_pushinteger(l, mb);
		lua_pushinteger(l, event);
		lua_pushinteger(l, mouse_wheel);
		int callret = lua_pcall(l, 5, 1, 0);
		if (callret)
		{
			char *error, *tolog;
			if (!strcmp(luacon_geterror(), "Error: Script not responding"))
			{
				for (int j = i; j <= len-1; j++)
				{
					lua_rawgeti(l, -2, j+1);
					lua_rawseti(l, -3, j);
				}
				lua_pushnil(l);
				lua_rawseti(l, -3, len);
				i--;
			}
			error = (char*)luacon_geterror();
			tolog = (char*)malloc(strlen(error) + 17);
			sprintf(tolog, "In mouse event: %s", error);
			luacon_log(tolog);
			lua_pop(l, 1);
		}
		else
		{
			if (!lua_isnoneornil(l, -1))
				mpcontinue = lua_toboolean(l, -1);
			lua_pop(l, 1);
		}
		len = lua_objlen(l, -1);
	}
	lua_pop(l, 1);
	return mpcontinue;
}

int luacon_step(int mx, int my)
{
	lua_pushinteger(l, my);
	lua_pushinteger(l, mx);
	lua_setfield(l, tptProperties, "mousex");
	lua_setfield(l, tptProperties, "mousey");
	lua_getglobal(l, "simulation");
	lua_pushinteger(l, NUM_PARTS); lua_setfield(l, -2, "NUM_PARTS");
	lua_pop(l, 1);

	lua_pushstring(l, "stepfunctions");
	lua_rawget(l, LUA_REGISTRYINDEX);
	if (!lua_istable(l, -1))
	{
		lua_pop(l, 1);
		lua_newtable(l);
		lua_pushstring(l, "stepfunctions");
		lua_pushvalue(l, -2);
		lua_rawset(l, LUA_REGISTRYINDEX);
	}
	int len = lua_objlen(l, -1);
	for (int i = 1; i <= len; i++)
	{
		loop_time = SDL_GetTicks();
		lua_rawgeti(l, -1, i);
		int callret = lua_pcall(l, 0, 0, 0);
		if (callret)
		{
			if (!strcmp(luacon_geterror(), "Error: Script not responding"))
			{
				for (int j = i; j <= len-1; j++)
				{
					lua_rawgeti(l, -2, j+1);
					lua_rawseti(l, -3, j);
				}
				lua_pushnil(l);
				lua_rawseti(l, -3, len);
				i--;
			}
			luacon_log(mystrdup(luacon_geterror()));
			lua_pop(l, 1);
		}
		len = lua_objlen(l, -1);
	}
	lua_pop(l, 1);
	return 0;
}

int luaL_tostring(lua_State *L, int n)
{
	luaL_checkany(L, n);
	switch (lua_type(L, n))
	{
		case LUA_TNUMBER:
			lua_pushstring(L, lua_tostring(L, n));
			break;
		case LUA_TSTRING:
			lua_pushvalue(L, n);
			break;
		case LUA_TBOOLEAN:
			lua_pushstring(L, (lua_toboolean(L, n) ? "true" : "false"));
			break;
		case LUA_TNIL:
			lua_pushliteral(L, "nil");
			break;
		default:
			lua_pushfstring(L, "%s: %p", luaL_typename(L, n), lua_topointer(L, n));
			break;
	}
	return 1;
}

void luacon_log(char *log)
{
	int i;
	if (log_history[19])
		free(log_history[19]);
	for (i = 19; i > 0; i--)
	{
		log_history[i] = log_history[i-1];
		log_history_times[i] = log_history_times[i-1];
	}
	log_history[0] = log;
	log_history_times[0] = 150;
	printf("%s\n",log);
}

char *lastCode = NULL;
char *logs = NULL; //logs from tpt.log are temporarily stored here
int luacon_eval(char *command, char **result)
{
	int level = lua_gettop(l), ret = -1;
	char *text = NULL, *tmp;
	if (logs)
	{
		free(logs);
		logs = NULL;
	}
	if (lastCode)
	{
		char *tmplastCode = (char*)malloc(strlen(lastCode)+strlen(command)+3);
		sprintf(tmplastCode, "%s\n%s", lastCode, command);
		free(lastCode);
		lastCode = tmplastCode;
	}
	else
	{
		lastCode = (char*)malloc(strlen(command)+1);
		sprintf(lastCode, "%s", command);
	}
	tmp = (char*)malloc(strlen(lastCode) + 8);
	sprintf(tmp, "return %s", lastCode);
	loop_time = SDL_GetTicks();
	luaL_loadbuffer(l, tmp, strlen(tmp), "@console");
	if(lua_type(l, -1) != LUA_TFUNCTION)
	{
		lua_pop(l, 1);
		luaL_loadbuffer(l, lastCode, strlen(lastCode), "@console");
	}
	if(lua_type(l, -1) != LUA_TFUNCTION)
	{
		*result = mystrdup(luacon_geterror());
		if (strstr(*result, "near '<eof>'"))
		{
			free(*result);
			*result = mystrdup("...");
		}
		else
		{
			free(lastCode);
			lastCode = NULL;
		}
		return 0;
	}
	else
	{
		free(lastCode);
		lastCode = NULL;
		ret = lua_pcall(l, 0, LUA_MULTRET, 0);
		if(ret)
			return ret;
		else
		{
			for(level++;level<=lua_gettop(l);level++)
			{
				luaL_tostring(l, level);
				if(text)
				{
					char *text2 = (char*)malloc(strlen(luaL_optstring(l, -1, "")) + strlen(text) + 3);
					sprintf(text2, "%s, %s", text, luaL_optstring(l, -1, ""));
					free(text);
					text = mystrdup(text2);
					free(text2);
				}
				else
				{
					text = mystrdup(luaL_optstring(l, -1, ""));
				}
				lua_pop(l, 1);
			}
			if (logs)
			{
				if (!text)
					text = mystrdup(logs);
				else
				{
					char *tmp2 = (char*)malloc(strlen(logs)+strlen(text)+3);
					sprintf(tmp2, "%s; %s", logs, text);
					free(text);
					text = tmp2;
				}
				free(logs);
				logs = NULL;
			}
			if(text)
				if(*result)
				{
					char *tmp2 = (char*)malloc(strlen(*result)+strlen(text)+3);
					sprintf(tmp2, "%s; %s", result, text);
					*result = tmp2;
				}
				else
					*result = mystrdup(text);

		}
	}
	return ret;
}

void lua_hook(lua_State *L, lua_Debug *ar)
{
	if(ar->event == LUA_HOOKCOUNT && SDL_GetTicks()-loop_time > 3000)
	{
		if (confirm_ui(vid_buf,"Infinite Loop","The Lua code might have an infinite loop. Press OK to stop it","OK"))
			luaL_error(l,"Error: Infinite loop");
		loop_time = SDL_GetTicks();
	}
}

int luacon_part_update(int t, int i, int x, int y, int surround_space, int nt)
{
	int retval = 0, callret;
	if(lua_el_func[t]){
		lua_rawgeti(l, LUA_REGISTRYINDEX, lua_el_func[t]);
		lua_pushinteger(l, i);
		lua_pushinteger(l, x);
		lua_pushinteger(l, y);
		lua_pushinteger(l, surround_space);
		lua_pushinteger(l, nt);
		callret = lua_pcall(l, 5, 1, 0);
		if (callret)
		{
			char *error = (char*)luacon_geterror();
			char *tolog = (char*)malloc(strlen(error) + 21);
			sprintf(tolog, "In particle update: %s", error);
			luacon_log(tolog);
		}
		if(lua_isboolean(l, -1)){
			retval = lua_toboolean(l, -1);
		}
		lua_pop(l, 1);
	}
	return retval;
}

int luacon_graphics_update(int t, int i, int *pixel_mode, int *cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb)
{
	int cache = 0, callret;
	lua_rawgeti(l, LUA_REGISTRYINDEX, lua_gr_func[t]);
	lua_pushinteger(l, i);
	lua_pushinteger(l, *colr);
	lua_pushinteger(l, *colg);
	lua_pushinteger(l, *colb);
	callret = lua_pcall(l, 4, 10, 0);
	if (callret)
	{
		char *error = (char*)luacon_geterror();
		char *tolog = (char*)malloc(strlen(error) + 23);
		sprintf(tolog, "In graphics function: %s", error);
		luacon_log(tolog);
		lua_pop(l, 1);
	}
	else
	{
		cache = luaL_optint(l, -10, 0);
		*pixel_mode = luaL_optint(l, -9, *pixel_mode);
		*cola = luaL_optint(l, -8, *cola);
		*colr = luaL_optint(l, -7, *colr);
		*colg = luaL_optint(l, -6, *colg);
		*colb = luaL_optint(l, -5, *colb);
		*firea = luaL_optint(l, -4, *firea);
		*firer = luaL_optint(l, -3, *firer);
		*fireg = luaL_optint(l, -2, *fireg);
		*fireb = luaL_optint(l, -1, *fireb);
		lua_pop(l, 10);
	}
	return cache;
}

const char *luacon_geterror()
{
	luaL_tostring(l, -1);
	const char* err = luaL_optstring(l, -1, "failed to execute");
	lua_pop(l, 1);
	return err;
}

void luacon_close()
{
	int i;
	lua_close(l);
	if (lastCode)
		free(lastCode);
	if (logs)
		free(logs);
	if (LuaCode)
		free(LuaCode);
	for (i = 0; i < 20; i++)
	{
		if (log_history[i])
			free(log_history[i]);
	}
	console_limit_history(0, last_command);
	console_limit_history(0, last_command_result);
}

int process_command_lua(pixel *vid_buf, char *command, char **result)
{
	if (command && strlen(command))
	{
		if(strncmp(command, "!", 1)==0)
		{
			return process_command_old(vid_buf, command+1, result);
		}
		else
		{
			int commandret = luacon_eval(command, result);
			if (commandret)
			{
				*result = mystrdup(luacon_geterror());
				if (!console_mode)
					luacon_log(mystrdup(*result));
			}
		}
	}
	return 1;
}

//Being TPT interface methods:
int luatpt_test(lua_State* l)
{
	int testint = 0;
	testint = luaL_optint(l, 1, 0);
	printf("Test successful, got %d\n", testint);
	return 0;
}

int luatpt_getelement(lua_State *l)
{
	int t;
	const char* name;
	if (lua_isnumber(l, 1))
	{
		t = luaL_optint(l, 1, 1);
		if (t<0 || t>=PT_NUM)
			return luaL_error(l, "Unrecognised element number '%d'", t);
		name = ptypes[t&0xFF].name;
		if (t == PT_EXPL)
			lua_pushstring(l, "");
		else
			lua_pushstring(l, name);
	}
	else
	{
		luaL_checktype(l, 1, LUA_TSTRING);
		name = luaL_optstring(l, 1, "");
		if (!console_parse_type(name, &t, NULL))
			return luaL_error(l, "Unrecognised element '%s'", name);
		lua_pushinteger(l, t);
	}
	return 1;
}

int luatpt_element_func(lua_State *l)
{
	if (lua_isfunction(l, 1))
	{
		int element = luaL_optint(l, 2, 0);
		int replace = luaL_optint(l, 3, 0);
		int function;
		lua_pushvalue(l, 1);
		function = luaL_ref(l, LUA_REGISTRYINDEX);
		if (element > 0 && element < PT_NUM)
		{
			lua_el_func[element] = function;
			if (replace == 2)
				lua_el_mode[element] = 3; // update before
			else if (replace)
				lua_el_mode[element] = 2; // replace
			else
				lua_el_mode[element] = 1; // update after
			return 0;
		}
		else
		{
			return luaL_error(l, "Invalid element");
		}
	}
	else if (lua_isnil(l, 1))
	{
		int element = luaL_optint(l, 2, 0);
		if (element > 0 && element < PT_NUM)
		{
			lua_el_func[element] = 0;
			lua_el_mode[element] = 0;
		}
		else
		{
			return luaL_error(l, "Invalid element");
		}
	}
	else
		return luaL_error(l, "Not a function");
	return 0;
}

int luatpt_graphics_func(lua_State *l)
{
	if (lua_isfunction(l, 1))
	{
		int element = luaL_optint(l, 2, 0);
		int function;
		lua_pushvalue(l, 1);
		function = luaL_ref(l, LUA_REGISTRYINDEX);
		if (element > 0 && element < PT_NUM)
		{
			lua_gr_func[element] = function;
			graphicscache[element].isready = 0;
			return 0;
		}
		else
		{
			return luaL_error(l, "Invalid element");
		}
	}
	else if (lua_isnil(l, 1))
	{
		int element = luaL_optint(l, 2, 0);
		if (element > 0 && element < PT_NUM)
		{
			lua_gr_func[element] = 0;
			graphicscache[element].isready = 0;
		}
		else
		{
			return luaL_error(l, "Invalid element");
		}
	}
	else
		return luaL_error(l, "Not a function");
	return 0;
}

int luatpt_error(lua_State* l)
{
	char *error = mystrdup((char*)luaL_optstring(l, 1, "Error text"));
	error_ui(vid_buf, 0, error);
	free(error);
	return 0;
}

int luatpt_drawtext(lua_State* l)
{
	const char *string;
	int textx, texty, textred, textgreen, textblue, textalpha;
	textx = luaL_optint(l, 1, 0);
	texty = luaL_optint(l, 2, 0);
	string = luaL_optstring(l, 3, "");
	textred = luaL_optint(l, 4, 255);
	textgreen = luaL_optint(l, 5, 255);
	textblue = luaL_optint(l, 6, 255);
	textalpha = luaL_optint(l, 7, 255);

	if (textx<0 || texty<0 || textx>=XRES+BARSIZE || texty>=YRES+MENUSIZE)
		return luaL_error(l, "Screen coordinates out of range (%d,%d)", textx, texty);
	if (textred<0) textred = 0;
	else if (textred>255) textred = 255;
	if (textgreen<0) textgreen = 0;
	else if (textgreen>255) textgreen = 255;
	if (textblue<0) textblue = 0;
	else if (textblue>255) textblue = 255;
	if (textalpha<0) textalpha = 0;
	else if (textalpha>255) textalpha = 255;

	drawtext(vid_buf, textx, texty, string, textred, textgreen, textblue, textalpha);
	return 0;
}

int luatpt_create(lua_State* l)
{
	int x, y, retid, t = -1;
	x = abs(luaL_optint(l, 1, 0));
	y = abs(luaL_optint(l, 2, 0));
	if (x < XRES && y < YRES)
	{
		if (lua_isnumber(l, 3))
		{
			t = luaL_optint(l, 3, 0);
			if (t<0 || t >= PT_NUM || !ptypes[t].enabled)
				return luaL_error(l, "Unrecognised element number '%d'", t);
		}
		else
		{
			const char* name = luaL_optstring(l, 3, "dust");
			if (!console_parse_type(name, &t, NULL))
				return luaL_error(l,"Unrecognised element '%s'", name);
		}
		retid = create_part(-1, x, y, t);
		// failing to create a particle often happens (e.g. if space is already occupied) and isn't usually important, so don't raise an error
		lua_pushinteger(l, retid);
		return 1;
	}
	return luaL_error(l, "Coordinates out of range (%d,%d)", x, y);
}

int luatpt_setpause(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, sys_pause);
		return 1;
	}
	int pausestate = luaL_checkinteger(l, 1);
	sys_pause = (pausestate==0?0:1);
	return 0;
}

int luatpt_togglepause(lua_State* l)
{
	sys_pause = !sys_pause;
	lua_pushnumber(l, sys_pause);
	return 1;
}

int luatpt_togglewater(lua_State* l)
{
	water_equal_test = !water_equal_test;
	lua_pushnumber(l, water_equal_test);
	return 1;
}

int luatpt_setconsole(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, console_mode);
		return 1;
	}
	int consolestate = luaL_checkint(l, 1);
	console_mode = (consolestate==0?0:1);
	return 0;
}

int luatpt_log(lua_State* l)
{
	char *buffer = NULL, *buffer2 = NULL;
	int args = lua_gettop(l), i;
	for(i = 1; i <= args; i++)
	{
		luaL_tostring(l, -1);
		if(buffer)
		{
			buffer2 = (char*)malloc(strlen(luaL_optstring(l, -1, "")) + strlen(buffer) + 3);
			sprintf(buffer2, "%s, %s", luaL_optstring(l, -1, ""), buffer);
			free(buffer);
			buffer = buffer2;
		}
		else
		{
			buffer = mystrdup(luaL_optstring(l, -1, ""));
		}
		lua_pop(l, 2);
	}
	if (!buffer)
		buffer = mystrdup("");
	if (console_mode)
	{
		if(logs)
		{
			buffer2 = (char*)malloc(strlen(logs)+strlen(buffer)+3);
			sprintf(buffer2, "%s; %s", logs, buffer);
			free(logs);
			logs = buffer2;
		}
		else
		{
			logs = buffer;
		}
		return 0;
	}
	else
	{
		luacon_log(buffer);
		return 0;
	}
}

void set_map(int x, int y, int width, int height, float value, int map)
{
	int nx, ny;
	if(x > (XRES/CELL)-1)
		x = (XRES/CELL)-1;
	if(y > (YRES/CELL)-1)
		y = (YRES/CELL)-1;
	if(x+width > (XRES/CELL)-1)
		width = (XRES/CELL)-x;
	if(y+height > (YRES/CELL)-1)
		height = (YRES/CELL)-y;
	for (nx = x; nx<x+width; nx++)
		for (ny = y; ny<y+height; ny++)
		{
			if (map == 1)
				pv[ny][nx] = value;
			else if (map == 2)
				hv[ny][nx] = value;
			else if (map == 3)
				vx[ny][nx] = value;
			else if (map == 4)
				vy[ny][nx] = value;
			else if (map == 5)
				gravmap[ny*(XRES/CELL)+nx] = value; //TODO: setting gravity setting doesn't work anymore?

		}
}

int luatpt_set_pressure(lua_State* l)
{
	int x, y, width, height;
	float value;
	x = abs(luaL_optint(l, 1, 0));
	y = abs(luaL_optint(l, 2, 0));
	width = abs(luaL_optint(l, 3, XRES/CELL));
	height = abs(luaL_optint(l, 4, YRES/CELL));
	value = (float)luaL_optnumber(l, 5, 0);
	if(value > 256.0f)
		value = 256.0f;
	else if(value < -256.0f)
		value = -256.0f;

	set_map(x, y, width, height, value, 1);
	return 0;
}

int luatpt_set_aheat(lua_State* l)
{
	int x, y, width, height;
	float value;
	x = abs(luaL_optint(l, 1, 0));
	y = abs(luaL_optint(l, 2, 0));
	width = abs(luaL_optint(l, 3, XRES/CELL));
	height = abs(luaL_optint(l, 4, YRES/CELL));
	value = (float)luaL_optnumber(l, 5, 0);
	if(value > MAX_TEMP)
		value = MAX_TEMP;
	else if(value < MIN_TEMP)
		value = MIN_TEMP;

	set_map(x, y, width, height, value, 2);
	return 0;
}

int luatpt_set_velocity(lua_State* l)
{
	int x, y, width, height;
	float value;
	char *direction = (char*)luaL_optstring(l, 1, "");
	x = abs(luaL_optint(l, 2, 0));
	y = abs(luaL_optint(l, 3, 0));
	width = abs(luaL_optint(l, 4, XRES/CELL));
	height = abs(luaL_optint(l, 5, YRES/CELL));
	value = (float)luaL_optnumber(l, 6, 0);
	if(value > 256.0f)
		value = 256.0f;
	else if(value < -256.0f)
		value = -256.0f;

	if (!strcmp(direction,"x"))
		set_map(x, y, width, height, value, 3);
	else if (!strcmp(direction,"y"))
		set_map(x, y, width, height, value, 4);
	else
		return luaL_error(l, "Invalid direction: %s", direction);
	return 0;
}

int luatpt_set_gravity(lua_State* l)
{
	int x, y, width, height;
	float value;
	x = abs(luaL_optint(l, 1, 0));
	y = abs(luaL_optint(l, 2, 0));
	width = abs(luaL_optint(l, 3, XRES/CELL));
	height = abs(luaL_optint(l, 4, YRES/CELL));
	value = (float)luaL_optnumber(l, 5, 0);

	set_map(x, y, width, height, value, 5);
	return 0;
}

int luatpt_reset_gravity_field(lua_State* l)
{
	int nx, ny;
	int x1, y1, width, height;
	x1 = abs(luaL_optint(l, 1, 0));
	y1 = abs(luaL_optint(l, 2, 0));
	width = abs(luaL_optint(l, 3, XRES/CELL));
	height = abs(luaL_optint(l, 4, YRES/CELL));
	if(x1 > (XRES/CELL)-1)
		x1 = (XRES/CELL)-1;
	if(y1 > (YRES/CELL)-1)
		y1 = (YRES/CELL)-1;
	if(x1+width > (XRES/CELL)-1)
		width = (XRES/CELL)-x1;
	if(y1+height > (YRES/CELL)-1)
		height = (YRES/CELL)-y1;
	for (nx = x1; nx<x1+width; nx++)
		for (ny = y1; ny<y1+height; ny++)
		{
			gravx[ny*(XRES/CELL)+nx] = 0;
			gravy[ny*(XRES/CELL)+nx] = 0;
			gravp[ny*(XRES/CELL)+nx] = 0;
		}
	return 0;
}

int luatpt_reset_velocity(lua_State* l)
{
	int nx, ny;
	int x1, y1, width, height;
	x1 = abs(luaL_optint(l, 1, 0));
	y1 = abs(luaL_optint(l, 2, 0));
	width = abs(luaL_optint(l, 3, XRES/CELL));
	height = abs(luaL_optint(l, 4, YRES/CELL));
	if(x1 > (XRES/CELL)-1)
		x1 = (XRES/CELL)-1;
	if(y1 > (YRES/CELL)-1)
		y1 = (YRES/CELL)-1;
	if(x1+width > (XRES/CELL)-1)
		width = (XRES/CELL)-x1;
	if(y1+height > (YRES/CELL)-1)
		height = (YRES/CELL)-y1;
	for (nx = x1; nx<x1+width; nx++)
		for (ny = y1; ny<y1+height; ny++)
		{
			vx[ny][nx] = 0;
			vy[ny][nx] = 0;
		}
	return 0;
}

int luatpt_reset_spark(lua_State* l)
{
	int i;
	for (i = 0; i < NPART; i++)
	{
		if (parts[i].type == PT_SPRK)
		{
			if (parts[i].ctype >= 0 && parts[i].ctype < PT_NUM && globalSim->elements[parts[i].ctype].Enabled)
			{
				parts[i].type = parts[i].ctype;
				parts[i].life = parts[i].ctype = 0;
			}
			else
				kill_part(i);
		}
	}
	return 0;
}

int luatpt_set_property(lua_State* l)
{
	const char *prop, *name;
	int r, i, x, y, w, h, t = 0, format, nx, ny, partsel = 0, acount;
	float f = 0.0f;
	size_t offset;
	acount = lua_gettop(l);
	prop = luaL_optstring(l, 1, "");

	offset = Particle_GetOffset(prop, &format);
	if (offset == -1)
		return luaL_error(l, "Invalid property '%s'", prop);
	else if (format == 3)
		format = 0;

	if (acount > 2)
	{
		if (!lua_isnumber(l, acount) && lua_isstring(l, acount))
		{
			name = luaL_optstring(l, acount, "none");
			if (!console_parse_type(name, &partsel, NULL))
				return luaL_error(l, "Unrecognised element '%s'", name);
		}
	}
	if (lua_isnumber(l, 2))
	{
		if (format == 1)
			f = (float)luaL_optnumber(l, 2, 0);
		else
			t = luaL_optint(l, 2, 0);

		if (format == 2 && (t<0 || t>=PT_NUM || !ptypes[t].enabled))
			return luaL_error(l, "Unrecognised element number '%d'", t);
	}
	else
	{
		name = luaL_checklstring(l, 2, NULL);
		if (!console_parse_type(name, &t, NULL))
			return luaL_error(l, "Unrecognised element '%s'", name);
	}
	if (!lua_isnumber(l, 3) || acount >= 6)
	{
		// Got a region
		if (acount < 6)
		{
			i = 0;
			y = 0;
			w = XRES;
			h = YRES;
		}
		else
		{
			i = abs(luaL_checkint(l, 3));
			y = abs(luaL_checkint(l, 4));
			w = abs(luaL_checkint(l, 5));
			h = abs(luaL_checkint(l, 6));
		}
		if (i>=XRES || y>=YRES)
			return luaL_error(l, "Coordinates out of range (%d,%d)", i, y);
		x = i;
		if (x+w > XRES)
			w = XRES-x;
		if (y+h > YRES)
			h = YRES-y;
		for (i = 0; i < NPART; i++)
		{
			if (parts[i].type)
			{
				nx = (int)(parts[i].x + .5f);
				ny = (int)(parts[i].y + .5f);
				if (nx >= x && nx < x+w && ny >= y && ny < y+h && (!partsel || partsel == parts[i].type))
				{
					if (format == 1)
						*((float*)(((unsigned char*)&parts[i])+offset)) = f;
					else if (format == 0)
						*((int*)(((unsigned char*)&parts[i])+offset)) = t;
					else if (format == 2)
						globalSim->part_change_type_force(i, t);
				}
			}
		}
	}
	else
	{
		i = abs(luaL_checkint(l, 3));
		// Got coords or particle index
		if (lua_isnumber(l, 4))
		{
			y = abs(luaL_checkint(l, 4));
			if (i>=XRES || y>=YRES)
				return luaL_error(l, "Coordinates out of range (%d,%d)", i, y);
			r = pmap[y][i];
			if (!r || (partsel && partsel != (r&0xFF)))
				r = photons[y][i];
			if (!r || (partsel && partsel != (r&0xFF)))
				return 0;
			i = r>>8;
		}
		if (i < 0 || i >= NPART)
			return luaL_error(l, "Invalid particle ID '%d'", i);
		if (!parts[i].type)
			return 0;
		if (partsel && partsel != parts[i].type)
			return 0;

		if (format == 1)
			*((float*)(((unsigned char*)&parts[i])+offset)) = f;
		else if (format == 0)
			*((int*)(((unsigned char*)&parts[i])+offset)) = t;
		else if (format == 2)
			globalSim->part_change_type_force(i, t);
	}
	return 0;
}

int luatpt_get_property(lua_State* l)
{
	int i, r, y;
	const char *prop = luaL_optstring(l, 1, "");
	i = luaL_optint(l, 2, 0);
	y = luaL_optint(l, 3, -1);
	if (y!=-1 && y < YRES && y >= 0 && i < XRES && i >= 0)
	{
		r = pmap[y][i];
		if (!r)
			r = photons[y][i];
		if (!r)
		{
			if (!strcmp(prop,"type"))
			{
				lua_pushinteger(l, 0);
				return 1;
			}
			return luaL_error(l, "Particle does not exist");
		}
		i = r>>8;
	}
	else if (y!=-1)
		return luaL_error(l, "Coordinates out of range (%d,%d)", i, y);
	if (i < 0 || i >= NPART)
		return luaL_error(l, "Invalid particle ID '%d'", i);
	if (parts[i].type)
	{
		int format, tempinteger;
		float tempfloat;
		int offset = Particle_GetOffset(prop, &format);

		if (offset == -1)
		{
			if (!strcmp(prop,"id"))
			{
				lua_pushnumber(l, i);
				return 1;
			}
			else
				return luaL_error(l, "Invalid property");
		}
		switch(format)
		{
		case 0:
		case 2:
		case 3:
			tempinteger = *((int*)(((unsigned char*)&parts[i])+offset));
			lua_pushnumber(l, tempinteger);
			break;
		case 1:
			tempfloat = *((float*)(((unsigned char*)&parts[i])+offset));
			lua_pushnumber(l, tempfloat);
			break;
		}
		return 1;
	}
	else if (!strcmp(prop,"type"))
	{
		lua_pushinteger(l, 0);
		return 1;
	}
	return luaL_error(l, "Particle does not exist");
}

int luatpt_drawpixel(lua_State* l)
{
	int x, y, r, g, b, a;
	x = luaL_optint(l, 1, 0);
	y = luaL_optint(l, 2, 0);
	r = luaL_optint(l, 3, 255);
	g = luaL_optint(l, 4, 255);
	b = luaL_optint(l, 5, 255);
	a = luaL_optint(l, 6, 255);

	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return luaL_error(l, "Screen coordinates out of range (%d,%d)", x, y);
	if (r<0) r = 0;
	else if (r>255) r = 255;
	if (g<0) g = 0;
	else if (g>255) g = 255;
	if (b<0) b = 0;
	else if (b>255) b = 255;
	if (a<0) a = 0;
	else if (a>255) a = 255;

	drawpixel(vid_buf, x, y, r, g, b, a);
	return 0;
}

int luatpt_drawrect(lua_State* l)
{
	int x, y, w, h, r, g, b, a;
	x = luaL_optint(l, 1, 0);
	y = luaL_optint(l, 2, 0);
	w = luaL_optint(l, 3, 10);
	h = luaL_optint(l, 4, 10);
	r = luaL_optint(l, 5, 255);
	g = luaL_optint(l, 6, 255);
	b = luaL_optint(l, 7, 255);
	a = luaL_optint(l, 8, 255);

	if (x<0 || y<0 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return luaL_error(l, "Screen coordinates out of range (%d,%d)", x, y);
	if(x+w > XRES+BARSIZE)
		w = XRES+BARSIZE-x;
	if(y+h > YRES+MENUSIZE)
		h = YRES+MENUSIZE-y;
	if (r<0) r = 0;
	else if (r>255) r = 255;
	if (g<0) g = 0;
	else if (g>255) g = 255;
	if (b<0) b = 0;
	else if (b>255) b = 255;
	if (a<0) a = 0;
	else if (a>255) a = 255;

	drawrect(vid_buf, x, y, w, h, r, g, b, a);
	return 0;
}

int luatpt_fillrect(lua_State* l)
{
	int x,y,w,h,r,g,b,a;
	x = luaL_optint(l, 1, 0);
	y = luaL_optint(l, 2, 0);
	w = luaL_optint(l, 3, 10);
	h = luaL_optint(l, 4, 10);
	r = luaL_optint(l, 5, 255);
	g = luaL_optint(l, 6, 255);
	b = luaL_optint(l, 7, 255);
	a = luaL_optint(l, 8, 255);

	if (x<-1 || y<-1 || x>=XRES+BARSIZE || y>=YRES+MENUSIZE)
		return luaL_error(l, "Screen coordinates out of range (%d,%d)", x, y);
	if(x+w > XRES+BARSIZE)
		w = XRES+BARSIZE-x;
	if(y+h > YRES+MENUSIZE)
		h = YRES+MENUSIZE-y;
	if (r<0) r = 0;
	else if (r>255) r = 255;
	if (g<0) g = 0;
	else if (g>255) g = 255;
	if (b<0) b = 0;
	else if (b>255) b = 255;
	if (a<0) a = 0;
	else if (a>255) a = 255;

	fillrect(vid_buf, x, y, w, h, r, g, b, a);
	return 0;
}

int luatpt_drawcircle(lua_State* l)
{
	return graphics_drawCircle(l);
}

int luatpt_fillcircle(lua_State* l)
{
	return graphics_fillCircle(l);
}

int luatpt_drawline(lua_State* l)
{
	int x1,y1,x2,y2,r,g,b,a;
	x1 = luaL_optint(l, 1, 0);
	y1 = luaL_optint(l, 2, 0);
	x2 = luaL_optint(l, 3, 10);
	y2 = luaL_optint(l, 4, 10);
	r = luaL_optint(l, 5, 255);
	g = luaL_optint(l, 6, 255);
	b = luaL_optint(l, 7, 255);
	a = luaL_optint(l, 8, 255);

	//Don't need to check coordinates, as they are checked in blendpixel
	if (r<0) r = 0;
	else if (r>255) r = 255;
	if (g<0) g = 0;
	else if (g>255) g = 255;
	if (b<0) b = 0;
	else if (b>255) b = 255;
	if (a<0) a = 0;
	else if (a>255) a = 255;

	blend_line(vid_buf, x1, y1, x2, y2, r, g, b, a);
	return 0;
}

int luatpt_textwidth(lua_State* l)
{
	int strwidth = 0;
	const char* string = (char*)luaL_optstring(l, 1, "");
	strwidth = textwidth(string);
	lua_pushinteger(l, strwidth);
	return 1;
}

int luatpt_get_name(lua_State* l)
{
	if (svf_login)
	{
		lua_pushstring(l, svf_user);
		return 1;
	}
	lua_pushstring(l, "");
	return 1;
}

int luatpt_set_shortcuts(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, sys_shortcuts);
		return 1;
	}
	int shortcut = luaL_checkinteger(l, 1);
	sys_shortcuts = (shortcut==0?0:1);
	return 0;
}

int luatpt_delete(lua_State* l)
{
	int arg1, arg2;
	arg1 = abs(luaL_optint(l, 1, 0));
	arg2 = luaL_optint(l, 2, -1);
	if (arg2 == -1 && arg1 < NPART)
	{
		kill_part(arg1);
		return 0;
	}
	arg2 = abs(arg2);
	if (arg2 < YRES && arg1 < XRES)
	{
		globalSim->part_delete(arg1, arg2);
		return 0;
	}
	return luaL_error(l,"Invalid coordinates or particle ID");
}

int luatpt_register_step(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		int c;
		lua_pushstring(l, "stepfunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			lua_newtable(l);
			lua_pushstring(l, "stepfunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		c = lua_objlen(l, -1);
		lua_pushvalue(l, 1);
		lua_rawseti(l, -2, c+1);
	}
	return 0;
}

int luatpt_unregister_step(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		lua_pushstring(l, "stepfunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, -1);
			lua_newtable(l);
			lua_pushstring(l, "stepfunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		int len = lua_objlen(l, -1);
		int adjust = 0;
		for (int i = 1; i <= len; i++)
		{
			lua_rawgeti(l, -1, i+adjust);
			//unregister the function
			if (lua_equal(l, 1, -1))
			{
				lua_pop(l, 1);
				adjust++;
				i--;
			}
			//else, move everything down if we removed something earlier
			else
			{
				lua_rawseti(l, -2, i);
			}
		}
	}
	return 0;
}

int luatpt_register_keypress(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		int c;
		lua_pushstring(l, "keyfunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			lua_newtable(l);
			lua_pushstring(l, "keyfunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		c = lua_objlen(l, -1);
		lua_pushvalue(l, 1);
		lua_rawseti(l, -2, c+1);
	}
	return 0;
}

int luatpt_unregister_keypress(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		int c, d= 0, i;
		lua_pushstring(l, "keyfunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			lua_newtable(l);
			lua_pushstring(l, "keyfunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		c = lua_objlen(l, -1);
		for (i=1;i<=c;i++)
		{
			lua_rawgeti(l, -1, i+d);
			if (lua_equal(l, 1, -1))
			{
				lua_pop(l, 1);
				d++;
				i--;
			}
			else
				lua_rawseti(l, -2, i);
		}
	}
	return 0;
}

int luatpt_register_mouseclick(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		int c;
		lua_pushstring(l, "mousefunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			lua_newtable(l);
			lua_pushstring(l, "mousefunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		c = lua_objlen(l, -1);
		lua_pushvalue(l, 1);
		lua_rawseti(l, -2, c+1);
	}
	return 0;
}

int luatpt_unregister_mouseclick(lua_State* l)
{
	if (lua_isfunction(l, 1))
	{
		int c, d = 0, i;
		lua_pushstring(l, "mousefunctions");
		lua_rawget(l, LUA_REGISTRYINDEX);
		if (!lua_istable(l, -1))
		{
			lua_pop(l, 1);
			lua_newtable(l);
			lua_pushstring(l, "mousefunctions");
			lua_pushvalue(l, -2);
			lua_rawset(l, LUA_REGISTRYINDEX);
		}
		c = lua_objlen(l, -1);
		for (i=1;i<=c;i++)
		{
			lua_rawgeti(l, -1, i+d);
			if (lua_equal(l, 1, -1))
			{
				lua_pop(l, 1);
				d++;
				i--;
			}
			else
				lua_rawseti(l, -2, i);
		}
	}
	return 0;
}

int luatpt_input(lua_State* l)
{
	char *prompt, *title, *result, *shadow, *text;
	title = mystrdup((char*)luaL_optstring(l, 1, "Title"));
	prompt = mystrdup((char*)luaL_optstring(l, 2, "Enter some text:"));
	text = mystrdup((char*)luaL_optstring(l, 3, ""));
	shadow = mystrdup((char*)luaL_optstring(l, 4, ""));

	result = input_ui(vid_buf, title, prompt, text, shadow);
	lua_pushstring(l, result);
	free(result);
	free(title);
	free(prompt);
	free(text);
	free(shadow);
	return 1;
}

int luatpt_message_box(lua_State* l)
{
	char *title, *text;
	title = mystrdup((char*)luaL_optstring(l, 1, "Title"));
	text = mystrdup((char*)luaL_optstring(l, 2, "Message"));

	info_ui(vid_buf, title, text);
	free(title);
	free(text);
	return 0;
}

int luatpt_get_numOfParts(lua_State* l)
{
	lua_pushinteger(l, NUM_PARTS);
	return 1;
}

int luatpt_start_getPartIndex(lua_State* l)
{
	getPartIndex_curIdx = -1;
	return 1;
}

int luatpt_next_getPartIndex(lua_State* l)
{
	while (1)
	{
		getPartIndex_curIdx++;
		if (getPartIndex_curIdx >= NPART)
		{
			getPartIndex_curIdx = -1;
			lua_pushboolean(l, 0);
			return 1;
		}
		if (parts[getPartIndex_curIdx].type)
			break;

	}

	lua_pushboolean(l, 1);
	return 1;
}

int luatpt_getPartIndex(lua_State* l)
{
	if(getPartIndex_curIdx < 0)
	{
		lua_pushinteger(l, -1);
		return 1;
	}
	lua_pushinteger(l, getPartIndex_curIdx);
	return 1;
}

int luatpt_hud(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, hud_enable);
		return 1;
	}
	int hudstate = luaL_checkint(l, 1);
	hud_enable = (hudstate==0?0:1);
	return 0;
}

int luatpt_gravity(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, ngrav_enable);
		return 1;
	}
	int gravstate = luaL_checkint(l, 1);
	if(gravstate)
		start_grav_async();
	else
		stop_grav_async();
	ngrav_enable = (gravstate==0?0:1);
	return 0;
}

int luatpt_airheat(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, aheat_enable);
		return 1;
	}
	int aheatstate = luaL_checkint(l, 1);
	aheat_enable = (aheatstate==0?0:1);
	return 0;
}

int luatpt_active_menu(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, active_menu);
		return 1;
	}
	int menuid = luaL_checkint(l, 1);
	if (menuid < SC_TOTAL && menuid >= 0)
		active_menu = menuid;
	else
		return luaL_error(l, "Invalid menu");
	return 0;
}

int luatpt_decorations_enable(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, decorations_enable);
		return 1;
	}
	int decostate = luaL_checkint(l, 1);
	decorations_enable = (decostate==0?0:1);
	return 0;
}

int luatpt_heat(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, !legacy_enable);
		return 1;
	}
	int heatstate = luaL_checkint(l, 1);
	legacy_enable = (heatstate==1?0:1);
	return 0;
}

int luatpt_cmode_set(lua_State* l)
{
	int cmode = luaL_optint(l, 1, CM_FIRE);
	set_cmode(cmode);
	return 0;
}

int luatpt_setfire(lua_State* l)
{
	int firesize = luaL_optint(l, 2, 4);
	float fireintensity = (float)luaL_optnumber(l, 1, 1.0f);
	prepare_alpha(firesize, fireintensity);
	return 0;
}

int luatpt_setdebug(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, debug_flags);
		return 1;
	}
	int debug = luaL_checkint(l, 1);
	debug_flags = debug;
	return 0;
}

int luatpt_setfpscap(lua_State* l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, limitFPS);
		return 1;
	}
	int fpscap = luaL_checkint(l, 1);
	if (fpscap < 2)
		return luaL_error(l, "fps cap too small");
	limitFPS = fpscap;
	return 0;
}

int luatpt_getscript(lua_State* l)
{
	char *fileid = NULL, *filedata = NULL, *fileuri = NULL, *fileauthor = NULL, *filename = NULL, *lastError = NULL, *luacommand = NULL;
	int len, ret,run_script;
	FILE * outputfile;

	fileauthor = mystrdup((char*)luaL_optstring(l, 1, ""));
	fileid = mystrdup((char*)luaL_optstring(l, 2, ""));
	run_script = luaL_optint(l, 3, 0);
	if(!fileauthor || !fileid || strlen(fileauthor)<1 || strlen(fileid)<1)
		goto fin;
	if(!confirm_ui(vid_buf, "Do you want to install script?", fileid, "Install"))
		goto fin;

	fileuri = (char*)malloc(strlen(SCRIPTSERVER)+strlen(fileauthor)+strlen(fileid)+44);
	sprintf(fileuri, "http://" SCRIPTSERVER "/GetScript.api?Author=%s&Filename=%s", fileauthor, fileid);
	
	filedata = (char*)http_auth_get(fileuri, svf_user_id, NULL, svf_session_id, &ret, &len);
	
	if(len <= 0 || !filedata)
	{
		lastError = "Server did not return data.";
		goto fin;
	}
	if(ret != 200)
	{
		lastError = (char*)http_ret_text(ret);
		goto fin;
	}
	
	filename = (char*)malloc(strlen(fileauthor)+strlen(fileid)+strlen(PATH_SEP)+strlen(LOCAL_LUA_DIR)+6);
	sprintf(filename, LOCAL_LUA_DIR PATH_SEP "%s_%s.lua", fileauthor, fileid);
	
#ifdef WIN
	_mkdir(LOCAL_LUA_DIR);
#else
	mkdir(LOCAL_LUA_DIR, 0755);
#endif
	
	outputfile = fopen(filename, "r");
	if(outputfile)
	{
		fclose(outputfile);
		outputfile = NULL;
		if(confirm_ui(vid_buf, "File already exists, overwrite?", filename, "Overwrite"))
		{
			outputfile = fopen(filename, "w");
		}
		else
		{
			goto fin;
		}
	}
	else
	{
		outputfile = fopen(filename, "w");
	}
	
	if(!outputfile)
	{
		lastError = "Unable to write to file";
		goto fin;
	}
	
	
	fputs(filedata, outputfile);
	fclose(outputfile);
	outputfile = NULL;
	if(run_script)
	{
		char *result = NULL;
		luacommand = (char*)malloc(strlen(filename)+20);
		sprintf(luacommand,"dofile(\"%s\")",filename);
		luacon_eval(luacommand, &result);
		if (result)
			free(result);
	}
	
fin:
	if(fileid) free(fileid);
	if(filedata) free(filedata);
	if(fileuri) free(fileuri);
	if(fileauthor) free(fileauthor);
	if(filename) free(filename);
	if(luacommand) free(luacommand);
	luacommand = NULL;
		
	if(lastError) return luaL_error(l, lastError);
	return 0;
}

int luatpt_setwindowsize(lua_State* l)
{
	int result, scale = luaL_optint(l,1,1), kiosk = luaL_optint(l,2,0);
	if (scale!=2) scale = 1;
	if (kiosk!=1) kiosk = 0;
	result = set_scale(scale, kiosk);
	lua_pushnumber(l, result);
	return 1;
}

int luatpt_screenshot(lua_State* l)
{
	int captureUI = luaL_optint(l, 1, 0);

	if(captureUI)
	{
		dump_frame(vid_buf, XRES+BARSIZE, YRES+MENUSIZE, XRES+BARSIZE);
	}
	else
	{
		dump_frame(vid_buf, XRES, YRES, XRES+BARSIZE);
	}
	return 0;
}

int luatpt_getclip(lua_State* l)
{
	lua_pushstring(l, clipboard_pull_text());
	return 1;
}

int luatpt_setclip(lua_State* l)
{
	luaL_checktype(l, 1, LUA_TSTRING);
	clipboard_push_text((char*)luaL_optstring(l, 1, ""));
	return 0;
}

int luatpt_bubble(lua_State* l)
{
	int x = luaL_optint(l, 1, 0);
	int y = luaL_optint(l, 1, 0);
	int first, rem1, rem2, i;

	first = create_part(-1, x+18, y, PT_SOAP);
	rem1 = first;

	for (i = 1; i<=30; i++)
	{
		rem2 = create_part(-1, (int)(x+18*cosf(i/5.0f)), (int)(y+18*sinf(i/5.0f)), PT_SOAP);

		parts[rem1].ctype = 7;
		parts[rem1].tmp = rem2;
		parts[rem2].tmp2 = rem1;

		rem1 = rem2;
	}

	parts[rem1].ctype = 7;
	parts[rem1].tmp = first;
	parts[first].tmp2 = rem1;
	parts[first].ctype = 7;
	return 0;
}

int luatpt_maxframes(lua_State* l)
{
	int maxFrames = luaL_optint(l,1,-1), i;
	if (maxFrames == -1)
	{
		lua_pushnumber(l, globalSim->maxFrames);
		return 1;
	}
	if (maxFrames > 0 && maxFrames <= 256)
		globalSim->maxFrames = maxFrames;
	else
		return luaL_error(l, "must be between 1 and 256");
	for (i = 0; i <= globalSim->parts_lastActiveIndex; i++)
		if (parts[i].type == PT_ANIM)
		{
			if (parts[i].animations)
			{
				free(parts[i].animations);
				parts[i].animations = NULL;
			}
			parts[i].tmp2 = parts[i].ctype = 0;
			parts[i].tmp = 1;
		}
	return 0;
}

int luatpt_createwall(lua_State* l)
{
	int acount, wx, wy, wt, width = 1, height = 1, nx, ny;
	acount = lua_gettop(l);
	wx = luaL_optint(l,1,-1);
	wy = luaL_optint(l,2,-1);
	if (acount > 3)
	{
		width = luaL_optint(l,3,1);
		height = luaL_optint(l,4,1);
	}
	if (lua_isnumber(l, acount))
		wt = luaL_optint(l,acount,WL_WALL);
	else
	{
		const char* name = luaL_optstring(l, acount, "WALL");
		if (!console_parse_wall_type(name, &wt))
			return luaL_error(l, "Unrecognised wall '%s'", name);
	}
	if (wx < 0 || wx >= XRES/CELL || wy < 0 || wy >= YRES/CELL)
		return luaL_error(l, "coordinates out of range (%d,%d)", wx, wy);
	if (wx+width > (XRES/CELL))
		width = (XRES/CELL)-wx;
	if (wy+height > (YRES/CELL))
		height = (YRES/CELL)-wy;
	if (wt < 0 || wt >= WALLCOUNT || (wallTypes[wt].drawstyle == -1 && !secret_els))
		return luaL_error(l, "Unrecognised wall number %d", wt);
	for (nx = wx; nx < wx+width; nx++)
		for (ny = wy; ny < wy+height; ny++)
			bmap[ny][nx] = wt;
	return 0;
}

int luatpt_set_elecmap(lua_State* l)
{
	int acount = lua_gettop(l);
	int x1 = luaL_optint(l,1,-1);
	int y1 = luaL_optint(l,2,-1);
	int width = luaL_optint(l,3,1);
	int height = luaL_optint(l,4,1);
	int value = luaL_optint(l,acount,0);
	int nx, ny;

	if (x1 < 0 || x1 >= XRES/CELL || y1 < 0 || y1 >= YRES/CELL)
		return luaL_error(l, "coordinates out of range (%d,%d)", x1, y1);

	if (x1+width > (XRES/CELL))
		width = (XRES/CELL)-x1;
	if (y1+height > (YRES/CELL))
		height = (YRES/CELL)-y1;

	for (nx = x1; nx < x1+width; nx++)
		for (ny = y1; ny < y1+height; ny++)
			emap[ny][nx] = value;
	return 0;
}

int luatpt_getwall(lua_State* l)
{
	int wx = luaL_optint(l,1,-1);
	int wy = luaL_optint(l,2,-1);
	if (wx < 0 || wx > XRES/CELL || wy < 0 || wy > YRES/CELL)
		return luaL_error(l, "coordinates out of range (%d,%d)", wx, wy);
	lua_pushnumber(l, bmap[wy][wx]);
	return 1;
}

int luatpt_get_elecmap(lua_State* l)
{
	int wx = luaL_optint(l,1,-1);
	int wy = luaL_optint(l,2,-1);
	if (wx < 0 || wx > XRES/CELL || wy < 0 || wy > YRES/CELL)
		return luaL_error(l, "coordinates out of range (%d,%d)", wx, wy);
	lua_pushinteger(l, emap[wy][wx]);
	return 1;
}

int luatpt_indestructible(lua_State* l)
{
	int el = 0, ind;
	if(lua_isnumber(l, 1))
	{
		el = luaL_optint(l, 1, 0);
		if (el<0 || el>=PT_NUM)
			return luaL_error(l, "Unrecognised element number '%d'", el);
	}
	else
	{
		const char* name = luaL_optstring(l, 1, "dust");
		if (!console_parse_type(name, &el, NULL))
			return luaL_error(l, "Unrecognised element '%s'", name);
	}
	ind = luaL_optint(l, 2, 1);
	if (ind)
	{
		ptypes[el].properties |= PROP_INDESTRUCTIBLE;
		globalSim->elements[el].Properties |= PROP_INDESTRUCTIBLE;
	}
	else
	{
		ptypes[el].properties &= ~PROP_INDESTRUCTIBLE;
		globalSim->elements[el].Properties &= ~PROP_INDESTRUCTIBLE;
	}
	return 0;
}

int luatpt_oldmenu(lua_State *l)
{
	int acount = lua_gettop(l);
	if (acount == 0)
	{
		lua_pushnumber(l, old_menu);
		return 1;
	}
	int oldmenu = luaL_checkint(l, 1);
	old_menu = oldmenu;
	return 0;
}

char* LuaCode = NULL;
int LuaCodeLen = 0;
bool ranLuaCode = true;
void ReadLuaCode()
{
	if (!file_exists("luacode.txt"))
	{
		error_ui(vid_buf, 0, "Place some code in luacode.txt");
		return;
	}
	char* code = (char*)file_load("luacode.txt", &LuaCodeLen);
	if (!code)
	{
		error_ui(vid_buf, 0, "Error reading luacode.txt");
		return;
	}
	if (LuaCode)
	{
		free(LuaCode);
		LuaCode = NULL;
	}
	// lua bytecode starts with byte 27, don't allow since can't be read and can do strange things
	if (code[0] == '\x1b')
	{
		error_ui(vid_buf, 0, "Lua bytecode detected");
		return;
	}
	LuaCode = code;
	ranLuaCode = false;
}

void ExecuteEmbededLuaCode()
{
	if (!ranLuaCode && LuaCode)
	{
		FILE* previewCode = fopen("newluacode.txt", "w");
		ranLuaCode = true;
		if (!previewCode)
		{
			error_ui(vid_buf, 0, "Could not write code to newluacode.txt");
			return;
		}
		fwrite(LuaCode, LuaCodeLen, 1, previewCode);
		fclose(previewCode);
		bool runCode = confirm_ui(vid_buf, "Lua code", "Run the lua code in newluacode.txt?", "Run");
		// lua bytecode starts with byte 27, don't allow since can't be read and can do strange things
		if (LuaCode[0] == '\x1b')
		{
			error_ui(vid_buf, 0, "Lua bytecode detected");
			free(LuaCode);
			LuaCode = NULL;
			return;
		}
		if (!runCode)
			return;

		//whitelist of functions we allow. Hopefully safe but just in case we write it to newluacode.txt above and ask the user to check it
		if (luaL_dostring(l,"\n\
			env = {\n\
				print = print,\n\
				ipairs = ipairs,\n\
				next = next,\n\
				pairs = pairs,\n\
				pcall = pcall,\n\
				tonumber = tonumber,\n\
				tostring = tostring,\n\
				type = type,\n\
				unpack = unpack,\n\
				coroutine = { create = coroutine.create, resume = coroutine.resume, \n\
					running = coroutine.running, status = coroutine.status, \n\
					wrap = coroutine.wrap }, \n\
				string = { byte = string.byte, char = string.char, find = string.find, \n\
					format = string.format, gmatch = string.gmatch, gsub = string.gsub, \n\
					len = string.len, lower = string.lower, match = string.match, \n\
					rep = string.rep, reverse = string.reverse, sub = string.sub, \n\
					upper = string.upper },\n\
				table = { insert = table.insert, maxn = table.maxn, remove = table.remove, \n\
					sort = table.sort },\n\
				math = { abs = math.abs, acos = math.acos, asin = math.asin, \n\
					atan = math.atan, atan2 = math.atan2, ceil = math.ceil, cos = math.cos, \n\
					cosh = math.cosh, deg = math.deg, exp = math.exp, floor = math.floor, \n\
					fmod = math.fmod, frexp = math.frexp, huge = math.huge, \n\
					ldexp = math.ldexp, log = math.log, log10 = math.log10, max = math.max, \n\
					min = math.min, modf = math.modf, pi = math.pi, pow = math.pow, \n\
					rad = math.rad, random = math.random, randomseed = math.randomseed, sin = math.sin, sinh = math.sinh, \n\
					sqrt = math.sqrt, tan = math.tan, tanh = math.tanh },\n\
				os = { clock = os.clock, difftime = os.difftime, time = os.time, date = os.date, exit = os.exit },\n\
				tpt = tpt,\n\
				sim = sim, simulation = simulation,\n\
				elem = elem, elements = elements,\n\
				gfx = gfx, graphics = graphics,\n\
				ren = ren, renderer = renderer,\n\
				bit = bit,\n\
				socket = { gettime = socket.gettime }} --[[I think socket.gettime() is safe?]]\n\
				\n\
			function run(untrusted_code)\n\
				if not untrusted_code then return nil end\n\
				setfenv(untrusted_code, env)\n\
				untrusted_code()\n\
			end\n\
			"))
			luacon_log(mystrdup(luacon_geterror())); //if large above thing errored

		loop_time = SDL_GetTicks();
		if (luaL_dostring(l, "run(loadfile(\"newluacode.txt\"))"))
		{
			luacon_log(mystrdup(luacon_geterror()));
		}
	}
}

#endif
