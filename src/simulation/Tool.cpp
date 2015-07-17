#include <sstream>
#include "Tool.h"
#include "graphics.h"
#include "interface.h"
#include "powder.h"
#include "graphics/ARGBColour.h"
#include "Simulation.h"
#include "WallNumbers.h"
#include "ToolNumbers.h"
#include "GolNumbers.h"
#include "game/Brush.h"

Tool::Tool(int toolID, std::string toolIdentifier):
	identifier(toolIdentifier),
	type(INVALID_TOOL),
	ID(toolID)
{

}

Tool::Tool(int toolType, int toolID, std::string toolIdentifier):
	identifier(toolIdentifier),
	type(toolType),
	ID(toolID)
{

}

int Tool::DrawPoint(Brush* brush, Point position)
{
	return globalSim->CreateParts(position.X, position.Y, ID, get_brush_flags(), true, brush);
}

void Tool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	if (held)
	{
		if (ID == PT_MOVS)
			return;
	}
	else
	{
		if (ID == PT_LIGH)
			return;
	}
	globalSim->CreateLine(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, get_brush_flags(), brush);
}

void Tool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	switch (ID)
	{
	case PT_LIGH:
		return;
	case PT_TESC:
	{
		int radiusInfo = brush->GetRadius().X*4+brush->GetRadius().Y*4+7;
		globalSim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, ID | (radiusInfo<<8), get_brush_flags());
		break;
	}
	default:
		globalSim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, get_brush_flags());
		break;
	}
}

int Tool::FloodFill(Brush* brush, Point position)
{
	switch (ID)
	{
	case PT_LIGH:
	case PT_PWHT:
	case PT_MOVS:
		return 0;
	case PT_TESC:
	{
		int radiusInfo = brush->GetRadius().X*4+brush->GetRadius().Y*4+7;
		return globalSim->FloodParts(position.X, position.Y, ID | (radiusInfo<<8), -1, get_brush_flags());
	}
	default:
		return globalSim->FloodParts(position.X, position.Y, ID, -1, get_brush_flags());
	}
}

void Tool::Click(Point position)
{

}

Tool* Tool::Sample(Point position)
{
	if (position.Y < 0 || position.Y >= YRES || position.X < 0 || position.X >= XRES)
		return this;

	int sample = pmap[position.Y][position.X];
	if (sample || (sample = photons[position.Y][position.X]))
	{
		if ((sample&0xFF) == PT_LIFE)
		{
			if (parts[sample>>8].ctype < NGOL)
				return GetToolFromIdentifier(golTypes[parts[sample>>8].ctype].identifier);
		}
		else
		{
			return GetToolFromIdentifier(globalSim->elements[sample&0xFF].Identifier);
		}
	}
	else if (bmap[position.Y/CELL][position.X/CELL] > 0 && bmap[position.Y/CELL][position.X/CELL] < WALLCOUNT)
	{
		return GetToolFromIdentifier(wallTypes[bmap[position.Y / CELL][position.X / CELL]].identifier);
	}
	return this;
}


ElementTool::ElementTool(int elementID):
	Tool(ELEMENT_TOOL, elementID, globalSim->elements[elementID].Identifier)
{

}
int ElementTool::GetID()
{
	if (type == ELEMENT_TOOL)
		return ID;
	else if (type == GOL_TOOL)
		return PT_LIFE;
	else
		return -1;
}


PlopTool::PlopTool(int elementID):
	ElementTool(elementID)
{

}
int PlopTool::DrawPoint(Brush* brush, Point position)
{
	return 0;
}
void PlopTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{

}

void PlopTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{

}

int PlopTool::FloodFill(Brush* brush, Point position)
{
	return 0;
}
void PlopTool::Click(Point position)
{
	globalSim->part_create(-1, position.X, position.Y, ID);
}


GolTool::GolTool(int golID):
	Tool(GOL_TOOL, golID, golTypes[golID].identifier)
{

}
int GolTool::GetID()
{
	if (type == GOL_TOOL)
		return ID;
	//else if (type == ELEMENT_TOOL)
	//	return PT_LIFE;
	else
		return -1;
}
int GolTool::DrawPoint(Brush* brush, Point position)
{
	return globalSim->CreateParts(position.X, position.Y, PT_LIFE | (ID<<8), get_brush_flags(), true, brush);
}
void GolTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	globalSim->CreateLine(startPos.X, startPos.Y, endPos.X, endPos.Y, PT_LIFE | (ID<<8), get_brush_flags(), brush);
}
void GolTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	globalSim->CreateBox(startPos.X, startPos.Y, endPos.X, endPos.Y, PT_LIFE | (ID<<8), get_brush_flags());
}
int GolTool::FloodFill(Brush* brush, Point position)
{
	return globalSim->FloodParts(position.X, position.Y, PT_LIFE+(ID<<8), -1, get_brush_flags());
}


WallTool::WallTool(int wallID):
	Tool(WALL_TOOL, wallID, wallTypes[wallID].identifier)
{

}
int WallTool::DrawPoint(Brush* brush, Point position)
{
	int rx = brush->GetRadius().X/CELL;
	int ry = brush->GetRadius().Y/CELL;
	int x = position.X/CELL;
	int y = position.Y/CELL;
	globalSim->CreateWallBox(x-rx, y-ry, x+rx, y+ry, ID);
	return 1;
}
void WallTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	if (!held && ID == WL_FAN && bmap[startPos.Y/CELL][startPos.X/CELL] == WL_FAN)
	{
		float nfvx = (endPos.X-startPos.X)*0.005f;
		float nfvy = (endPos.Y-startPos.Y)*0.005f;
		globalSim->FloodWalls(startPos.X/CELL, startPos.Y/CELL, WL_FANHELPER, WL_FAN);
		for (int j=0; j<YRES/CELL; j++)
			for (int i=0; i<XRES/CELL; i++)
				if (bmap[j][i] == WL_FANHELPER)
				{
					fvx[j][i] = nfvx;
					fvy[j][i] = nfvy;
					bmap[j][i] = WL_FAN;
				}
	}
	else
	{
		globalSim->CreateWallLine(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, brush->GetRadius().X/CELL, brush->GetRadius().Y/CELL, ID);
	}
}
void WallTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	globalSim->CreateWallBox(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, ID);
}
int WallTool::FloodFill(Brush* brush, Point position)
{
	return globalSim->FloodWalls(position.X/CELL, position.Y/CELL, ID, -1);
}

StreamlineTool::StreamlineTool():
	WallTool(WL_STREAM)
{

}
int StreamlineTool::DrawPoint(Brush* brush, Point position)
{
	globalSim->CreateWall(position.X, position.Y, ID);
	return 0;
}
void StreamlineTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	globalSim->CreateWallLine(startPos.X/CELL, startPos.Y/CELL, endPos.X/CELL, endPos.Y/CELL, 0, 0, ID);
}
int StreamlineTool::FloodFill(Brush* brush, Point position)
{
	return 0;
}

ToolTool::ToolTool(int toolID):
	Tool(TOOL_TOOL, toolID, toolTypes[toolID].identifier)
{

}
int ToolTool::DrawPoint(Brush* brush, Point position)
{
	if (ID == TOOL_SIGN)
		return 1;
	globalSim->CreateToolBrush(position.X, position.Y, ID, toolStrength, brush);
	return 0;
}
void ToolTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	if ((ID == TOOL_WIND && !held) || ID == TOOL_SIGN)
		return;
	globalSim->CreateToolLine(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, toolStrength, brush);
}
void ToolTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	if (ID == TOOL_SIGN)
		return;
	globalSim->CreateToolBox(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, toolStrength);
}
int ToolTool::FloodFill(Brush* brush, Point position)
{
	return 0;
}
void ToolTool::Click(Point position)
{
	if (ID == TOOL_SIGN)
	{
		add_sign_ui(vid_buf, position.X, position.Y);
	}
}

PropTool::PropTool():
ToolTool(TOOL_PROP),
propType(Integer),
propOffset(0)
{
	propValue.Integer = 0;
}
int PropTool::DrawPoint(Brush* brush, Point position)
{
	globalSim->CreatePropBrush(position.X, position.Y, propType, propValue, propOffset, brush);
	return 0;
}
void PropTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	globalSim->CreatePropLine(startPos.X, startPos.Y, endPos.X, endPos.Y, propType, propValue, propOffset, brush);
}
void PropTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	globalSim->CreatePropBox(startPos.X, startPos.Y, endPos.X, endPos.Y, propType, propValue, propOffset);
}
int PropTool::FloodFill(Brush* brush, Point position)
{
	return globalSim->FloodProp(position.X, position.Y, propType, propValue, propOffset);
}


DecoTool::DecoTool(int decoID):
Tool(DECO_TOOL, decoID, decoTypes[decoID].identifier)
{

}
int DecoTool::DrawPoint(Brush* brush, Point position)
{
	ARGBColour col = (ID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolour;
	globalSim->CreateDecoBrush(position.X, position.Y, ID, col, brush);
	return 0;
}
void DecoTool::DrawLine(Brush* brush, Point startPos, Point endPos, bool held)
{
	ARGBColour col = (ID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolour;
	globalSim->CreateDecoLine(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, col, brush);
}
void DecoTool::DrawRect(Brush* brush, Point startPos, Point endPos)
{
	ARGBColour col = (ID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolour;
	globalSim->CreateDecoBox(startPos.X, startPos.Y, endPos.X, endPos.Y, ID, col);
}
int DecoTool::FloodFill(Brush* brush, Point position)
{
	PropertyValue col;
	col.UInteger = (ID == DECO_CLEAR) ? COLARGB(0, 0, 0, 0) : decocolour;
	return globalSim->FloodProp(position.X, position.Y, UInteger, col, offsetof(particle, dcolour));
}
Tool* DecoTool::Sample(Point position)
{
	if (position.Y < 0 || position.Y >= YRES || position.X < 0 || position.X >= XRES)
		return this;

	int cr = PIXR(sampleColor);
	int cg = PIXG(sampleColor);
	int cb = PIXB(sampleColor);
	if (cr || cg || cb)
	{
		if (cr && cr<255) cr++;
		if (cg && cg<255) cg++;
		if (cb && cb<255) cb++;
		decocolour = COLARGB(255, cr, cg, cb);
		currR = cr, currG = cg, currB = cb, currA = 255;
		RGB_to_HSV(currR, currG, currB, &currH, &currS, &currV);
	}
	return this;
}
