#ifdef LUACONSOLE
#ifndef LUASCRIPT_H
#define LUASCRIPT_H

#include <luaconsole.h>

// idea from mniip, makes things much simpler 
#define SETCONST(L, NAME)\
	lua_pushinteger(L, NAME);\
	lua_setfield(L, -2, #NAME)

int simulation_signIndex(lua_State *l);
int simulation_signNewIndex(lua_State *l);
int simulation_newsign(lua_State *l);
void initSimulationAPI(lua_State * l);
int simulation_partNeighbours(lua_State * l);
int simulation_partChangeType(lua_State * l);
int simulation_partCreate(lua_State * l);
int simulation_partID(lua_State * l);
int simulation_partProperty(lua_State * l);
int simulation_partPosition(lua_State * l);
int simulation_partKill(lua_State * l);
int simulation_pressure(lua_State* l);
int simulation_ambientHeat(lua_State* l);
int simulation_velocityX(lua_State* l);
int simulation_velocityY(lua_State* l);
int simulation_gravMap(lua_State* l);
int simulation_createParts(lua_State * l);
int simulation_createLine(lua_State * l);
int simulation_createBox(lua_State * l);
int simulation_floodParts(lua_State * l);
int simulation_createWalls(lua_State * l);
int simulation_createWallLine(lua_State * l);
int simulation_createWallBox(lua_State * l);
int simulation_floodWalls(lua_State * l);
int simulation_toolBrush(lua_State * l);
int simulation_toolLine(lua_State * l);
int simulation_toolBox(lua_State * l);
int simulation_decoBrush(lua_State * l);
int simulation_decoLine(lua_State * l);
int simulation_decoBox(lua_State * l);
int simulation_floodDeco(lua_State * l);
int simulation_decoColor(lua_State * l);
int simulation_clearSim(lua_State * l);
int simulation_clearRect(lua_State * l);
int simulation_resetTemp(lua_State * l);
int simulation_resetPressure(lua_State * l);
int simulation_saveStamp(lua_State * l);
int simulation_loadStamp(lua_State * l);
int simulation_deleteStamp(lua_State * l);
int simulation_loadSave(lua_State * l);
int simulation_reloadSave(lua_State * l);
int simulation_getSaveID(lua_State * l);
int simulation_adjustCoords(lua_State * l);
int simulation_prettyPowders(lua_State * l);
int simulation_gravityGrid(lua_State * l);
int simulation_edgeMode(lua_State * l);
int simulation_gravityMode(lua_State * l);
int simulation_airMode(lua_State * l);
int simulation_waterEqualization(lua_State * l);
int simulation_ambientAirTemp(lua_State * l);
int simulation_elementCount(lua_State* l);
int simulation_canMove(lua_State * l);
int simulation_parts(lua_State * l);
int simulation_pmap(lua_State * l);
int simulation_photons(lua_State * l);
int simulation_framerender(lua_State * l);
int simulation_neighbours(lua_State * l);
int simulation_stickman(lua_State * l);

void initRendererAPI(lua_State * l);
int renderer_renderModes(lua_State * l);
int renderer_displayModes(lua_State * l);
int renderer_colourMode(lua_State * l);
int renderer_decorations(lua_State * l);
int renderer_grid(lua_State * l);
int renderer_debugHUD(lua_State * l);

void initFileSystemAPI(lua_State * l);
int fileSystem_list(lua_State * l);
int fileSystem_exists(lua_State * l);
int fileSystem_isFile(lua_State * l);
int fileSystem_isDirectory(lua_State * l);
int fileSystem_makeDirectory(lua_State * l);
int fileSystem_removeDirectory(lua_State * l);
int fileSystem_removeFile(lua_State * l);
int fileSystem_move(lua_State * l);
int fileSystem_copy(lua_State * l);

void initGraphicsAPI(lua_State * l);
int graphics_textSize(lua_State * l);
int graphics_drawText(lua_State * l);
int graphics_drawLine(lua_State * l);
int graphics_drawRect(lua_State * l);
int graphics_fillRect(lua_State * l);
int graphics_drawCircle(lua_State * l);
int graphics_fillCircle(lua_State * l);
int graphics_getColors(lua_State * l);
int graphics_getHexColor(lua_State * l);
int graphics_toolTip(lua_State * l);

int elements_getProperty(const char * key, int * format, unsigned int * modifiedStuff);
void elements_setProperty(lua_State * l, int id, int format, int offset);
void elements_writeProperty(lua_State *l, int id, int format, int offset);
void initElementsAPI(lua_State * l);
int elements_allocate(lua_State * l);
int elements_element(lua_State * l);
int elements_property(lua_State * l);
int elements_loadDefault(lua_State * l);
int elements_free(lua_State * l);

#endif
#endif
