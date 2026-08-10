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

#include "common/tpt-minmax.h"
#include <bzlib.h>
#include <climits>
#include <cmath>
#include <memory>
#include "Save.h"
#include "BSON.h"
#include "defines.h"
#include "hmap.h" // for firw_data
#include "misc.h" // For restrict_flt

#include "common/Format.h"
#include "common/Platform.h"
#include "common/Version.h"
#include "simulation/ElementNumbers.h"
#include "simulation/GolNumbers.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/ToolNumbers.h"
#include "simulation/WallNumbers.h"

#include "simulation/elements/PPIP.h"

using namespace Matrix;

// Used for creating saves from save data. Loading stamps / online saves, for example
Save::Save(const char * saveData, unsigned int saveSize)
{
	this->saveData = new unsigned char[saveSize];
	std::copy(&saveData[0], &saveData[saveSize], &this->saveData[0]);
	this->saveSize = saveSize;
	expanded = false;
	InitData();
	InitVars();
}

// Used for creating brand new saves. Simulation::CreateSave will fill in the necessary data
Save::Save(int blockW, int blockH)
{
	this->saveData = NULL;
	this->saveSize = 0;
	expanded = false;
	SetSize(blockW, blockH);
	InitVars();
}

Save::Save(const Save & save):
	expanded(save.expanded),
	fromNewerVersion(save.fromNewerVersion),
	hasPressure(save.hasPressure),
	hasAmbientHeat(save.hasAmbientHeat),
	luaCode(save.luaCode),
	logMessages(save.logMessages),
	adminLogMessages(save.adminLogMessages),
	createdVersion(save.createdVersion),
	modCreatedVersion(save.modCreatedVersion),
	androidCreatedVersion(save.androidCreatedVersion),
	leftSelectedIdentifier(save.leftSelectedIdentifier),
	rightSelectedIdentifier(save.rightSelectedIdentifier),
	saveInfo(save.saveInfo),
	saveInfoPresent(save.saveInfoPresent),

	legacyEnable(save.legacyEnable),
	gravityEnable(save.gravityEnable),
	aheatEnable(save.aheatEnable),
	waterEEnabled(save.waterEEnabled),
	paused(save.paused),
	gravityMode(save.gravityMode),
	customGravityX(save.customGravityX),
	customGravityY(save.customGravityY),
	airMode(save.airMode),
	ambientAirTemp(save.ambientAirTemp),
	ambientAirTempPresent(save.ambientAirTempPresent),
	edgePressure(save.edgePressure),
	edgePressurePresent(save.edgePressurePresent),
	edgeVelocityX(save.edgeVelocityX),
	edgeVelocityY(save.edgeVelocityY),
	edgeVelocityPresent(save.edgeVelocityPresent),
	vorticityCoeff(save.vorticityCoeff),
	convectionMode(save.convectionMode),
	convectionModePresent(save.convectionModePresent),
	edgeMode(save.edgeMode),
	hudEnable(save.hudEnable),
	hudEnablePresent(save.hudEnablePresent),
	msRotation(save.msRotation),
	msRotationPresent(save.msRotationPresent),
	activeMenu(save.activeMenu),
	activeMenuPresent(save.activeMenuPresent),
	decorationsEnable(save.decorationsEnable),
	decorationsEnablePresent(save.decorationsEnablePresent),
	legacyHeatSave(save.legacyHeatSave),
	signs(save.signs),
	stkm(save.stkm),
	palette(save.palette),
	sim(save.sim),
	renderMode(save.renderMode),
	renderModePresent(save.renderModePresent),
	displayMode(save.displayMode),
	displayModePresent(save.displayModePresent),
	colorMode(save.colorMode),
	colorModePresent(save.colorModePresent),
	MOVSdata(save.MOVSdata),
	ANIMdata(save.ANIMdata),
	pmapbits(save.pmapbits)
{
	InitData();
	if (save.expanded)
	{
		SetSize(save.blockWidth, save.blockHeight);

		std::copy(save.particles, save.particles+NPART, particles);
		for (unsigned int j = 0; j < blockHeight; j++)
		{
			std::copy(save.blockMap[j], save.blockMap[j]+blockWidth, blockMap[j]);
			std::copy(save.fanVelX[j], save.fanVelX[j]+blockWidth, fanVelX[j]);
			std::copy(save.fanVelY[j], save.fanVelY[j]+blockWidth, fanVelY[j]);
			std::copy(save.pressure[j], save.pressure[j]+blockWidth, pressure[j]);
			std::copy(save.velocityX[j], save.velocityX[j]+blockWidth, velocityX[j]);
			std::copy(save.velocityY[j], save.velocityY[j]+blockWidth, velocityY[j]);
			std::copy(save.ambientHeat[j], save.ambientHeat[j]+blockWidth, ambientHeat[j]);
		}
	}
	else
	{
		blockWidth = save.blockWidth;
		blockHeight = save.blockHeight;
	}
	particlesCount = save.particlesCount;
	authors = save.authors;

	if (save.saveData)
	{
		saveData = new unsigned char[save.saveSize];
		std::copy(&save.saveData[0], &save.saveData[save.saveSize], &saveData[0]);
		saveSize = save.saveSize;
	}
	else
	{
		saveData = nullptr;
	}
}

Save::~Save()
{
	delete[] saveData;
	Dealloc();
}

void Save::Dealloc()
{
	if (particles)
	{
		delete[] particles;
		particles = NULL;
	}
	Deallocate2DArray<unsigned char>(&blockMap, blockHeight);
	Deallocate2DArray<float>(&fanVelX, blockHeight);
	Deallocate2DArray<float>(&fanVelY, blockHeight);
	Deallocate2DArray<float>(&pressure, blockHeight);
	Deallocate2DArray<float>(&velocityX, blockHeight);
	Deallocate2DArray<float>(&velocityY, blockHeight);
	Deallocate2DArray<float>(&ambientHeat, blockHeight);
	expanded = false;
}

// Called on every new GameSave, including the copy constructor
void Save::InitData()
{
	blockWidth = blockHeight = 0;
	particlesCount = 0;

	blockMap = NULL;
	fanVelX = NULL;
	fanVelY = NULL;
	particles = NULL;
	pressure = NULL;
	velocityX = NULL;
	velocityY = NULL;
	ambientHeat = NULL;
	//fromNewerVersion = false;
	//hasAmbientHeat = false;
	//authors.clear();
}

void Save::SetSize(int newWidth, int newHeight)
{
	blockWidth = newWidth;
	blockHeight = newHeight;

	particlesCount = 0;
	particles = new particle[NPART];

	blockMap = Allocate2DArray<unsigned char>(blockWidth, blockHeight, 0);
	fanVelX = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
	fanVelY = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
	pressure = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
	velocityX = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
	velocityY = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
	ambientHeat = Allocate2DArray<float>(blockWidth, blockHeight, 0.0f);
}

// Called on every new GameSave, except the copy constructor
void Save::InitVars()
{
	createdVersion = 0;
	modCreatedVersion = 0;
	androidCreatedVersion = 0;

	waterEEnabled = false;
	legacyEnable = false;
	gravityEnable = false;
	aheatEnable = false;
	paused = false;
	gravityMode = GRAV_VERTICAL;
	customGravityX = 0.0f;
	customGravityY = 0.0f;
	airMode = AIR_ON;
	ambientAirTemp = R_TEMP + 273.15;
	edgePressure = 0.0f;
	edgeVelocityX = edgeVelocityY = 0.0f;
	vorticityCoeff = 0.0f;
	convectionMode = AIRC_LEGACY;
	edgeMode = EDGE_VOID;
	hasPressure = false;
	hasAmbientHeat = false;
	// jacob1's mod
	hudEnable = true;
	hudEnablePresent = false;
	msRotation = false;
	msRotationPresent = false;
	activeMenu = -1;
	activeMenuPresent = false;
	decorationsEnable = true;
	decorationsEnablePresent = false;
	legacyHeatSave = false;

	saveInfoPresent = false;

	renderMode = 0;
	renderModePresent = false;
	displayMode = 0;
	displayModePresent = false;
	colorMode = 0;
	colorModePresent = false;

	translated.x = translated.y = 0;
	pmapbits = 8;
}

const unsigned char * const Save::GetSaveData()
{
	if (expanded && !saveData)
		BuildSave();
	return saveData;
}

unsigned int Save::GetSaveSize()
{
	if (expanded && !saveData)
		BuildSave();
	return saveSize;
}

int Save::FixType(int type) const
{
	// invalid element, we don't care about it
	if (type < 0 || type > PT_NUM)
		return type;
	else if (modCreatedVersion)
	{
		int max = 161;
		if (modCreatedVersion == 18)
		{
			if (type >= 190 && type <= 204)
				return PT_LOLZ;
		}

		if (createdVersion >= 90)
			max = 179;
		else if (createdVersion >= 89 || modCreatedVersion >= 16)
			max = 177;
		else if (createdVersion >= 87)
			max = 173;
		else if (createdVersion >= 86 || modCreatedVersion == 14)
			max = 170;
		else if (createdVersion >= 84 || modCreatedVersion == 13)
			max = 167;
		else if (modCreatedVersion == 12)
			max = 165;
		else if (createdVersion >= 83)
			max = 163;
		else if (createdVersion >= 82)
			max = 162;

		if (type >= max)
		{
			type += (PT_NORMAL_NUM-max);
		}
		//change VIRS into official elements, and CURE into SOAP; adjust ids
		if (modCreatedVersion <= 15)
		{
			if (type >= PT_NORMAL_NUM+6 && type <= PT_NORMAL_NUM+8)
				type = PT_VIRS + type-(PT_NORMAL_NUM+6);
			else if (type == PT_NORMAL_NUM+9)
				type = PT_SOAP;
			else if (type > PT_NORMAL_NUM+9)
				type -= 4;
		}
		//change GRVT and DRAY into official elements
		if (modCreatedVersion <= 19)
		{
			if (type >= PT_NORMAL_NUM+12 && type <= PT_NORMAL_NUM+13)
				type -= 14;
		}
		//change OTWR and COND into METL, adjust ids
		if (modCreatedVersion <= 20)
		{
			if (type == PT_NORMAL_NUM+3 || type == PT_NORMAL_NUM+9)
				type = PT_METL;
			else if (type > PT_NORMAL_NUM+3 && type < PT_NORMAL_NUM+9)
				type--;
			else if (type > PT_NORMAL_NUM+9)
				type -= 2;
		}
	}
	return type;
}

int Save::ChangeWall(int wt)
{
	if (wt == 1)
		return WL_WALL;
	else if (wt == 2)
		return WL_DESTROYALL;
	else if (wt == 3)
		return WL_ALLOWLIQUID;
	else if (wt == 4)
		return WL_FAN;
	else if (wt == 5)
		return WL_STREAM;
	else if (wt == 6)
		return WL_DETECT;
	else if (wt == 7)
		return WL_EWALL;
	else if (wt == 8)
		return WL_WALLELEC;
	else if (wt == 9)
		return WL_ALLOWAIR;
	else if (wt == 10)
		return WL_ALLOWPOWDER;
	else if (wt == 11)
		return WL_ALLOWALLELEC;
	else if (wt == 12)
		return WL_EHOLE;
	else if (wt == 13)
		return WL_ALLOWGAS;
	return wt;
}

int Save::ChangeWallpp(int wt)
{
	if (wt == O_WL_WALLELEC)
		return WL_WALLELEC;
	else if (wt == O_WL_EWALL)
		return WL_EWALL;
	else if (wt == O_WL_DETECT)
		return WL_DETECT;
	else if (wt == O_WL_STREAM)
		return WL_STREAM;
	else if (wt == O_WL_FAN)
		return WL_FAN;
	else if (wt == O_WL_ALLOWLIQUID)
		return WL_ALLOWLIQUID;
	else if (wt == O_WL_DESTROYALL)
		return WL_DESTROYALL;
	else if (wt == O_WL_WALL)
		return WL_WALL;
	else if (wt == O_WL_ALLOWAIR)
		return WL_ALLOWAIR;
	else if (wt == O_WL_ALLOWSOLID)
		return WL_ALLOWPOWDER;
	else if (wt == O_WL_ALLOWALLELEC)
		return WL_ALLOWALLELEC;
	else if (wt == O_WL_EHOLE)
		return WL_EHOLE;
	else if (wt == O_WL_ALLOWGAS)
		return WL_ALLOWGAS;
	else if (wt == O_WL_GRAV)
		return WL_GRAV;
	else if (wt == O_WL_ALLOWENERGY)
		return WL_ALLOWENERGY;
	return wt;
}

void Save::ParseSave()
{
	if (expanded)
		return;

	if (!saveData)
		throw ParseException("Save data doesn't exist");

	if (saveSize < 12)
		throw ParseException("Save too small");

	if ((saveData[0] == 0x66 && saveData[1] == 0x75 && saveData[2] == 0x43) || (saveData[0] == 0x50 && saveData[1] == 0x53 && saveData[2] == 0x76))
	{
		ParseSavePSv();
	}
	else if (saveData[0] == 'O' && saveData[1] == 'P' && saveData[2] == 'S')
	{
		if (saveData[3] != '1')
			throw ParseException("Save format from newer version");
		ParseSaveOPS();
	}
	else
	{
		throw ParseException("Invalid save format");
	}
	expanded = true;
}

void Save::CheckBsonFieldUser(bson_iterator iter, const char *field, unsigned char **data, unsigned int *fieldLen)
{
	if (!strcmp(bson_iterator_key(&iter), field))
	{
		if (bson_iterator_type(&iter) == BSON_BINDATA && ((unsigned char)bson_iterator_bin_type(&iter)) == BSON_BIN_USER && (*fieldLen = bson_iterator_bin_len(&iter)) > 0)
		{
			*data = (unsigned char*)bson_iterator_bin_data(&iter);
		}
		else
		{
			fprintf(stderr, "Invalid datatype for %s: %d[%d] %d[%d] %d[%d]\n", field, bson_iterator_type(&iter), bson_iterator_type(&iter) == BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter))==BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
		}
	}
}

bool Save::CheckBsonFieldBool(bson_iterator iter, const char *field, bool *flag)
{
	if (!strcmp(bson_iterator_key(&iter), field))
	{
		if (bson_iterator_type(&iter) == BSON_BOOL)
		{
			*flag = bson_iterator_bool(&iter);
			return true;
		}
		else
		{
			fprintf(stderr, "Wrong type for %s, expected bool, got type %i\n", bson_iterator_key(&iter),  bson_iterator_type(&iter));
		}
	}
	return false;
}

bool Save::CheckBsonFieldInt(bson_iterator iter, const char *field, int *setting)
{
	if (!strcmp(bson_iterator_key(&iter), field))
	{
		if (bson_iterator_type(&iter) == BSON_INT)
		{
			*setting = bson_iterator_int(&iter);
			return true;
		}
		else
		{
			fprintf(stderr, "Wrong type for %s, expected int, got type %i\n", bson_iterator_key(&iter),  bson_iterator_type(&iter));
		}
	}
	return false;
}

bool Save::CheckBsonFieldFloat(bson_iterator iter, const char *field, float *setting)
{
	if (!strcmp(bson_iterator_key(&iter), field))
	{
		if (bson_iterator_type(&iter) == BSON_DOUBLE)
		{
			*setting = float(bson_iterator_double(&iter));
			return true;
		}
		else
		{
			fprintf(stderr, "Wrong type for %s, expected double, got type %i\n", bson_iterator_key(&iter),  bson_iterator_type(&iter));
		}
	}
	return false;
}

void Save::ParseSaveOPS()
{
	unsigned char *bsonData = NULL, *partsData = NULL, *partsPosData = NULL, *fanData = NULL, *wallData = NULL, *soapLinkData = NULL;
	unsigned char *pressData = NULL, *vxData = NULL, *vyData = NULL, *ambientData = NULL;
	unsigned int bsonDataLen = 0, partsDataLen, partsPosDataLen, fanDataLen, wallDataLen, soapLinkDataLen;
	unsigned int pressDataLen, vxDataLen, vyDataLen, ambientDataLen = 0;
#ifndef NOMOD
	unsigned char *movsData = NULL, *animData = NULL;
	unsigned int movsDataLen, animDataLen;
#endif
	unsigned int blockX, blockY, blockW, blockH, fullX, fullY, fullW, fullH;
	createdVersion = saveData[4];
	Version version = { createdVersion, 0 };

	bson b;
	b.data = NULL;
	bson_iterator iter;
	auto bson_deleter = [](bson * b) { bson_destroy(b); };
	// Use unique_ptr with a custom deleter to ensure that bson_destroy is called even when an exception is thrown
	std::unique_ptr<bson, decltype(bson_deleter)> b_ptr(&b, bson_deleter);

	// Block sizes
	blockX = 0;
	blockY = 0;
	blockW = saveData[6];
	blockH = saveData[7];

	// Full size, normalized
	fullX = blockX*CELL;
	fullY = blockY*CELL;
	fullW = blockW*CELL;
	fullH = blockH*CELL;

	// Incompatible cell size
	if (saveData[5] != CELL)
	{
		throw ParseException("Incorrect CELL size");
	}

	if (blockW <= 0 || blockH <= 0)
		throw ParseException("Save too small");

	// Too large/off screen
	if (blockX+blockW > XRES/CELL || blockY+blockH > YRES/CELL)
	{
		throw ParseException("Save too large");
	}

	SetSize(blockW, blockH);

	bsonDataLen = ((unsigned)saveData[8]);
	bsonDataLen |= ((unsigned)saveData[9]) << 8;
	bsonDataLen |= ((unsigned)saveData[10]) << 16;
	bsonDataLen |= ((unsigned)saveData[11]) << 24;

	// Check for overflows, don't load saves larger than 200MB
	unsigned int toAlloc = bsonDataLen + 1;
	if (toAlloc > 209715200 || !toAlloc)
	{
		throw ParseException("Save data too large");
	}

	bsonData = (unsigned char*)malloc(bsonDataLen+1);
	if (!bsonData)
	{
		throw ParseException("Could not allocate memory");
	}
	// Make sure bsonData is null terminated, since all string functions need null terminated strings
	// (bson_iterator_key returns a pointer into bsonData, which is then used with strcmp)
	bsonData[bsonDataLen] = 0;

	int bz2ret;
	if ((bz2ret = BZ2_bzBuffToBuffDecompress((char*)bsonData, (unsigned*)(&bsonDataLen), (char*)saveData+12, saveSize-12, 0, 0)) != BZ_OK)
	{
		throw ParseException("Unable to decompress (ret " + Format::NumberToString<int>(bz2ret) + ")");
	}

	set_bson_err_handler([](const char* err) { throw ParseException("BSON error when parsing save: " + std::string(err)); });
	bson_init_data_size(&b, (char*)bsonData, bsonDataLen);
	bson_iterator_init(&iter, &b);
	while (bson_iterator_next(&iter))
	{
		CheckBsonFieldUser(iter, "parts", &partsData, &partsDataLen);
		CheckBsonFieldUser(iter, "partsPos", &partsPosData, &partsPosDataLen);
		CheckBsonFieldUser(iter, "wallMap", &wallData, &wallDataLen);
		CheckBsonFieldUser(iter, "pressMap", &pressData, &pressDataLen);
		CheckBsonFieldUser(iter, "vxMap", &vxData, &vxDataLen);
		CheckBsonFieldUser(iter, "vyMap", &vyData, &vyDataLen);
		CheckBsonFieldUser(iter, "ambientMap", &ambientData, &ambientDataLen);
		CheckBsonFieldUser(iter, "fanMap", &fanData, &fanDataLen);
		CheckBsonFieldUser(iter, "soapLinks", &soapLinkData, &soapLinkDataLen);
#ifndef NOMOD
		CheckBsonFieldUser(iter, "movs", &movsData, &movsDataLen);
		CheckBsonFieldUser(iter, "anim", &animData, &animDataLen);
#endif
		CheckBsonFieldBool(iter, "legacyEnable", &legacyEnable);
		CheckBsonFieldBool(iter, "gravityEnable", &gravityEnable);
		CheckBsonFieldBool(iter, "aheat_enable", &aheatEnable);
		CheckBsonFieldBool(iter, "waterEEnabled", &waterEEnabled);
		CheckBsonFieldBool(iter, "paused", &paused);
		msRotationPresent = CheckBsonFieldBool(iter, "msrotation", &msRotation) || msRotationPresent;
		hudEnablePresent = CheckBsonFieldBool(iter, "hud_enable", &hudEnable) || hudEnablePresent;
		CheckBsonFieldInt(iter, "gravityMode", &gravityMode);
		CheckBsonFieldFloat(iter, "customGravityX", &customGravityX);
		CheckBsonFieldFloat(iter, "customGravityY", &customGravityY);
		CheckBsonFieldInt(iter, "airMode", &airMode);
		ambientAirTempPresent = CheckBsonFieldFloat(iter, "ambientAirTemp", &ambientAirTemp) || ambientAirTempPresent;
		edgePressurePresent = CheckBsonFieldFloat(iter, "edgePressure", &edgePressure) || edgePressurePresent;
		edgeVelocityPresent = CheckBsonFieldFloat(iter, "edgeVelocityX", &edgeVelocityX) || edgeVelocityPresent;
		edgeVelocityPresent = CheckBsonFieldFloat(iter, "edgeVelocityY", &edgeVelocityY) || edgeVelocityPresent;
		CheckBsonFieldFloat(iter, "vorticityCoeff", &vorticityCoeff);
		convectionModePresent = CheckBsonFieldInt(iter, "convectionMode", &convectionMode) || convectionModePresent;
		CheckBsonFieldInt(iter, "edgeMode", &edgeMode);
		CheckBsonFieldInt(iter, "pmapbits", &pmapbits);
		activeMenuPresent = CheckBsonFieldInt(iter, "activeMenu", &activeMenu) || activeMenuPresent;
		decorationsEnablePresent = CheckBsonFieldBool(iter, "decorations_enable", &decorationsEnable) || decorationsEnablePresent;

		if (!strcmp(bson_iterator_key(&iter), "signs"))
		{
			if (bson_iterator_type(&iter) == BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (!strcmp(bson_iterator_key(&subiter), "sign"))
					{
						if (bson_iterator_type(&subiter) == BSON_OBJECT)
						{
							bson_iterator signiter;
							bson_iterator_subiterator(&subiter, &signiter);
							// Stop reading signs if we have no free spaces
							if (signs.size() >= MAXSIGNS)
								break;

							Sign theSign = Sign("", fullX, fullY, Sign::Middle);
							while (bson_iterator_next(&signiter))
							{
								if (!strcmp(bson_iterator_key(&signiter), "text") && bson_iterator_type(&signiter) == BSON_STRING)
								{
									theSign.SetText(Format::CleanString(bson_iterator_string(&signiter), true, true, true).substr(0, 45));
								}
								else if (!strcmp(bson_iterator_key(&signiter), "justification") && bson_iterator_type(&signiter) == BSON_INT)
								{
									int ju = bson_iterator_int(&signiter);
									if (ju >= 0 && ju <= 3)
										theSign.SetJustification((Sign::Justification)bson_iterator_int(&signiter));
								}
								else if (!strcmp(bson_iterator_key(&signiter), "x") && bson_iterator_type(&signiter) == BSON_INT)
								{
									theSign.SetPos(Point(bson_iterator_int(&signiter)+fullX, theSign.GetRealPos().Y));
								}
								else if (!strcmp(bson_iterator_key(&signiter), "y") && bson_iterator_type(&signiter) == BSON_INT)
								{
									theSign.SetPos(Point(theSign.GetRealPos().X, bson_iterator_int(&signiter)+fullY));
								}
								else
								{
									fprintf(stderr, "Unknown sign property %s\n", bson_iterator_key(&signiter));
								}
							}
							signs.push_back(theSign);
						}
						else
						{
							fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&subiter));
						}
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "stkm"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				bson_iterator stkmiter;
				bson_iterator_subiterator(&iter, &stkmiter);
				while (bson_iterator_next(&stkmiter))
				{
					CheckBsonFieldBool(stkmiter, "rocketBoots1", &stkm.rocketBoots1);
					CheckBsonFieldBool(stkmiter, "rocketBoots2", &stkm.rocketBoots2);
					CheckBsonFieldBool(stkmiter, "fan1", &stkm.fan1);
					CheckBsonFieldBool(stkmiter, "fan2", &stkm.fan2);
					if (!strcmp(bson_iterator_key(&stkmiter), "rocketBootsFigh") && bson_iterator_type(&stkmiter) == BSON_ARRAY)
					{
						bson_iterator fighiter;
						bson_iterator_subiterator(&stkmiter, &fighiter);
						while (bson_iterator_next(&fighiter))
						{
							if (bson_iterator_type(&fighiter) == BSON_INT)
								stkm.rocketBootsFigh.push_back(bson_iterator_int(&fighiter));
						}
					}
					else if (!strcmp(bson_iterator_key(&stkmiter), "fanFigh") && bson_iterator_type(&stkmiter) == BSON_ARRAY)
					{
						bson_iterator fighiter;
						bson_iterator_subiterator(&stkmiter, &fighiter);
						while (bson_iterator_next(&fighiter))
						{
							if (bson_iterator_type(&fighiter) == BSON_INT)
								stkm.fanFigh.push_back(bson_iterator_int(&fighiter));
						}
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
#ifndef NOMOD
#ifdef LUACONSOLE
		else if (!strcmp(bson_iterator_key(&iter), "LuaCode"))
		{
			if (bson_iterator_type(&iter) == BSON_BINDATA && (unsigned char)bson_iterator_bin_type(&iter) == BSON_BIN_USER && bson_iterator_bin_len(&iter) > 0)
			{
				luaCode = bson_iterator_bin_data(&iter);
			}
			else
			{
				fprintf(stderr, "Invalid datatype of anim data: %d[%d] %d[%d] %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter) == BSON_BINDATA, (unsigned char)bson_iterator_bin_type(&iter), ((unsigned char)bson_iterator_bin_type(&iter)) == BSON_BIN_USER, bson_iterator_bin_len(&iter), bson_iterator_bin_len(&iter)>0);
			}
		}
#endif
#endif
		else if (!strcmp(bson_iterator_key(&iter), "palette"))
		{
			palette.clear();
			if (bson_iterator_type(&iter) == BSON_ARRAY)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (bson_iterator_type(&subiter) == BSON_INT)
					{
						std::string id = std::string(bson_iterator_key(&subiter));
						int num = bson_iterator_int(&subiter);
						palette.push_back(PaletteItem(id, num));
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for element palette: %d[%d]\n", bson_iterator_type(&iter), bson_iterator_type(&iter)==BSON_ARRAY);
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "minimumVersion"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				int major = INT_MAX, minor = INT_MAX;
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (bson_iterator_type(&subiter) == BSON_INT)
					{
						if (!strcmp(bson_iterator_key(&subiter), "major"))
							major = bson_iterator_int(&subiter);
						else if (!strcmp(bson_iterator_key(&subiter), "minor"))
							minor = bson_iterator_int(&subiter);
					}
				}
				if (major > FAKE_SAVE_VERSION || (major == FAKE_SAVE_VERSION && minor > FAKE_MINOR_VER))
				{
					std::stringstream errorMessage;
					errorMessage << "Save from a newer version: Requires version " << major << "." << minor;
					if (loadIncompatibleSaves)
						logMessages.push_back(errorMessage.str());
					else
						throw ParseException(errorMessage.str());
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "leftSelectedElementIdentifier") || !strcmp(bson_iterator_key(&iter), "rightSelectedElementIdentifier"))
		{
			if (bson_iterator_type(&iter) == BSON_STRING)
			{
				if (bson_iterator_key(&iter)[0] == 'l')
				{
					leftSelectedIdentifier = bson_iterator_string(&iter);
				}
				else
				{
					rightSelectedIdentifier = bson_iterator_string(&iter);
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "Jacob1's_Mod"))
		{
			if (bson_iterator_type(&iter)==BSON_INT)
			{
				modCreatedVersion = bson_iterator_int(&iter);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "origin"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				bson_iterator subiter;
				bson_iterator_subiterator(&iter, &subiter);
				while (bson_iterator_next(&subiter))
				{
					if (!strcmp(bson_iterator_key(&subiter), "mobileBuildVersion") && bson_iterator_type(&subiter) == BSON_INT)
					{
						androidCreatedVersion =  bson_iterator_int(&subiter);
					}
					else if (!strcmp(bson_iterator_key(&subiter), "minorVersion") && bson_iterator_type(&subiter) == BSON_INT)
					{
						version[1] = bson_iterator_int(&subiter);
					}
				}
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "saveInfo"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				bson_iterator saveInfoiter;
				bson_iterator_subiterator(&iter, &saveInfoiter);
				while (bson_iterator_next(&saveInfoiter))
				{
					if (!strcmp(bson_iterator_key(&saveInfoiter), "saveOpened") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						saveInfo.SetSaveOpened(bson_iterator_bool(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "fileOpened") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						saveInfo.SetFileOpened(bson_iterator_bool(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "saveName") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetSaveName(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "fileName") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetFileName(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "published") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						saveInfo.SetPublished(bson_iterator_bool(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "ID") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetSaveID(Format::StringToNumber<int>(bson_iterator_string(&saveInfoiter)));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "version") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetSaveVersion(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "description") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetDescription(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "author") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetAuthor(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "tags") && bson_iterator_type(&saveInfoiter) == BSON_STRING)
						saveInfo.SetTags(bson_iterator_string(&saveInfoiter));
					else if (!strcmp(bson_iterator_key(&saveInfoiter), "myVote") && bson_iterator_type(&saveInfoiter) == BSON_INT)
						saveInfo.SetMyVote(bson_iterator_int(&saveInfoiter));
					else
						fprintf(stderr, "Unknown save info property %s\n", bson_iterator_key(&saveInfoiter));
				}
				saveInfoPresent = true;
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
		else if (!strcmp(bson_iterator_key(&iter), "render_mode") && bson_iterator_type(&iter) == BSON_INT)
		{
			renderMode = bson_iterator_int(&iter);
			renderModePresent = true;
		}
		else if (!strcmp(bson_iterator_key(&iter), "display_mode") && bson_iterator_type(&iter) == BSON_INT)
		{
			displayMode = bson_iterator_int(&iter);
			displayModePresent = true;
		}
		else if (!strcmp(bson_iterator_key(&iter), "color_mode") && bson_iterator_type(&iter) == BSON_INT)
		{
			colorMode = bson_iterator_int(&iter);
			colorModePresent = true;
		}
		else if (!strcmp(bson_iterator_key(&iter), "authors"))
		{
			if (bson_iterator_type(&iter) == BSON_OBJECT)
			{
				ConvertBsonToJson(&iter, &authors);
			}
			else
			{
				fprintf(stderr, "Wrong type for %s\n", bson_iterator_key(&iter));
			}
		}
	}

	auto paletteRemap = [this, version](auto maxVersion, std::string from, std::string to) {
		if (version <= maxVersion)
		{
			auto it = std::find_if(palette.begin(), palette.end(), [&from](auto &item) {
				return item.first == from;
			});
			if (it != palette.end())
			{
				it->first = to;
			}
		}
	};
	paletteRemap(Version(87, 1), "DEFAULT_PT_TUGN", "DEFAULT_PT_TUNG");
	paletteRemap(Version(90, 1), "DEFAULT_PT_REPL", "DEFAULT_PT_RPEL");
	paletteRemap(Version(92, 0), "DEFAULT_PT_E180", "DEFAULT_PT_HEAC");
	paletteRemap(Version(92, 0), "DEFAULT_PT_E181", "DEFAULT_PT_SAWD");
	paletteRemap(Version(92, 0), "DEFAULT_PT_E182", "DEFAULT_PT_POLO");
	paletteRemap(Version(93, 3), "DEFAULT_PT_RAYT", "DEFAULT_PT_LDTC");

	// Read wall and fan data
	if (wallData)
	{
		unsigned int j = 0;
		if (blockW * blockH > wallDataLen)
			throw ParseException("Not enough wall data");
		for (unsigned int x = 0; x < blockW; x++)
		{
			for (unsigned int y = 0; y < blockH; y++)
			{
				if (wallData[y*blockW+x])
				{
					int wt = ChangeWallpp(wallData[y*blockW+x]);
					if (wt < 0 || wt >= WALLCOUNT)
						continue;
					blockMap[blockY+y][blockX+x] = wt;
				}
				if (wallData[y*blockW+x] == WL_FAN && fanData)
				{
					if (j+1 >= fanDataLen)
					{
						throw ParseException("Not enough fan data");
					}
					fanVelX[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
					fanVelY[blockY+y][blockX+x] = (fanData[j++]-127.0f)/64.0f;
				}
			}
		}
	}
	
	// Read pressure data
	if (pressData)
	{
		unsigned int j = 0;
		unsigned int i, i2;
		if (blockW * blockH * 2 > pressDataLen)
			throw ParseException("Not enough pressure data");
		hasPressure = true;
		for (unsigned int x = 0; x < blockW; x++)
		{
			for (unsigned int y = 0; y < blockH; y++)
			{
				i = pressData[j++];
				i2 = pressData[j++];
				pressure[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	// Read vx data
	if (vxData)
	{
		unsigned int j = 0;
		unsigned int i, i2;
		if (blockW * blockH * 2 > vxDataLen)
			throw ParseException("Not enough vx data");
		for (unsigned int x = 0; x < blockW; x++)
		{
			for (unsigned int y = 0; y < blockH; y++)
			{
				i = vxData[j++];
				i2 = vxData[j++];
				velocityX[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	// Read vy data
	if (vyData)
	{
		unsigned int j = 0;
		unsigned int i, i2;
		if (blockW * blockH * 2 > vyDataLen)
			throw ParseException("Not enough vy data");
		for (unsigned int x = 0; x < blockW; x++)
		{
			for (unsigned int y = 0; y < blockH; y++)
			{
				i = vyData[j++];
				i2 = vyData[j++];
				velocityY[blockY+y][blockX+x] = ((i+(i2<<8))/128.0f)-256;
			}
		}
	}

	// Read ambient heat data
	if (ambientData)
	{
		unsigned int tempTemp, j = 0;
		if (blockW * blockH * 2 > ambientDataLen)
			throw ParseException("Not enough ambient heat data");
		hasAmbientHeat = true;
		for (unsigned int x = 0; x < blockW; x++)
		{
			for (unsigned int y = 0; y < blockH; y++)
			{
				tempTemp = ambientData[j++];
				tempTemp |= (((unsigned)ambientData[j++]) << 8);
				ambientHeat[blockY+y][blockX+x] = tempTemp;
			}
		}
	}

	// Read particle data
	if (partsData && partsPosData)
	{
		int newIndex = 0, tempTemp;
		int posCount, posTotal, partsPosDataIndex = 0;
		if (fullW * fullH * 3 > partsPosDataLen)
			throw ParseException("Not enough particle position data");
		unsigned int i = 0, x, y;
		for (unsigned int saved_y = 0; saved_y < fullH; saved_y++)
		{
			for (unsigned int saved_x = 0; saved_x < fullW; saved_x++)
			{
				// Read total number of particles at this position
				posTotal = 0;
				posTotal |= partsPosData[partsPosDataIndex++]<<16;
				posTotal |= partsPosData[partsPosDataIndex++]<<8;
				posTotal |= partsPosData[partsPosDataIndex++];
				// Put the next posTotal particles at this position
				for (posCount = 0; posCount < posTotal; posCount++)
				{
					// i+3 because we have 4 bytes of required fields (type (1), descriptor (2), temp (1))
					if (i+3 >= partsDataLen)
						throw ParseException("Ran past particle data buffer");
					x = saved_x + fullX;
					y = saved_y + fullY;
					unsigned int fieldDescriptor = (unsigned int)(partsData[i+1]);
					fieldDescriptor |= (unsigned int)(partsData[i+2]) << 8;
					if (x >= XRES || y >= YRES)
						throw ParseException("Particle out of range");

					if (newIndex < 0 || newIndex >= NPART)
						throw ParseException("Too many particles");

					// Clear the particle, ready for our new properties
					memset(&(particles[newIndex]), 0, sizeof(particle));
					
					// Required fields
					particles[newIndex].type = partsData[i];
					particles[newIndex].x = (float)x;
					particles[newIndex].y = (float)y;
					i+=3;

					// Read type (2nd byte)
					if (fieldDescriptor & 0x4000)
						particles[newIndex].type |= (((unsigned)partsData[i++]) << 8);

					// Read temp
					if (fieldDescriptor & 0x01)
					{
						// Full 16bit int
						tempTemp = partsData[i++];
						tempTemp |= (((unsigned)partsData[i++]) << 8);
						particles[newIndex].temp = (float)tempTemp;
					}
					else
					{
						// 1 Byte room temp offset
						tempTemp = partsData[i++];
						if (tempTemp >= 0x80)
							tempTemp -= 0x100;
						particles[newIndex].temp = tempTemp+294.15f;
					}

					// fieldDesc3
					if (fieldDescriptor & 0x8000)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading third byte of field descriptor");
						fieldDescriptor |= (unsigned int)(partsData[i++]) << 16;
					}

					// Read life
					if (fieldDescriptor & 0x02)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading life");
						particles[newIndex].life = partsData[i++];
						// Read 2nd byte
						if (fieldDescriptor & 0x04)
						{
							if (i >= partsDataLen)
								throw ParseException("Ran past particle data buffer while loading life");
							particles[newIndex].life |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					// Read tmp
					if (fieldDescriptor & 0x08)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading tmp");
						particles[newIndex].tmp = partsData[i++];
						// Read 2nd byte
						if (fieldDescriptor & 0x10)
						{
							if (i >= partsDataLen)
								throw ParseException("Ran past particle data buffer while loading tmp");
							particles[newIndex].tmp |= (((unsigned)partsData[i++]) << 8);
							// Read 3rd and 4th bytes
							if (fieldDescriptor & 0x1000)
							{
								if (i+1 >= partsDataLen)
									throw ParseException("Ran past particle data buffer while loading tmp");
								particles[newIndex].tmp |= (((unsigned)partsData[i++]) << 24);
								particles[newIndex].tmp |= (((unsigned)partsData[i++]) << 16);
							}
						}
					}
					
					// Read ctype
					if (fieldDescriptor & 0x20)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading ctype");
						particles[newIndex].ctype = partsData[i++];
						// Read additional bytes
						if (fieldDescriptor & 0x200)
						{
							if (i+2 >= partsDataLen)
								throw ParseException("Ran past particle data buffer while loading ctype");
							particles[newIndex].ctype |= (((unsigned)partsData[i++]) << 24);
							particles[newIndex].ctype |= (((unsigned)partsData[i++]) << 16);
							particles[newIndex].ctype |= (((unsigned)partsData[i++]) << 8);
						}
					}
					
					// Read dcolor
					if (fieldDescriptor & 0x40)
					{
						if (i+3 >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading deco");
						unsigned char alpha = partsData[i++];
						unsigned char red = partsData[i++];
						unsigned char green = partsData[i++];
						unsigned char blue = partsData[i++];
						particles[newIndex].dcolour = COLARGB(alpha, red, green, blue);
					}
					
					// Read vx
					if (fieldDescriptor & 0x80)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading vx");
						particles[newIndex].vx = (partsData[i++]-127.0f)/16.0f;
					}
					
					// Read vy
					if (fieldDescriptor & 0x100)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading vy");
						particles[newIndex].vy = (partsData[i++]-127.0f)/16.0f;
					}

					// Read tmp2
					if (fieldDescriptor & 0x400)
					{
						if (i >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading tmp2");
						particles[newIndex].tmp2 = partsData[i++];
						// Read 2nd byte
						if (fieldDescriptor & 0x800)
						{
							if (i >= partsDataLen)
								throw ParseException("Ran past particle data buffer while loading tmp2");
							particles[newIndex].tmp2 |= (((unsigned)partsData[i++]) << 8);
						}
					}

					// Read tmp3 and tmp4
					if (fieldDescriptor & 0x2000)
					{
						if (i+3 >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading tmp3/tmp4");
						if (fieldDescriptor & 0x10000 && i+7 >= partsDataLen)
							throw ParseException("Ran past particle data buffer while loading high halves of tmp3/tmp4");

						unsigned int tmp34  = (unsigned int)partsData[i + 0];
						tmp34 |= (unsigned int)partsData[i + 1] << 8;
						if (fieldDescriptor & 0x10000)
						{
							tmp34 |= (unsigned int)partsData[i + 4] << 16;
							tmp34 |= (unsigned int)partsData[i + 5] << 24;
						}
						particles[newIndex].tmp3 = int(tmp34);
						tmp34  = (unsigned int)partsData[i + 2];
						tmp34 |= (unsigned int)partsData[i + 3] << 8;
						if (fieldDescriptor & 0x10000)
						{
							tmp34 |= (unsigned int)partsData[i + 6] << 16;
							tmp34 |= (unsigned int)partsData[i + 7] << 24;
						}
						particles[newIndex].tmp4 = int(tmp34);
						i += 4;
						if (fieldDescriptor & 0x10000)
							i += 4;
					}

					if (modCreatedVersion && modCreatedVersion <= 20)
					{
						// Read flags (for instantly activated powered elements in my mod)
						// now removed so that the partsData save format is exactly the same as tpt and won't cause errors
						if (fieldDescriptor & 0x4000)
						{
							if (i >= partsDataLen)
								throw ParseException("Ran past particle data buffer while loading flags");
							particles[newIndex].flags = partsData[i++];
						}
					}

					// No more particle properties to load, so we can change type here without messing up loading
					if (particles[newIndex].type == PT_SOAP)
						// Delete all soap connections, they are regenerated properly using new IDs elsewhere
						particles[newIndex].ctype &= ~6;

					if (createdVersion < 81)
					{
						if (particles[newIndex].type == PT_BOMB && particles[newIndex].tmp != 0)
						{
							particles[newIndex].type = PT_EMBR;
							particles[newIndex].ctype = 0;
							if (particles[newIndex].tmp == 1)
								particles[newIndex].tmp = 0;
						}
						if (particles[newIndex].type == PT_DUST && particles[newIndex].life > 0)
						{
							particles[newIndex].type = PT_EMBR;
							particles[newIndex].ctype = (particles[newIndex].tmp2<<16) | (particles[newIndex].tmp<<8) | particles[newIndex].ctype;
							particles[newIndex].tmp = 1;
						}
						if (particles[newIndex].type == PT_FIRW && particles[newIndex].tmp >= 2)
						{
							int caddress = (int)restrict_flt(restrict_flt((float)(particles[newIndex].tmp-4), 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
							particles[newIndex].type = PT_EMBR;
							particles[newIndex].tmp = 1;
							particles[newIndex].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
						}
					}
					if (createdVersion < 87 && particles[newIndex].type == PT_PSTN && particles[newIndex].ctype)
						particles[newIndex].life = 1;
					if (createdVersion < 89)
					{
						if (particles[newIndex].type == PT_FILT)
						{
							if (particles[newIndex].tmp < 0 || particles[newIndex].tmp > 3)
								particles[newIndex].tmp = 6;
							particles[newIndex].ctype = 0;
						}
						else if (particles[newIndex].type == PT_QRTZ || particles[newIndex].type == PT_PQRT)
						{
							particles[newIndex].tmp2 = particles[newIndex].tmp;
							particles[newIndex].tmp = particles[newIndex].ctype;
							particles[newIndex].ctype = 0;
						}
					}
					if (createdVersion < 90)
					{
						if (particles[newIndex].type == PT_PHOT)
							particles[newIndex].flags |= FLAG_PHOTDECO;
					}
					if (createdVersion < 91)
					{
						if (particles[newIndex].type == PT_VINE)
							particles[newIndex].tmp = 1;
						else if (particles[newIndex].type == PT_PSTN)
							particles[newIndex].temp = 283.15;
						else if (particles[newIndex].type == PT_DLAY)
							particles[newIndex].temp = particles[newIndex].temp - 1.0f;
						else if (particles[newIndex].type == PT_CRAY)
						{
							if (particles[newIndex].tmp2)
							{
								particles[newIndex].ctype |= particles[newIndex].tmp2<<8;
								//particles[newIndex].tmp2 = 0;
							}
						}
						else if (particles[newIndex].type == PT_CONV)
						{
							if (particles[newIndex].tmp)
							{
								particles[newIndex].ctype |= particles[newIndex].tmp<<8;
								// particles[newIndex].tmp = 0;
							}
						}
					}
					if (createdVersion < 93)
					{
						if (particles[newIndex].type == PT_PIPE || particles[newIndex].type == PT_PPIP)
						{
							if (particles[newIndex].ctype == 1)
								particles[newIndex].tmp |= 0x00020000; //PFLAG_INITIALIZING
							particles[newIndex].tmp |= (particles[newIndex].ctype - 1) << 18;
							particles[newIndex].ctype = particles[newIndex].tmp & 0xFF;
						}
						if (particles[newIndex].type == PT_TSNS || particles[newIndex].type == PT_HSWC
						        || particles[newIndex].type == PT_PSNS || particles[newIndex].type == PT_PUMP)
						{
							particles[newIndex].tmp = 0;
						}
					}
					if (createdVersion < 96)
					{
						if (particles[newIndex].type == PT_LIFE && particles[newIndex].ctype >= 0 && particles[newIndex].ctype < NGOL)
						{
							particles[newIndex].tmp2 = particles[newIndex].tmp;
							if (!particles[newIndex].dcolour)
								particles[newIndex].dcolour = builtinGol[particles[newIndex].ctype].color;
							particles[newIndex].tmp = builtinGol[particles[newIndex].ctype].color2;
						}
					}
					if (PressureInTmp3(particles[newIndex].type))
					{
						// pavg[1] used to be saved as a u16, which PressureInTmp3 elements then treated as
						// an i16. tmp3 is now saved as a u32, or as a u16 if it's small enough. PressureInTmp3
						// elements will never use the upper 16 bits, and should still treat the lower 16 bits
						// as an i16, so they need sign extension.
						auto tmp3 = (unsigned int)(particles[newIndex].tmp3);
						if (tmp3 & 0x8000U)
						{
							tmp3 |= 0xFFFF0000U;
							particles[newIndex].tmp3 = int(tmp3);
						}
					}
					if (createdVersion < 100)
					{
						// tmp flags now exist in the spot previously used by PIPE before ver. 93, clear them
						if (particles[newIndex].type == PT_PIPE || particles[newIndex].type == PT_PPIP)
							particles[newIndex].tmp &= ~0xFF;
					}
					// Note: PSv was used in version 77.0 and every version before, add something in PSv too if the element is that old

					newIndex++;
					particlesCount++;
				}
			}
		}
		if (i != partsDataLen)
			throw ParseException("Didn't reach end of particle data buffer");

#ifndef NOMOD
		if (movsData)
		{
			int movsDataPos = 0;
			for (unsigned int i = 0; i < movsDataLen/2; i++)
			{
				MOVSdataItem data = MOVSdataItem(movsData[movsDataPos], movsData[movsDataPos+1]);
				MOVSdata.push_back(data);
				movsDataPos += 2;
			}
		}
		if (animData)
		{
			unsigned int animDataPos = 0;
			for (unsigned i = 0; i < particlesCount; i++)
			{
				if (particles[i].type == PT_ANIM)
				{
					if (animDataPos >= animDataLen)
						break;

					ANIMdataItem data;
					int animLen = animData[animDataPos++];
					data.first = animLen;
					if (animDataPos+4*(animLen+1) > animDataLen)
						throw ParseException("Ran past particle data buffer while loading animation data");

					for (int j = 0; j <= animLen; j++)
					{
						unsigned char alpha = animData[animDataPos++];
						unsigned char red = animData[animDataPos++];
						unsigned char green = animData[animDataPos++];
						unsigned char blue = animData[animDataPos++];
						data.second.push_back(COLARGB(alpha, red, green, blue));
					}
					ANIMdata.push_back(data);
				}
			}
		}
#endif
		if (soapLinkData)
		{
			unsigned int soapLinkDataPos = 0;
			for (unsigned int i = 0; i < particlesCount; i++)
			{
				if (particles[i].type == PT_SOAP)
				{
					// Get the index of the particle forward linked from this one, if present in the save data
					unsigned int linkedIndex = 0;
					if (soapLinkDataPos+3 > soapLinkDataLen) break;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<16;
					linkedIndex |= soapLinkData[soapLinkDataPos++]<<8;
					linkedIndex |= soapLinkData[soapLinkDataPos++];
					// All indexes in soapLinkData have 1 added to them (0 means not saved/loaded)
					if (!linkedIndex || linkedIndex-1 >= particlesCount)
						continue;
					linkedIndex = linkedIndex-1;

					// Attach the two particles
					particles[i].ctype |= 2;
					particles[i].tmp = linkedIndex;
					particles[linkedIndex].ctype |= 4;
					particles[linkedIndex].tmp2 = i;
				}
			}
		}
	}

	if (androidCreatedVersion)
		adminLogMessages.push_back("Made in android build version " + Format::NumberToString<int>(androidCreatedVersion));

	if (modCreatedVersion && !androidCreatedVersion)
		adminLogMessages.push_back("Made in jacob1's mod version " + Format::NumberToString<int>(modCreatedVersion));
}

void Save::ParseSavePSv()
{
	int pos = 0;
	bool new_format = false, legacy_beta = false;

	//New file header uses PSv, replacing fuC. This is to detect if the client uses a new save format for temperatures
	//This creates a problem for old clients, that display and "corrupt" error instead of a "newer version" error

	if (saveSize < 16)
		throw ParseException("No save data");
	if (!(saveData[2] == 0x43 && saveData[1] == 0x75 && saveData[0] == 0x66) && !(saveData[2] == 0x76 && saveData[1] == 0x53 && saveData[0] == 0x50))
		throw ParseException("Unknown format");
	if (saveData[2] == 0x76 && saveData[1] == 0x53 && saveData[0] == 0x50)
		new_format = true;
	int ver = saveData[4];
	if ((ver > SAVE_VERSION && ver < 200) || (ver < 237 && ver > 200+MOD_SAVE_VERSION))
		throw ParseException("Save from a newer version");
	if (ver == 240)
	{
		ver = 65;
		modCreatedVersion = 3;
	}
	else if (ver == 242)
	{
		ver = 66;
		modCreatedVersion = 5;
	}
	else if (ver == 243)
	{
		ver = 68;
		modCreatedVersion = 6;
	}
	else if (ver == 244)
	{
		ver = 69;
		modCreatedVersion = 7;
	}
	else if (ver >= 200) 
	{
		ver = 71;
		modCreatedVersion = 8;
	}
	createdVersion = ver;

	int bw = saveData[6];
	int bh = saveData[7];
	SetSize(bw, bh);

	if (saveData[5] != CELL || bw > XRES/CELL || bh > YRES/CELL)
		throw ParseException("Save too large");
	int size = (unsigned)saveData[8];
	size |= ((unsigned)saveData[9])<<8;
	size |= ((unsigned)saveData[10])<<16;
	size |= ((unsigned)saveData[11])<<24;
	if (size > 209715200 || !size)
		throw ParseException("Save data too large");

	auto dataPtr = std::unique_ptr<unsigned char[]>(new unsigned char[size]);
	unsigned char *data = dataPtr.get();
	if (!data)
		throw ParseException("Cannot allocate memory");

	int bz2ret;
	if ((bz2ret = BZ2_bzBuffToBuffDecompress((char *)data, (unsigned *)&size, (char *)(saveData+12), saveSize-12, 0, 0)) != BZ_OK)
	{
		throw BuildException("Could not compress (ret " + Format::NumberToString<int>(bz2ret) + ")");
	}

	if (size < bw*bh)
		throw ParseException("Save data corrupt (missing data)");

	// normalize coordinates
	int w  = bw *CELL;
	int h  = bh *CELL;

	auto particleIDMapPtr = std::unique_ptr<int[]>(new int[XRES*YRES]);
	int *particleIDMap = particleIDMapPtr.get();
	std::fill(&particleIDMap[0], &particleIDMap[XRES*YRES], 0);
	if (!particleIDMap)
		throw ParseException("Cannot allocate memory");

	if (ver < 34)
	{
		legacyEnable = true;
	}
	else
	{
		if (ver >= 44)
		{
			legacyEnable = saveData[3] & 0x01;
			if (!sys_pause)
			{
				sys_pause = (saveData[3] >> 1) & 0x01;
			}
			if (ver >= 46)
			{
				gravityMode = ((saveData[3] >> 2) & 0x03);// | ((c[3]>>2)&0x01);
				airMode = ((saveData[3] >> 4) & 0x07);// | ((c[3]>>4)&0x02) | ((c[3]>>4)&0x01);
			}
			if (ver >= 49)
			{
				gravityEnable = ((saveData[3] >> 7) & 0x01);
			}
		}
		else
		{
			if (saveData[3] == 1 || saveData[3] == 0)
			{
				legacyEnable = saveData[3];
			}
			else
			{
				legacy_beta = true;
			}
		}
	}

	// load the required air state
	for (int y = 0; y < bh; y++)
		for (int x = 0; x < bw; x++)
		{
			if (data[pos])
			{
				//In old saves, ignore walls created by sign tool bug
				//Not ignoring other invalid walls or invalid walls in new saves, so that any other bugs causing them are easier to notice, find and fix
				if (ver >= 44 && ver < 71 && data[pos] == OLD_WL_SIGN)
				{
					pos++;
					continue;
				}

				//TODO: if wall id's are changed look at https://github.com/simtr/The-Powder-Toy/commit/02a4c17d72def847205c8c89dacabe9ecdcb0dab
				//for now, old saves shouldn't have id's this large
				if (ver < 44 && blockMap[y][x] >= 122)
					blockMap[y][x] = 0;
				blockMap[y][x] = ChangeWall(data[pos]);
				if (ver >= 44)
					blockMap[y][x] = ChangeWallpp(blockMap[y][x]);
				if (blockMap[y][x] < 0 || blockMap[y][x] >= WALLCOUNT)
					blockMap[y][x] = 0;
			}

			pos++;
		}
	for (int y = 0; y < bh; y++)
		for (int x = 0; x < bw; x++)
			if (data[y*bw+x]==4 || (ver>=44 && data[y*bw+x] == O_WL_FAN))
			{
				if (pos >= size)
					throw ParseException("Ran past fanVelX data buffer");
				fanVelX[y][x] = (data[pos++]-127.0f)/64.0f;
			}
	for (int y = 0; y < bh; y++)
		for (int x = 0; x < bw; x++)
			if (data[y*bw+x]==4 || (ver>=44 && data[y*bw+x] == O_WL_FAN))
			{
				if (pos >= size)
					throw ParseException("Ran past fanVelY data buffer");
				fanVelY[y][x] = (data[pos++]-127.0f)/64.0f;
			}

	// load the particle map
	int particlePos = pos;
	int currID = 0;
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
		{
			if (pos >= size)
				throw ParseException("Ran past particle data buffer");
			int type = data[pos++];
			if (type >= PT_NUM)
			{
				type = PT_DUST;
			}
			if (type)
			{
				if (modCreatedVersion > 0 && modCreatedVersion <= 5)
				{
					if (type >= 136 && type <= 140)
						type += (PT_NORMAL_NUM - 136);
					else if (type >= 142 && type <= 146)
						type += (PT_NORMAL_NUM - 137);
					data[pos-1] = type;
				}
				if (currID >= NPART)
				{
					particleIDMap[x+y*w] = NPART+1;
					continue;
				}
				memset(particles+currID, 0, sizeof(particle));
				particles[currID].type = type;
				if (type == PT_COAL)
					particles[currID].tmp = 50;
				if (type == PT_FUSE)
					particles[currID].tmp = 50;
				if (type == PT_PHOT)
					particles[currID].ctype = 0x3fffffff;
				if (type == PT_SOAP)
					particles[currID].ctype = 0;
				if (type==PT_BIZR || type==PT_BIZRG || type==PT_BIZRS)
					particles[currID].ctype = 0x47FFFF;
				particles[currID].x = (float)x;
				particles[currID].y = (float)y;
				particleIDMap[x+y*w] = currID+1;
				particlesCount = ++currID;
			}
		}

	// load particle properties
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			i--;
			if (pos+1 >= size)
				throw ParseException("Ran past velocity data buffer");
			if (i > 0 && i < NPART)
			{
				particles[i].vx = (data[pos++]-127.0f)/16.0f;
				particles[i].vy = (data[pos++]-127.0f)/16.0f;
			}
			else
				pos += 2;
		}
	}
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 44)
			{
				if (pos + 1 >= size)
					throw ParseException("Ran past .life data buffer");
				if (i > 0 && i <= NPART)
				{
					int life = (data[pos++])<<8;
					life |= (data[pos++]);
					particles[i-1].life = life;
				}
				else
					pos+=2;
			}
			else
			{
				if (pos >= size)
					throw ParseException("Ran past .life data buffer");
				if (i > 0 && i <= NPART)
					particles[i-1].life = data[pos++]*4;
				else
					pos++;
			}
		}
	}
	if (ver >= 44)
	{
		for (int j = 0; j < w*h; j++)
		{
			int i = particleIDMap[j];
			if (i)
			{
				if (pos + 1 >= size)
					throw ParseException("Ran past .tmp data buffer");
				if (i > 0 && i <= NPART)
				{
					int tmp = (data[pos++])<<8;
					tmp |= (data[pos++]);
					particles[i-1].tmp = tmp;
					if (ver < 53 && !particles[i-1].tmp)
						for (int q = 0; q < NGOL; q++)
						{
							if (particles[i-1].type == builtinGol[q].oldtype && (builtinGol[q].ruleset >> 17)==0)
								particles[i-1].tmp = (builtinGol[q].ruleset >> 17)+1;
						}
					if (ver>=51 && ver<53 && particles[i-1].type==PT_PBCN)
					{
						particles[i-1].tmp2 = particles[i-1].tmp;
						particles[i-1].tmp = 0;
					}
				}
				else
					pos+=2;
			}
		}
	}
	if (ver >= 53)
	{
		for (int j = 0; j < w*h; j++)
		{
			int i = particleIDMap[j];
			int type = data[particlePos+j];
			if (i && (type == PT_PBCN || (type == PT_TRON && ver >= 77)))
			{
				if (pos >= size)
					throw ParseException("Ran past .tmp2 data buffer");
				if (i > 0 && i <= NPART)
				{
					particles[i-1].tmp2 = data[pos++];
				}
				else
					pos++;
			}
		}
	}
	// Read ALPHA component
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 49)
			{
				if (pos >= size)
					throw ParseException("Ran past alpha deco data buffer");
				if (i > 0 && i <= NPART)
					particles[i-1].dcolour = (data[pos++]<<24);
				else
					pos++;
			}
		}
	}
	// Read RED component
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 49)
			{
				if (pos >= size)
					throw ParseException("Ran past red deco data buffer");
				if (i > 0 && i <= NPART)
					particles[i-1].dcolour |= (data[pos++]<<16);
				else
					pos++;
			}
		}
	}
	// Read GREEN component
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 49)
			{
				if (pos >= size)
					throw ParseException("Ran past green deco data buffer");
				if (i > 0 && i <= NPART)
					particles[i-1].dcolour |= (data[pos++]<<8);
				else
					pos++;
			}
		}
	}
	// Read BLUE component
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 49)
			{
				if (pos >= size)
					throw ParseException("Ran past blue deco data buffer");
				if (i > 0 && i <= NPART)
					particles[i-1].dcolour |= data[pos++];
				else
					pos++;
			}
		}
	}
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		if (i)
		{
			if (ver >= 34 && !legacy_beta)
			{
				if (pos >= size)
					throw ParseException("Ran past .temp data buffer");
				if (i >= 0 && i <= NPART)
				{
					if (ver >= 42)
					{
						if (new_format)
						{
							int temp = (data[pos++])<<8;
							if (pos >= size)
							{
								throw ParseException("Not enough data at line " MTOS(__LINE__) " in " MTOS(__FILE__));
							}
							temp |= (data[pos++]);
							// Fix PUMP saved at 0, so that it loads at 0.
							if (particles[i-1].type == PT_PUMP)
								particles[i-1].temp = temp + 0.15f;
							else
								particles[i-1].temp = (float)temp;
						}
						else
						{
							particles[i-1].temp = (float)(data[pos++] * ((MAX_TEMP-MIN_TEMP)/255)) + MIN_TEMP;
						}
					}
					else
					{
						particles[i-1].temp = ((data[pos++] * ((O_MAX_TEMP-O_MIN_TEMP)/255))+O_MIN_TEMP) + 273.0f;
					}
				}
				else
				{
					pos++;
					if (new_format)
					{
						if (pos >= size)
						{
							throw ParseException("Not enough data at line " MTOS(__LINE__) " in " MTOS(__FILE__));
						}
						pos++;
					}
				}
			}
			else
			{
				legacyHeatSave = true;
			}
		}
	} 
	for (int j = 0; j < w*h; j++)
	{
		int i = particleIDMap[j];
		int type = data[particlePos+j];
		if (i && (type==PT_CLNE || (type==PT_PCLN && ver>=43) || (type==PT_BCLN && ver>=44) || (type==PT_SPRK && ver>=21) || (type==PT_LAVA && ver>=34) || (type==PT_PIPE && ver>=43) || (type==PT_LIFE && ver>=51) || (type==PT_PBCN && ver>=52) || (type==PT_WIRE && ver>=55) || (type==PT_STOR && ver>=59) || (type==PT_CONV && ver>=60)))
		{
			if (pos >= size)
				throw ParseException("Ran past .ctype data buffer");
			if (i > 0 && i <= NPART)
				particles[i-1].ctype = data[pos++];
			else
				pos++;
		}
		// no more particle properties to load, so we can change type here without messing up loading
		if (i > 0 && i <= NPART)
		{
			if (ver < 48 && (type == OLD_PT_WIND || (type == PT_BRAY && particles[i-1].life == 0)))
			{
				// Replace invisible particles with something sensible and add decoration to hide it
				particles[i-1].dcolour = COLARGB(255, 0, 0, 0);
				particles[i-1].type = PT_DMND;
			}
			if (ver < 51 && ((type >= 78 && type <= 89) || (type >= 134 && type <= 146 && type != 141)))
			{
				//Replace old GOL
				particles[i-1].type = PT_LIFE;
				for (int gnum = 0; gnum < NGOL; gnum++)
				{
					if (type == builtinGol[gnum].oldtype)
						particles[i-1].ctype = gnum;
				}
				type = PT_LIFE;
			}
			if (ver < 52 && (type == PT_CLNE || type == PT_PCLN || type == PT_BCLN))
			{
				//Replace old GOL ctypes in clone
				for (int gnum = 0; gnum<NGOL; gnum++)
				{
					if (particles[i-1].ctype == builtinGol[gnum].oldtype)
					{
						particles[i-1].ctype = PT_LIFE;
						particles[i-1].tmp = gnum;
					}
				}
			}
			if (type == PT_LIFE)
			{
				particles[i-1].tmp2 = particles[i-1].tmp;
				particles[i-1].tmp = 0;
				if (particles[i-1].ctype >= 0 && particles[i-1].ctype < NGOL)
				{
					if (!particles[i-1].dcolour)
						particles[i-1].dcolour = builtinGol[particles[i-1].ctype].color;
					particles[i-1].tmp = builtinGol[particles[i-1].ctype].color2;
				}
			}
			if (type == PT_LCRY)
			{
				if (ver < 67)
				{
					//New LCRY uses TMP not life
					if (particles[i-1].life >= 10)
					{
						particles[i-1].life = 10;
						particles[i-1].tmp2 = 10;
						particles[i-1].tmp = 3;
					}
					else if (particles[i-1].life <= 0)
					{
						particles[i-1].life = 0;
						particles[i-1].tmp2 = 0;
						particles[i-1].tmp = 0;
					}
					else if (particles[i-1].life < 10 && particles[i-1].life > 0)
					{
						particles[i-1].tmp = 1;
					}
				}
				else
				{
					particles[i-1].tmp2 = particles[i-1].life;
				}
			}

			// PSv isn't used past version 77, but check version anyway ...
			if (ver<81)
			{
				if (particles[i-1].type == PT_BOMB && particles[i-1].tmp != 0)
				{
					particles[i-1].type = PT_EMBR;
					particles[i-1].ctype = 0;
					if (particles[i-1].tmp == 1)
						particles[i-1].tmp = 0;
				}
				if (particles[i-1].type == PT_DUST && particles[i-1].life > 0)
				{
					particles[i-1].type = PT_EMBR;
					particles[i-1].ctype = (particles[i-1].tmp2<<16) | (particles[i-1].tmp<<8) | particles[i-1].ctype;
					particles[i-1].tmp = 1;
				}
				if (particles[i-1].type == PT_FIRW && particles[i-1].tmp >= 2)
				{
					int caddress = (int)restrict_flt(restrict_flt((float)(particles[i-1].tmp-4), 0.0f, 200.0f)*3, 0.0f, (200.0f*3)-3);
					particles[i-1].type = PT_EMBR;
					particles[i-1].tmp = 1;
					particles[i-1].ctype = (((unsigned char)(firw_data[caddress]))<<16) | (((unsigned char)(firw_data[caddress+1]))<<8) | ((unsigned char)(firw_data[caddress+2]));
				}
			}
			if (ver < 89)
			{
				if (particles[i-1].type == PT_FILT)
				{
					if (particles[i-1].tmp<0 || particles[i-1].tmp>3)
						particles[i-1].tmp = 6;
					particles[i-1].ctype = 0;
				}
				if (particles[i-1].type == PT_QRTZ || particles[i-1].type == PT_PQRT)
				{
					particles[i-1].tmp2 = particles[i-1].tmp;
					particles[i-1].tmp = particles[i-1].ctype;
					particles[i-1].ctype = 0;
				}
			}
			if (ver < 90 && particles[i-1].type == PT_PHOT)
			{
				particles[i-1].flags |= FLAG_PHOTDECO;
			}
			if (ver < 91 && particles[i-1].type == PT_VINE)
			{
				particles[i-1].tmp = 1;
			}
			if (ver < 91 && particles[i-1].type == PT_CONV)
			{
				if (particles[i-1].tmp)
				{
					particles[i-1].ctype |= PMAPID(particles[i-1].tmp);
					particles[i-1].tmp = 0;
				}
			}
			if (ver < 93)
			{
				if (particles[i-1].type == PT_PIPE || particles[i-1].type == PT_PPIP)
				{
					if (particles[i-1].ctype == 1)
						particles[i-1].tmp |= 0x00020000; //PFLAG_INITIALIZING
					particles[i-1].tmp |= (particles[i-1].ctype - 1) << 18;
					particles[i-1].ctype = particles[i-1].tmp & 0xFF;
					particles[i-1].tmp &= ~0xFF;
				}
				if (particles[i-1].type == PT_HSWC || particles[i-1].type == PT_PUMP)
				{
					particles[i-1].tmp = 0;
				}
			}
		}
	}

	if (pos == size) // no sign data, "version 1" PSv
		return;
	int signLen = data[pos++];
	for (int i = 0; i < signLen; i++)
	{
		if (pos+6 > size)
			throw ParseException("Ran past sign data buffer");

		if (signs.size() >= MAXSIGNS)
		{
			pos += 5;
			int size = data[pos++];
			pos += size;
		}
		else
		{
			int x = data[pos++];
			x |= ((unsigned)data[pos++])<<8;

			int y = data[pos++];
			y |= ((unsigned)data[pos++])<<8;

			int ju = data[pos++];
			if (ju < 0 || ju > 3)
				ju = 1;

			int textSize = data[pos++];
			if (pos+textSize > size)
				throw ParseException("Ran past sign data buffer");

			char temp[256];
			memcpy(temp, data+pos, textSize);
			temp[textSize] = 0;
			std::string text = Format::CleanString(temp, true, true, true).substr(0, 45);
			signs.push_back(Sign(text, x, y, (Sign::Justification)ju));
			pos += textSize;
		}
	}

	if (modCreatedVersion >= 3)
	{
		if (pos >= size)
			throw ParseException("Ran past mod settings data buffer");
		decorationsEnable = (data[pos++])&0x01;
		aheatEnable = (data[pos]>>1)&0x01;
		hudEnable = (data[pos]>>2)&0x01;
		hudEnablePresent = true;
		waterEEnabled = (data[pos]>>3)&0x01;
	}
}

#include <iostream>
// restrict the minimum version this save can be opened with
#define RESTRICTVERSION(major, minor) if ((major) > minimumMajorVersion || (((major) == minimumMajorVersion && (minor) > minimumMinorVersion))) {\
	minimumMajorVersion = major;\
	minimumMinorVersion = minor;\
}

void Save::BuildSave()
{
	// minimum version this save is compatible with
	// when building, this number may be increased depending on what elements are used
	// or what properties are detected
	int minimumMajorVersion = 90, minimumMinorVersion = 2;

	//Original size + offset of original corner from snapped corner, rounded up by adding CELL-1
	//int blockW = (orig_w+orig_x0-fullX+CELL-1)/CELL;
	//int blockH = (orig_h+orig_y0-fullY+CELL-1)/CELL;
	int fullW = blockWidth*CELL;
	int fullH = blockHeight*CELL;

	auto wallData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight]);
	unsigned int wallDataLen = blockWidth*blockHeight;
	bool hasWallData = false;
	auto fanData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight*2]);
	auto pressData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight*2]);
	auto vxData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight*2]);
	auto vyData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight*2]);
	auto ambientData = std::unique_ptr<unsigned char[]>(new unsigned char[blockWidth*blockHeight*2]);
	std::fill(&ambientData[0], &ambientData[blockWidth*blockHeight*2], 0);
	if (!wallData || !fanData || !pressData || !vxData || !vyData || !ambientData)
		throw BuildException("Save error, out of memory (blockmaps)");

	auto &possiblyCarriesType = particle::PossiblyCarriesType();
	auto &properties = particle::GetProperties();

	// Copy wall and fan data
	unsigned int fanDataLen = 0, pressDataLen = 0, vxDataLen = 0, vyDataLen = 0, ambientDataLen = 0;
	for (unsigned int x = 0; x < blockWidth; x++)
	{
		for (unsigned int y = 0; y < blockHeight; y++)
		{
			wallData[y*blockWidth+x] = blockMap[y][x];
			if (blockMap[y][x])
				hasWallData = true;

			if (hasPressure)
			{
				// save pressure and x/y velocity grids
				float pres = std::max(-255.0f, std::min(255.0f, pressure[y][x])) + 256.0f;
				float velX = std::max(-255.0f, std::min(255.0f, velocityX[y][x])) + 256.0f;
				float velY = std::max(-255.0f, std::min(255.0f, velocityY[y][x])) + 256.0f;
				pressData[pressDataLen++] = (unsigned char)((int)(pres*128)&0xFF);
				pressData[pressDataLen++] = (unsigned char)((int)(pres*128)>>8);

				vxData[vxDataLen++] = (unsigned char)((int)(velX*128)&0xFF);
				vxData[vxDataLen++] = (unsigned char)((int)(velX*128)>>8);

				vyData[vyDataLen++] = (unsigned char)((int)(velY*128)&0xFF);
				vyData[vyDataLen++] = (unsigned char)((int)(velY*128)>>8);
			}

			if (hasAmbientHeat)
			{
				int tempTemp = (int)(ambientHeat[y][x]+0.5f);
				ambientData[ambientDataLen++] = tempTemp;
				ambientData[ambientDataLen++] = tempTemp >> 8;
			}

			if (blockMap[y][x] == WL_FAN)
			{
				int fanVX = (int)(fanVelX[y][x] * 64.0f + 127.5f);
				if (fanVX < 0)
					fanVX = 0;
				else if (fanVX > 255)
					fanVX = 255;
				fanData[fanDataLen++] = fanVX;
				int fanVY = (int)(fanVelY[y][x] * 64.0f + 127.5f);
				if (fanVY < 0)
					fanVY = 0;
				else if (fanVY > 255)
					fanVY = 255;
				fanData[fanDataLen++] = fanVY;
			}
			else if (blockMap[y][x] == WL_STASIS)
			{
				RESTRICTVERSION(94, 0);
			}
		}
	}

	// Index positions of all particles, using linked lists
	// partsPosFirstMap is pmap for the first particle in each position
	// partsPosLastMap is pmap for the last particle in each position
	// partsPosCount is the number of particles in each position
	// partsPosLink contains, for each particle, (i<<8)|1 of the next particle in the same position
	auto partsPosFirstMap = std::unique_ptr<unsigned[]>(new unsigned[fullW*fullH]);
	auto partsPosLastMap = std::unique_ptr<unsigned[]>(new unsigned[fullW*fullH]);
	auto partsPosCount = std::unique_ptr<unsigned[]>(new unsigned[fullW*fullH]);
	auto partsPosLink = std::unique_ptr<unsigned[]>(new unsigned[NPART]);
	if (!partsPosFirstMap || !partsPosLastMap || !partsPosCount || !partsPosLink)
		throw BuildException("Save error, out of memory  (partmaps)");
	std::fill(&partsPosFirstMap[0], &partsPosFirstMap[fullW*fullH], 0);
	std::fill(&partsPosLastMap[0], &partsPosLastMap[fullW*fullH], 0);
	std::fill(&partsPosCount[0], &partsPosCount[fullW*fullH], 0);
	std::fill(&partsPosLink[0], &partsPosLink[NPART], 0);
	unsigned int soapCount = 0;
	for (unsigned int i = 0; i < particlesCount; i++)
	{
		if (particles[i].type)
		{
			int x = (int)(particles[i].x+0.5f);
			int y = (int)(particles[i].y+0.5f);
			if (!partsPosFirstMap[y*fullW + x])
			{
				// First entry in list
				partsPosFirstMap[y*fullW + x] = (i<<8)|1;
				partsPosLastMap[y*fullW + x] = (i<<8)|1;
			}
			else
			{
				// Add to end of list
				partsPosLink[partsPosLastMap[y*fullW + x]>>8] = (i<<8)|1; // Link to current end of list
				partsPosLastMap[y*fullW + x] = (i<<8)|1; // Set as new end of list
			}
			partsPosCount[y*fullW + x]++;
		}
	}

	// Store number of particles in each position
	auto partsPosData = std::unique_ptr<unsigned char[]>(new unsigned char[fullW*fullH*3]);
	unsigned int partsPosDataLen = 0;
	if (!partsPosData)
		throw BuildException("Save error, out of memory (partposdata)");
	for (int y = 0; y < fullH; y++)
	{
		for (int x = 0; x < fullW; x++)
		{
			unsigned posCount = partsPosCount[y*fullW + x];
			partsPosData[partsPosDataLen++] = (posCount&0x00FF0000)>>16;
			partsPosData[partsPosDataLen++] = (posCount&0x0000FF00)>>8;
			partsPosData[partsPosDataLen++] = (posCount&0x000000FF);
		}
	}

	//Copy parts data
	/* Field descriptor format:
	|      0       |      14       |      13       |      12       |      11       |      10       |       9       |       8       |       7       |       6       |       5       |       4       |       3       |       2       |       1       |       0       |
	|  fieldDesc3  |    type[2]    |  tmp3/4[1+2]  |   tmp[3+4]    |   tmp2[2]     |     tmp2      |   ctype[2]    |      vy       |      vx       |  decorations  |   ctype[1]    |    tmp[2]     |    tmp[1]     |    life[2]    |    life[1]    | .temp dbl len |
	life[2] means a second byte (for a 16 bit field) if life[1] is present
	fieldDesc3 means Field descriptor [3] exists
	   Field descriptor [3] format:
	|      23      |      22       |      21       |      20       |      19       |      18       |      17       |      16       |
	|   RESERVED   |     FREE      |     FREE      |     FREE      |     FREE      |     FREE      |     FREE      |  tmp3/4[3+4]  |
	last bit is reserved. If necessary, use it to signify that fieldDescriptor will have another byte
	That way, if we ever need a 25th bit, we won't have to change the save format
	*/
	// Allocate enough space to store all Particles and 3 bytes on top of that per Particle, for the field descriptors.
	// In practice, a Particle will never need as much space in the save as in memory; this is just an upper bound to simplify allocation.
	auto partsData = std::unique_ptr<unsigned char[]>(new unsigned char[NPART * (sizeof(particle)+3)]);
	unsigned int partsDataLen = 0;
	auto partsSaveIndex = std::unique_ptr<unsigned[]>(new unsigned[NPART]);
	unsigned int partsCount = 0;
	if (!partsData || !partsSaveIndex)
		throw BuildException("Save error, out of memory (partsdata)");
	std::fill(&partsSaveIndex[0], &partsSaveIndex[NPART], 0);
	for (int y = 0; y < fullH; y++)
	{
		for (int x = 0; x < fullW; x++)
		{
			// Find the first particle in this position
			int i = partsPosFirstMap[y*fullW + x];

			// Loop while there is a pmap entry
			while (i)
			{
				unsigned int fieldDesc = 0;
				int tempTemp, vTemp;
				
				// Turn pmap entry into a particles index
				i = i>>8;

				// Store saved particle index+1 for this particles index (0 means not saved)
				partsSaveIndex[i] = (partsCount++) + 1;

				// Type (required)
				partsData[partsDataLen++] = particles[i].type;
				
				// Location of the field descriptor
				int fieldDesc3Loc;
				int fieldDescLoc = partsDataLen++;
				partsDataLen++;

				// tmp3 / tmp4 and higher fieldDesc preparations
				auto tmp3 = (unsigned int)(particles[i].tmp3);
				auto tmp4 = (unsigned int)(particles[i].tmp4);
				if ((tmp3 || tmp4) && (!PressureInTmp3(particles[i].type) || hasPressure))
				{
					fieldDesc |= 1 << 13;
					if (((tmp3 >> 16) || (tmp4 >> 16)) && !PressureInTmp3(particles[i].type))
					{
						fieldDesc |= 1 << 15;
						fieldDesc |= 1 << 16;
						RESTRICTVERSION(97, 0);
					}
				}

				// Extra type byte if necessary
				if (particles[i].type & 0xFF00)
				{
					partsData[partsDataLen++] = particles[i].type >> 8;
					fieldDesc |= 1 << 14;
					RESTRICTVERSION(93, 0);
				}

				// Extra Temperature (2nd byte optional, 1st required), 1 to 2 bytes
				// Store temperature as an offset of 21C(294.15K) or go into a 16byte int and store the whole thing
				if (std::abs(particles[i].temp - 294.15f) < 127.0f)
				{
					tempTemp = (int)floor(particles[i].temp - 294.15f + 0.5f);
					partsData[partsDataLen++] = tempTemp;
				}
				else
				{
					fieldDesc |= 1;
					tempTemp = (int)(particles[i].temp + 0.5f);
					partsData[partsDataLen++] = tempTemp;
					partsData[partsDataLen++] = tempTemp >> 8;
				}

				// Additional fieldDesc byte if necessary
				if (fieldDesc & (1 << 15))
				{
					fieldDesc3Loc = partsDataLen++;
				}

				// Life (optional), 1 to 2 bytes
				if (particles[i].life)
				{
					int life = particles[i].life;
					if (life > 0xFFFF)
						life = 0xFFFF;
					else if (life < 0)
						life = 0;
					fieldDesc |= 1 << 1;
					partsData[partsDataLen++] = life;
					if (life & 0xFF00)
					{
						fieldDesc |= 1 << 2;
						partsData[partsDataLen++] = life >> 8;
					}
				}

				// Tmp (optional), 1, 2 or 4 bytes
				if (particles[i].tmp)
				{
					fieldDesc |= 1 << 3;
					partsData[partsDataLen++] = particles[i].tmp;
					if (particles[i].tmp & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 4;
						partsData[partsDataLen++] = particles[i].tmp >> 8;
						if (particles[i].tmp & 0xFFFF0000)
						{
							fieldDesc |= 1 << 12;
							partsData[partsDataLen++] = (particles[i].tmp&0xFF000000)>>24;
							partsData[partsDataLen++] = (particles[i].tmp&0x00FF0000)>>16;
						}
					}
				}
				
				// Ctype (optional), 1 or 4 bytes
				if (particles[i].ctype)
				{
					fieldDesc |= 1 << 5;
					partsData[partsDataLen++] = particles[i].ctype;
					if (particles[i].ctype & 0xFFFFFF00)
					{
						fieldDesc |= 1 << 9;
						partsData[partsDataLen++] = (particles[i].ctype&0xFF000000)>>24;
						partsData[partsDataLen++] = (particles[i].ctype&0x00FF0000)>>16;
						partsData[partsDataLen++] = (particles[i].ctype&0x0000FF00)>>8;
					}
				}
				
				// Dcolour (optional), 4 bytes
				if (particles[i].dcolour && (COLA(particles[i].dcolour) || particles[i].type == PT_LIFE))
				{
					fieldDesc |= 1 << 6;
					partsData[partsDataLen++] = COLA(particles[i].dcolour);
					partsData[partsDataLen++] = COLR(particles[i].dcolour);
					partsData[partsDataLen++] = COLG(particles[i].dcolour);
					partsData[partsDataLen++] = COLB(particles[i].dcolour);
				}
				
				// VX (optional), 1 byte
				if (std::abs(particles[i].vx) > 0.001f)
				{
					fieldDesc |= 1 << 7;
					vTemp = (int)(particles[i].vx*16.0f+127.5f);
					if (vTemp < 0)
						vTemp = 0;
					else if (vTemp > 255)
						vTemp = 255;
					partsData[partsDataLen++] = vTemp;
				}
				
				// VY (optional), 1 byte
				if (std::abs(particles[i].vy) > 0.001f)
				{
					fieldDesc |= 1 << 8;
					vTemp = (int)(particles[i].vy*16.0f+127.5f);
					if (vTemp<0)
						vTemp=0;
					if (vTemp>255)
						vTemp=255;
					partsData[partsDataLen++] = vTemp;
				}

				// Tmp2 (optional), 1 or 2 bytes
#ifndef NOMOD
				if (particles[i].tmp2 && particles[i].type != PT_PINV)
#else
				if (particles[i].tmp2)
#endif
				{
					fieldDesc |= 1 << 10;
					partsData[partsDataLen++] = particles[i].tmp2;
					if (particles[i].tmp2 & 0xFF00)
					{
						fieldDesc |= 1 << 11;
						partsData[partsDataLen++] = particles[i].tmp2 >> 8;
					}
				}

				//tmp3 and tmp4, 4 bytes
				if (fieldDesc & (1 << 13))
				{
					partsData[partsDataLen++] = tmp3;
					partsData[partsDataLen++] = tmp3 >> 8;
					partsData[partsDataLen++] = tmp4;
					partsData[partsDataLen++] = tmp4 >> 8;
					if (fieldDesc & (1 << 16))
					{
						partsData[partsDataLen++] = tmp3 >> 16;
						partsData[partsDataLen++] = tmp3 >> 24;
						partsData[partsDataLen++] = tmp4 >> 16;
						partsData[partsDataLen++] = tmp4 >> 24;
					}
				}
				
				// Write the field descriptor
				partsData[fieldDescLoc] = fieldDesc&0xFF;
				partsData[fieldDescLoc+1] = fieldDesc>>8;
				if (fieldDesc & (1 << 15))
				{
					partsData[fieldDesc3Loc] = fieldDesc>>16;
				}

				if (particles[i].type == PT_SOAP)
					soapCount++;

				if (particles[i].type == PT_RPEL && particles[i].ctype)
				{
					RESTRICTVERSION(91, 4);
				}
				else if (particles[i].type == PT_NWHL && particles[i].tmp)
				{
					RESTRICTVERSION(91, 5);
				}
				if (particles[i].type == PT_HEAC || particles[i].type == PT_SAWD || particles[i].type == PT_POLO
						|| particles[i].type == PT_RFRG || particles[i].type == PT_RFGL || particles[i].type == PT_LSNS)
				{
					RESTRICTVERSION(92, 0);
				}
				else if ((particles[i].type == PT_FRAY || particles[i].type == PT_INVIS) && particles[i].tmp)
				{
					RESTRICTVERSION(92, 0);
				}
				else if (particles[i].type == PT_PIPE || particles[i].type == PT_PPIP)
				{
					RESTRICTVERSION(93, 0);
				}
				else if (particles[i].type == PT_TSNS || particles[i].type == PT_PSNS
				         || particles[i].type == PT_HSWC || particles[i].type == PT_PUMP)
				{
					if (particles[i].tmp == 1)
					{
						RESTRICTVERSION(93, 0);
					}
				}
				if (PMAPBITS > 8 && sim != nullptr)
				{
					for (auto index : possiblyCarriesType)
					{
						if (sim->elements[particles[i].type].CarriesTypeIn & (1U << index))
						{
							auto *prop = reinterpret_cast<const int *>(reinterpret_cast<const char *>(&particles[i]) + properties[index].Offset);
							if (TYP(*prop) > 0xFF)
							{
								RESTRICTVERSION(93, 0);
							}
						}
					}
				}
				if (particles[i].type == PT_LDTC)
				{
					RESTRICTVERSION(94, 0);
				}
				if (particles[i].type == PT_TSNS || particles[i].type == PT_PSNS)
				{
					if (particles[i].tmp == 2)
					{
						RESTRICTVERSION(94, 0);
					}
				}
				if (particles[i].type == PT_LSNS)
				{
					if (particles[i].tmp >= 1 || particles[i].tmp <= 3)
					{
						RESTRICTVERSION(95, 0);
					}
				}
				if (particles[i].type == PT_LIFE && (particles[i].ctype < 0 || particles[i].ctype >= NGOL))
				{
					RESTRICTVERSION(96, 0);
				}
				if (particles[i].type == PT_GLAS && particles[i].life > 0)
				{
					RESTRICTVERSION(97, 0);
				}
				if (PressureInTmp3(particles[i].type))
				{
					RESTRICTVERSION(97, 0);
				}
				if (particles[i].type == PT_CONV && particles[i].tmp2 != 0)
				{
					RESTRICTVERSION(97, 0);
				}
				if (particles[i].type == PT_RSST || particles[i].type == PT_RSSS)
				{
					RESTRICTVERSION(98, 0);
				}
				if (particles[i].type == PT_ETRD && (particles[i].tmp || particles[i].tmp2))
				{
					RESTRICTVERSION(98, 0);
				}
				if (particles[i].type == PT_BASE || particles[i].type == PT_SEED)
				{
					RESTRICTVERSION(100, 0);
				}
				if ((particles[i].type == PT_PIPE || particles[i].type == PT_PPIP) && (particles[i].tmp & PFLAG_CAN_CONDUCT))
				{
					RESTRICTVERSION(100, 0);
				}

				// Get the pmap entry for the next particle in the same position
				i = partsPosLink[i];
			}
		}
	}

#ifndef NOMOD
	unsigned char *movsData = NULL, *animData = NULL;
	unsigned int movsDataLen = 0, animDataLen = 0;
	auto movsDataPtr = std::unique_ptr<unsigned char[]>(), animDataPtr = std::unique_ptr<unsigned char[]>();

	if (MOVSdata.size())
	{
		movsData = new unsigned char[MOVSdata.size()*2];
		if (!movsData)
			throw BuildException("Save error, out of memory (BALL)");
		movsDataPtr = std::move(std::unique_ptr<unsigned char[]>(movsData));
		for (MOVSdataItem movs : MOVSdata)
		{
			movsData[movsDataLen++] = movs.first;
			movsData[movsDataLen++] = movs.second;
		}
	}

	if (ANIMdata.size())
	{
		int ANIMsize = 0;
		for (ANIMdataItem anim : ANIMdata)
		{
			ANIMsize += (anim.first+1)*4+1;
		}

		animData = new unsigned char[ANIMsize];
		if (!animData)
			throw BuildException("Save error, out of memory (ANIM)");
		animDataPtr = std::move(std::unique_ptr<unsigned char[]>(animData));
		
		for (ANIMdataItem anim : ANIMdata)
		{
			animData[animDataLen++] = anim.first;
			for (ARGBColour color : anim.second)
			{
				animData[animDataLen++] = COLA(color);
				animData[animDataLen++] = COLR(color);
				animData[animDataLen++] = COLG(color);
				animData[animDataLen++] = COLB(color);
			}
		}
	}
#endif

	unsigned char *soapLinkData = NULL;
	auto soapLinkDataPtr = std::unique_ptr<unsigned char[]>();
	unsigned int soapLinkDataLen = 0;
	if (soapCount)
	{
		soapLinkData = new unsigned char[3*soapCount];
		if (!soapLinkData)
			throw BuildException("Save error, out of memory (SOAP)");
		soapLinkDataPtr = std::move(std::unique_ptr<unsigned char[]>(soapLinkData));
		
		// Iterate through particles in the same order that they were saved
		for (int y = 0; y < fullH; y++)
		{
			for (int x = 0; x < fullW; x++)
			{
				// Find the first particle in this position
				int i = partsPosFirstMap[y*fullW + x];
	
				// Loop while there is a pmap entry
				while (i)
				{
					// Turn pmap entry into a particles index
					i = i>>8;
	
					if (particles[i].type == PT_SOAP)
					{
						// Only save forward link for each particle, back links can be deduced from other forward links
						// linkedIndex is index within saved particles + 1, 0 means not saved or no link
						unsigned linkedIndex = 0;
						if ((particles[i].ctype&2) && particles[i].tmp>=0 && particles[i].tmp<NPART)
						{
							linkedIndex = partsSaveIndex[particles[i].tmp];
						}
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0xFF0000)>>16;
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0x00FF00)>>8;
						soapLinkData[soapLinkDataLen++] = (linkedIndex&0x0000FF);
					}
	
					// Get the pmap entry for the next particle in the same position
					i = partsPosLink[i];
				}
			}
		}
	}

	for (size_t i = 0; i < signs.size(); i++)
	{
		Point pos = signs[i].GetRealPos();
		if (signs[i].GetText().length() && pos.X >= 0 && pos.X <= fullW && pos.Y >= 0 && pos.Y <= fullH)
		{
			bool v95 = false;
			signs[i].GetDisplayText(nullptr, &v95);
			if (v95)
				RESTRICTVERSION(95, 0);
		}
	}

	bson b;
	b.data = NULL;
	auto bson_deleter = [](bson * b) { bson_destroy(b); };
	// Use unique_ptr with a custom deleter to ensure that bson_destroy is called even when an exception is thrown
	std::unique_ptr<bson, decltype(bson_deleter)> b_ptr(&b, bson_deleter);

	set_bson_err_handler([](const char* err) { throw BuildException("BSON error when building save: " + std::string(err)); });
	bson_init(&b);
	bson_append_start_object(&b, "origin");
	bson_append_int(&b, "majorVersion", SAVE_VERSION);
	bson_append_int(&b, "minorVersion", MINOR_VERSION);
	bson_append_int(&b, "buildNum", BUILD_NUM);
	bson_append_int(&b, "snapshotId", 0);
#ifdef ANDROID
	bson_append_int(&b, "mobileMajorVersion", MOBILE_MAJOR);
	bson_append_int(&b, "mobileMinorVersion", MOBILE_MINOR);
	bson_append_int(&b, "mobileBuildVersion", MOBILE_BUILD);
#endif
	bson_append_string(&b, "releaseType", IDENT_RELTYPE);
	bson_append_string(&b, "platform", IDENT_PLATFORM);
	bson_append_string(&b, "builtType", IDENT_BUILD);
	bson_append_finish_object(&b);
	bson_append_start_object(&b, "minimumVersion");
	bson_append_int(&b, "major", minimumMajorVersion);
	bson_append_int(&b, "minor", minimumMinorVersion);
	bson_append_finish_object(&b);

	bson_append_bool(&b, "waterEEnabled", waterEEnabled);
	bson_append_bool(&b, "legacyEnable", legacyEnable);
	bson_append_bool(&b, "gravityEnable", gravityEnable);
	bson_append_bool(&b, "paused", paused);
	bson_append_int(&b, "gravityMode", gravityMode);
	bson_append_int(&b, "airMode", airMode);
	if (fabsf(ambientAirTemp - (R_TEMP + 273.15f)) > 0.0001f)
	{
		bson_append_double(&b, "ambientAirTemp", ambientAirTemp);
		RESTRICTVERSION(96, 0);
	}
	if (std::fabs(edgePressure) > 0.0001f)
	{
		bson_append_double(&b, "edgePressure", edgePressure);
		// TODO: uncomment
		//RESTRICTVERSION(100, 0);
	}
	if (std::fabs(edgeVelocityX) > 0.0001f)
	{
		bson_append_double(&b, "edgeVelocityX", edgeVelocityX);
		// TODO: uncomment
		//RESTRICTVERSION(100, 0);
	}
	if (std::fabs(edgeVelocityY) > 0.0001f)
	{
		bson_append_double(&b, "edgeVelocityY", edgeVelocityY);
		// TODO: uncomment
		//RESTRICTVERSION(100, 0);
	}
	if (vorticityCoeff > 0.0001f && vorticityCoeff < 1.0f)
	{
		bson_append_double(&b, "vorticityCoeff", double(vorticityCoeff));
		// TODO: uncomment
		//RESTRICTVERSION(100, 0);
	}
	bson_append_int(&b, "convectionMode", convectionMode);
#ifndef NOMOD
	bson_append_bool(&b, "msrotation", msRotation);
#endif
	if (decorationsEnablePresent)
		bson_append_bool(&b, "decorations_enable", decorationsEnable);
	if (hudEnablePresent)
		bson_append_bool(&b, "hud_enable", hudEnable);
	bson_append_bool(&b, "aheat_enable", aheatEnable);
	bson_append_int(&b, "edgeMode", edgeMode);
	if (gravityMode == GRAV_CUSTOM)
	{
		bson_append_double(&b, "customGravityX", double(customGravityX));
		bson_append_double(&b, "customGravityY", double(customGravityY));
		RESTRICTVERSION(97, 0);
	}

	if (stkm.hasData())
	{
		bson_append_start_object(&b, "stkm");
		if (stkm.rocketBoots1)
			bson_append_bool(&b, "rocketBoots1", stkm.rocketBoots1);
		if (stkm.rocketBoots2)
			bson_append_bool(&b, "rocketBoots2", stkm.rocketBoots2);
		if (stkm.fan1)
			bson_append_bool(&b, "fan1", stkm.fan1);
		if (stkm.fan2)
			bson_append_bool(&b, "fan2", stkm.fan2);
		if (stkm.rocketBootsFigh.size())
		{
			bson_append_start_array(&b, "rocketBootsFigh");
			for (unsigned int fighNum : stkm.rocketBootsFigh)
				bson_append_int(&b, "num", fighNum);
			bson_append_finish_array(&b);
		}
		if (stkm.fanFigh.size())
		{
			bson_append_start_array(&b, "fanFigh");
			for (unsigned int fighNum : stkm.fanFigh)
				bson_append_int(&b, "num", fighNum);
			bson_append_finish_array(&b);
		}
		bson_append_finish_object(&b);
	}

	// Render modes (Jacob1's mod)
	if (renderModePresent)
		bson_append_int(&b, "render_mode", renderMode);

	// Display modes (Jacob1's mod)
	if (displayModePresent)
		bson_append_int(&b, "display_mode", displayMode);

	// other Jacob1's mod stuff
	if (colorModePresent)
		bson_append_int(&b, "color_mode", colorMode);
	bson_append_int(&b, "Jacob1's_Mod", MOD_SAVE_VERSION);
	bson_append_string(&b, "leftSelectedElementIdentifier", leftSelectedIdentifier.c_str());
	bson_append_string(&b, "rightSelectedElementIdentifier", rightSelectedIdentifier.c_str());
	if (activeMenuPresent)
		bson_append_int(&b, "activeMenu", activeMenu);

	bson_append_int(&b, "pmapbits", pmapbits);
	if (partsData && partsDataLen)
	{
		bson_append_binary(&b, "parts", (char)BSON_BIN_USER, (const char*)partsData.get(), partsDataLen);

		bson_append_start_array(&b, "palette");
		for (std::vector<PaletteItem>::iterator iter = palette.begin(), end = palette.end(); iter != end; ++iter)
		{
			bson_append_int(&b, (*iter).first.c_str(), (*iter).second);
		}
		bson_append_finish_array(&b);

		if (partsPosData && partsPosDataLen)
			bson_append_binary(&b, "partsPos", (char)BSON_BIN_USER, (const char*)partsPosData.get(), partsPosDataLen);
	}
	if (wallData && hasWallData)
		bson_append_binary(&b, "wallMap", (char)BSON_BIN_USER, (const char*)wallData.get(), wallDataLen);
	if (fanData && fanDataLen)
		bson_append_binary(&b, "fanMap", (char)BSON_BIN_USER, (const char*)fanData.get(), fanDataLen);
	if (pressData && hasPressure && pressDataLen)
		bson_append_binary(&b, "pressMap", (char)BSON_BIN_USER, (const char*)pressData.get(), pressDataLen);
	if (vxData && hasPressure && vxDataLen)
		bson_append_binary(&b, "vxMap", (char)BSON_BIN_USER, (const char*)vxData.get(), vxDataLen);
	if (vyData && hasPressure && vyDataLen)
		bson_append_binary(&b, "vyMap", (char)BSON_BIN_USER, (const char*)vyData.get(), vyDataLen);
	if (ambientData && hasAmbientHeat && ambientDataLen)
		bson_append_binary(&b, "ambientMap", (char)BSON_BIN_USER, (const char*)ambientData.get(), ambientDataLen);
	if (soapLinkData && soapLinkDataLen)
		bson_append_binary(&b, "soapLinks", (char)BSON_BIN_USER, (const char*)soapLinkData, soapLinkDataLen);
#ifndef NOMOD
	if (movsData && movsDataLen)
		bson_append_binary(&b, "movs", (char)BSON_BIN_USER, (const char*)movsData, movsDataLen);
	if (animData && animDataLen)
		bson_append_binary(&b, "anim", (char)BSON_BIN_USER, (const char*)animData, animDataLen);
#endif
#ifdef LUACONSOLE
	if (luaCode.length())
	{
		bson_append_binary(&b, "LuaCode", (char)BSON_BIN_USER, luaCode.c_str(), luaCode.length());
	}
#endif
	if (signs.size())
	{
		bson_append_start_array(&b, "signs");
		for (std::vector<Sign>::iterator iter = signs.begin(), end = signs.end(); iter != end; ++iter)
		{
			Sign sign = (*iter);
			bson_append_start_object(&b, "sign");
			bson_append_string(&b, "text", sign.GetText().c_str());
			bson_append_int(&b, "justification", (int)sign.GetJustification());
			bson_append_int(&b, "x", sign.GetRealPos().X);
			bson_append_int(&b, "y", sign.GetRealPos().Y);
			bson_append_finish_object(&b);
		}
		bson_append_finish_array(&b);
	}

	if (saveInfoPresent)
	{
		bson_append_start_object(&b, "saveInfo");
		bson_append_int(&b, "saveOpened", saveInfo.GetSaveOpened());
		bson_append_int(&b, "fileOpened", saveInfo.GetFileOpened());
		bson_append_string(&b, "saveName", saveInfo.GetSaveName().c_str());
		bson_append_string(&b, "fileName", saveInfo.GetFileName().c_str());
		bson_append_int(&b, "published", saveInfo.GetPublished());
		bson_append_string(&b, "ID", Format::NumberToString<int>(saveInfo.GetSaveID()).c_str());
		bson_append_string(&b, "version", saveInfo.GetSaveVersion().c_str());
		bson_append_string(&b, "description", saveInfo.GetDescription().c_str());
		bson_append_string(&b, "author", saveInfo.GetAuthor().c_str());
		bson_append_string(&b, "tags", saveInfo.GetTags().c_str());
		bson_append_int(&b, "myVote", saveInfo.GetMyVote());
		bson_append_finish_object(&b);
	}

	if (authors.size())
	{
		bson_append_start_object(&b, "authors");
		ConvertJsonToBson(&b, authors);
		bson_append_finish_object(&b);
	}
	if (bson_finish(&b) == BSON_ERROR)
		throw BuildException("Error building bson data");
	//bson_print(&b);

	// Mark save as incompatible with latest release
	if (minimumMajorVersion > SAVE_VERSION || (minimumMajorVersion == SAVE_VERSION && minimumMinorVersion > MINOR_VERSION))
		fromNewerVersion = true;

	unsigned char *finalData = (unsigned char*)bson_data(&b);
	unsigned int finalDataLen = bson_size(&b);
	auto outputData = std::unique_ptr<unsigned char[]>(new unsigned char[finalDataLen*2+12]);
	if (!outputData)
		throw BuildException("Save error, out of memory (finalData): " + Format::NumberToString<unsigned int>(finalDataLen*2+12));

	outputData[0] = 'O';
	outputData[1] = 'P';
	outputData[2] = 'S';
	outputData[3] = '1';
	outputData[4] = SAVE_VERSION;
	outputData[5] = CELL;
	outputData[6] = blockWidth;
	outputData[7] = blockHeight;
	outputData[8] = finalDataLen;
	outputData[9] = finalDataLen >> 8;
	outputData[10] = finalDataLen >> 16;
	outputData[11] = finalDataLen >> 24;

	unsigned int compressedSize = finalDataLen*2, bz2ret;
	if ((bz2ret = BZ2_bzBuffToBuffCompress((char*)(outputData.get()+12), &compressedSize, (char*)finalData, bson_size(&b), 9, 0, 0)) != BZ_OK)
	{
		throw BuildException("Save error, could not compress (ret " + Format::NumberToString<int>(bz2ret) + ")");
	}

	saveSize = compressedSize + 12;
	saveData = new unsigned char[saveSize];
	std::copy(&outputData[0], &outputData[saveSize], &saveData[0]);
}

vector2d Save::Translate(vector2d translate)
{
	try
	{
		if (!expanded)
			ParseSave();
	}
	catch (ParseException& e)
	{
		return Matrix::v2d_zero;
	}

	vector2d translateReal = translate;
	float minx = 0, miny = 0, maxx = 0, maxy = 0;
	// determine minimum and maximum position of all particles / signs
	for (const Sign &sign: signs)
	{
		vector2d pos = v2d_new(sign.GetRealPos().X, sign.GetRealPos().Y);
		pos = v2d_add(pos, translate);
		int nx = std::floor(pos.x + 0.5f);
		int ny = std::floor(pos.y + 0.5f);
		if (nx < minx)
			minx = nx;
		else if (nx > maxx)
			maxx = nx;
		if (ny < miny)
			miny = ny;
		else if (ny > maxy)
			maxy = ny;
	}
	for (unsigned int i = 0; i < particlesCount; i++)
	{
		if (!particles[i].type)
			continue;
		vector2d pos = v2d_new(particles[i].x, particles[i].y);
		pos = v2d_add(pos, translate);
		int nx = std::floor(pos.x + 0.5f);
		int ny = std::floor(pos.y + 0.5f);
		if (nx < minx)
			minx = nx;
		else if (nx > maxx)
			maxx = nx;
		if (ny < miny)
			miny = ny;
		else if (ny > maxy)
			maxy = ny;
	}
	// determine whether corrections are needed. If moving in this direction would delete stuff, expand the save
	vector2d backCorrection = v2d_new(
		(minx < 0) ? (-floor(minx / CELL)) : 0,
		(miny < 0) ? (-floor(miny / CELL)) : 0
	);
	unsigned int blockBoundsX = int(maxx / CELL) + 1, blockBoundsY = int(maxy / CELL) + 1;
	vector2d frontCorrection = v2d_new(
		(blockBoundsX > blockWidth) ? (blockBoundsX - blockWidth) : 0,
		(blockBoundsY > blockHeight) ? (blockBoundsY - blockHeight) : 0
	);

	// get new width based on corrections
	int newWidth = (blockWidth + backCorrection.x + frontCorrection.x) * CELL;
	int newHeight = (blockHeight + backCorrection.y + frontCorrection.y) * CELL;
	if (newWidth > XRES)
		frontCorrection.x = backCorrection.x = 0;
	if (newHeight > YRES)
		frontCorrection.y = backCorrection.y = 0;

	// call Transform to do the transformation we wanted when calling this function
	translate = v2d_add(translate, v2d_multiply_float(backCorrection, CELL));
	Transform(m2d_identity, translate, translateReal,
	    (blockWidth + backCorrection.x + frontCorrection.x) * CELL,
	    (blockHeight + backCorrection.y + frontCorrection.y) * CELL
	);

	// return how much we corrected. This is used to offset the position of the current stamp
	// otherwise it would attempt to recenter it with the current size
	return v2d_add(v2d_multiply_float(backCorrection, -CELL), v2d_multiply_float(frontCorrection, CELL));
}

void Save::Transform(matrix2d transform, vector2d translate)
{
	try
	{
		if (!expanded)
			ParseSave();
	}
	catch (ParseException& e)
	{
		return;
	}

	int width = blockWidth*CELL, height = blockHeight*CELL, newWidth, newHeight;
	// top left, bottom right corner
	vector2d ctl, cbr;
	vector2d cornerso[4];
	vector2d translateReal = translate;
	// undo any translation caused by rotation
	cornerso[0] = v2d_new(0, 0);
	cornerso[1] = v2d_new(width - 1, 0);
	cornerso[2] = v2d_new(0, height - 1);
	cornerso[3] = v2d_new(width - 1, height - 1);
	for (int i = 0; i < 4; i++)
	{
		vector2d tmp = m2d_multiply_v2d(transform, cornerso[i]);
		if (i == 0)
			ctl = cbr = tmp;
		if (tmp.x < ctl.x)
			ctl.x = tmp.x;
		else if (tmp.x > cbr.x)
			cbr.x = tmp.x;
		if (tmp.y < ctl.y)
			ctl.y = tmp.y;
		else if (tmp.y > cbr.y)
			cbr.y = tmp.y;
	}
	// casting as int doesn't quite do what we want with negative numbers, so use floor()
	vector2d tmp = v2d_new(floor(ctl.x+0.5f),floor(ctl.y+0.5f));
	translate = v2d_sub(translate,tmp);
	newWidth = std::floor(cbr.x+0.5f)-floor(ctl.x+0.5f)+1;
	newHeight = std::floor(cbr.y+0.5f)-floor(ctl.y+0.5f)+1;
	Transform(transform, translate, translateReal, newWidth, newHeight);
}

// transform is a matrix describing how we want to rotate this save
// translate can vary depending on whether the save is bring rotated, or if a normal translate caused it to expand
// translateReal is the original amount we tried to translate, used to calculate wall shifting
void Save::Transform(matrix2d transform, vector2d translate, vector2d translateReal, int newWidth, int newHeight)
{
	try
	{
		if (!expanded)
			ParseSave();
	}
	catch (ParseException& e)
	{
		return;
	}

	if (newWidth > XRES)
		newWidth = XRES;
	if (newHeight > YRES)
		newHeight = YRES;

	int newBlockWidth = newWidth / CELL, newBlockHeight = newHeight / CELL;

	unsigned char **blockMapNew = Allocate2DArray<unsigned char>(newBlockWidth, newBlockHeight, 0);
	float **fanVelXNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);
	float **fanVelYNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);
	float **pressureNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);
	float **velocityXNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);
	float **velocityYNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);
	float **ambientHeatNew = Allocate2DArray<float>(newBlockWidth, newBlockHeight, 0.0f);

	// Match these up with the matrices provided in GameView::OnKeyPress.
	bool patchPipeR = transform.a ==  0 && transform.b ==  1 && transform.c == -1 && transform.d ==  0;
	bool patchPipeH = transform.a == -1 && transform.b ==  0 && transform.c ==  0 && transform.d ==  1;
	bool patchPipeV = transform.a ==  1 && transform.b ==  0 && transform.c ==  0 && transform.d == -1;

	// rotate and translate signs, parts, walls
	for (size_t i = 0; i < signs.size(); i++)
	{
		vector2d pos = v2d_new(signs[i].GetRealPos().X, signs[i].GetRealPos().Y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos), translate);
		int nx = std::floor(pos.x + 0.5f);
		int ny = std::floor(pos.y + 0.5f);
		if (nx < 0 || nx >= newWidth || ny < 0 || ny >= newHeight)
		{
			signs[i].SetText("");
			continue;
		}
		signs[i].SetPos(Point(nx, ny));
	}
	for (unsigned int i = 0; i < particlesCount; i++)
	{
		if (!particles[i].type)
			continue;
		vector2d pos = v2d_new(particles[i].x, particles[i].y);
		pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
		int nx = std::floor(pos.x + 0.5f);
		int ny = std::floor(pos.y + 0.5f);
		if (nx<0 || nx>=newWidth || ny<0 || ny>=newHeight)
		{
			particles[i].type = PT_NONE;
			continue;
		}
		particles[i].x = nx;
		particles[i].y = ny;
		vector2d vel = v2d_new(particles[i].vx, particles[i].vy);
		vel = m2d_multiply_v2d(transform, vel);
		particles[i].vx = vel.x;
		particles[i].vy = vel.y;
		if (particles[i].type == PT_PIPE || particles[i].type == PT_PPIP)
		{
			if (patchPipeR)
				PIPE_patchR(particles[i]);
			if (patchPipeH)
				PIPE_patchH(particles[i]);
			if (patchPipeV)
				PIPE_patchV(particles[i]);
		}
	}

	// translate walls and other grid items when the stamp is shifted more than 4 pixels in any direction
	int translateX = 0, translateY = 0;
	if (translateReal.x > 0 && ((int)translated.x%CELL == 3
	                        || (translated.x < 0 && (int)translated.x%CELL == 0)))
		translateX = CELL;
	else if (translateReal.x < 0 && ((int)translated.x%CELL == -3
	                             || (translated.x > 0 && (int)translated.x%CELL == 0)))
		translateX = -CELL;
	if (translateReal.y > 0 && ((int)translated.y%CELL == 3
	                        || (translated.y < 0 && (int)translated.y%CELL == 0)))
		translateY = CELL;
	else if (translateReal.y < 0 && ((int)translated.y%CELL == -3
	                             || (translated.y > 0 && (int)translated.y%CELL == 0)))
		translateY = -CELL;

	for (unsigned int y = 0; y < blockHeight; y++)
		for (unsigned int x = 0; x < blockWidth; x++)
		{
			vector2d pos = v2d_new(x*CELL+CELL*0.4f+translateX, y*CELL+CELL*0.4f+translateY);
			pos = v2d_add(m2d_multiply_v2d(transform,pos),translate);
			int nx = pos.x/CELL;
			int ny = pos.y/CELL;
			if (pos.x<0 || nx>=newBlockWidth || pos.y<0 || ny>=newBlockHeight)
				continue;
			if (blockMap[y][x])
			{
				blockMapNew[ny][nx] = blockMap[y][x];
				if (blockMap[y][x] == WL_FAN)
				{
					vector2d vel = v2d_new(fanVelX[y][x], fanVelY[y][x]);
					vel = m2d_multiply_v2d(transform, vel);
					fanVelXNew[ny][nx] = vel.x;
					fanVelYNew[ny][nx] = vel.y;
				}
			}
			pressureNew[ny][nx] = pressure[y][x];
			{
				vector2d vel = v2d_new(velocityX[y][x], velocityY[y][x]);
				vel = m2d_multiply_v2d(transform, vel);
				velocityXNew[ny][nx] = vel.x;
				velocityYNew[ny][nx] = vel.y;
			}
			velocityXNew[ny][nx] = velocityX[y][x];
			velocityYNew[ny][nx] = velocityY[y][x];
			ambientHeatNew[ny][nx] = ambientHeat[y][x];
		}
	translated = v2d_add(m2d_multiply_v2d(transform, translated), translateReal);

	for (unsigned int j = 0; j < blockHeight; j++)
	{
		delete[] blockMap[j];
		delete[] fanVelX[j];
		delete[] fanVelY[j];
		delete[] pressure[j];
		delete[] velocityX[j];
		delete[] velocityY[j];
		delete[] ambientHeat[j];
	}

	blockWidth = newBlockWidth;
	blockHeight = newBlockHeight;

	delete[] blockMap;
	delete[] fanVelX;
	delete[] fanVelY;
	delete[] pressure;
	delete[] velocityX;
	delete[] velocityY;
	delete[] ambientHeat;

	blockMap = blockMapNew;
	fanVelX = fanVelXNew;
	fanVelY = fanVelYNew;
	pressure = pressureNew;
	velocityX = velocityXNew;
	velocityY = velocityYNew;
	ambientHeat = ambientHeatNew;

	// invalidate save data
	delete[] saveData;
	saveData = NULL;
}

template <typename T>
T ** Save::Allocate2DArray(int blockWidth, int blockHeight, T defaultVal)
{
	T ** temp = new T*[blockHeight];
	for (int y = 0; y < blockHeight; y++)
	{
		temp[y] = new T[blockWidth];
		std::fill(&temp[y][0], &temp[y][0]+blockWidth, defaultVal);
	}
	return temp;
}

// deallocates a pointer to a 2D array and sets it to NULL
template <typename T>
void Save::Deallocate2DArray(T ***array, int blockHeight)
{
	if (*array)
	{
		for (int y = 0; y < blockHeight; y++)
			delete[] (*array)[y];
		delete[] (*array);
		*array = NULL;
	}
}

void Save::ConvertBsonToJson(bson_iterator *iter, Json::Value *j, int depth)
{
	bson_iterator subiter;
	bson_iterator_subiterator(iter, &subiter);
	while (bson_iterator_next(&subiter))
	{
		std::string key = bson_iterator_key(&subiter);
		if (bson_iterator_type(&subiter) == BSON_STRING)
			(*j)[key] = bson_iterator_string(&subiter);
		else if (bson_iterator_type(&subiter) == BSON_BOOL)
			(*j)[key] = bson_iterator_bool(&subiter);
		else if (bson_iterator_type(&subiter) == BSON_INT)
			(*j)[key] = bson_iterator_int(&subiter);
		else if (bson_iterator_type(&subiter) == BSON_LONG)
			(*j)[key] = (Json::Value::UInt64)bson_iterator_long(&subiter);
		else if (bson_iterator_type(&subiter) == BSON_ARRAY && depth < 5)
		{
			bson_iterator arrayiter;
			bson_iterator_subiterator(&subiter, &arrayiter);
			int length = 0, length2 = 0;
			while (bson_iterator_next(&arrayiter))
			{
				if (bson_iterator_type(&arrayiter) == BSON_OBJECT && !strcmp(bson_iterator_key(&arrayiter), "part"))
				{
					Json::Value tempPart;
					ConvertBsonToJson(&arrayiter, &tempPart, depth + 1);
					(*j)["links"].append(tempPart);
					length++;
				}
				else if (bson_iterator_type(&arrayiter) == BSON_INT && !strcmp(bson_iterator_key(&arrayiter), "saveID"))
				{
					(*j)["links"].append(bson_iterator_int(&arrayiter));
				}
				length2++;
				if (length > (int)(40 / ((depth+1) * (depth+1))) || length2 > 50)
					break;
			}
		}
	}
}

std::set<int> Save::GetNestedSaveIDs(Json::Value j)
{
	Json::Value::Members members = j.getMemberNames();
	std::set<int> saveIDs = std::set<int>();
	for (Json::Value::Members::iterator iter = members.begin(), end = members.end(); iter != end; ++iter)
	{
		std::string member = *iter;
		if (member == "id" && j[member].isInt())
			saveIDs.insert(j[member].asInt());
		else if (j[member].isArray())
		{
			for (Json::Value::ArrayIndex i = 0; i < j[member].size(); i++)
			{
				// only supports objects and ints here because that is all we need
				if (j[member][i].isInt())
				{
					saveIDs.insert(j[member][i].asInt());
					continue;
				}
				if (!j[member][i].isObject())
					continue;
				std::set<int> nestedSaveIDs = GetNestedSaveIDs(j[member][i]);
				saveIDs.insert(nestedSaveIDs.begin(), nestedSaveIDs.end());
			}
		}
	}
	return saveIDs;
}

// converts a json object to bson
void Save::ConvertJsonToBson(bson *b, Json::Value j, int depth)
{
	Json::Value::Members members = j.getMemberNames();
	for (Json::Value::Members::iterator iter = members.begin(), end = members.end(); iter != end; ++iter)
	{
		std::string member = *iter;
		if (j[member].isString())
			bson_append_string(b, member.c_str(), j[member].asCString());
		else if (j[member].isBool())
			bson_append_bool(b, member.c_str(), j[member].asBool());
		else if (j[member].type() == Json::intValue)
			bson_append_int(b, member.c_str(), j[member].asInt());
		else if (j[member].type() == Json::uintValue)
			bson_append_long(b, member.c_str(), j[member].asInt64());
		else if (j[member].isArray())
		{
			bson_append_start_array(b, member.c_str());
			std::set<int> saveIDs = std::set<int>();
			int length = 0;
			for (Json::Value::ArrayIndex i = 0; i < j[member].size(); i++)
			{
				// only supports objects and ints here because that is all we need
				if (j[member][i].isInt())
				{
					saveIDs.insert(j[member][i].asInt());
					continue;
				}
				if (!j[member][i].isObject())
					continue;
				if (depth > 4 || length > (int)(40 / ((depth+1) * (depth+1))))
				{
					std::set<int> nestedSaveIDs = GetNestedSaveIDs(j[member][i]);
					saveIDs.insert(nestedSaveIDs.begin(), nestedSaveIDs.end());
				}
				else
				{
					bson_append_start_object(b, "part");
					ConvertJsonToBson(b, j[member][i], depth+1);
					bson_append_finish_object(b);
				}
				length++;
			}
			for (std::set<int>::iterator iter = saveIDs.begin(), end = saveIDs.end(); iter != end; ++iter)
			{
				bson_append_int(b, "saveID", *iter);
			}
			bson_append_finish_array(b);
		}
	}
}

bool Save::PressureInTmp3(int type)
{
	return type == PT_QRTZ || type == PT_GLAS || type == PT_TUNG;
}

Save& Save::operator <<(particle v)
{
	if (particlesCount < NPART && v.type)
	{
		particles[particlesCount++] = v;
	}
	return *this;
}

Save& Save::operator <<(Sign v)
{
	if (signs.size() < MAXSIGNS && v.GetText().length())
		signs.push_back(v);
	return *this;
}
