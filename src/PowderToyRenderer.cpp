#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "graphics/Renderer.h"
#include "common/String.h"
#include "common/tpt-rand.h"
#include "Format.h"
#include "gui/interface/Engine.h"
#include "client/GameSave.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "common/platform/Platform.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[])
{
	if (!argv[1] || !argv[2]) {
		std::cout << "Usage: " << argv[0] << " <inputFilename> <outputPrefix>" << std::endl;
		return 1;
	}
	auto inputFilename = ByteString(argv[1]);
	auto outputPrefix = ByteString(argv[2]);
	ByteString ppmFilename = outputPrefix+".ppm";
	ByteString ptiFilename = outputPrefix+".pti";
	ByteString ptiSmallFilename = outputPrefix+"-small.pti";
	ByteString pngFilename = outputPrefix+".png";
	ByteString pngSmallFilename = outputPrefix+"-small.png";

	auto simulationData = std::make_unique<SimulationData>();

	std::vector<char> fileData;
	if (!Platform::ReadFile(fileData, inputFilename))
	{
		return 1;
	}

	std::unique_ptr<GameSave> gameSave;
	try
	{
		gameSave = std::make_unique<GameSave>(fileData, false);
	}
	catch (ParseException &e)
	{
		//Render the save again later or something? I don't know
		if (ByteString(e.what()).FromUtf8() == "Save from newer version")
			throw e;
	}

	Simulation * sim = new Simulation();
	Renderer * ren = new Renderer();
	ren->sim = sim;

	if (gameSave)
	{
		sim->Load(gameSave.get(), true, { 0, 0 });

		//Render save
		RendererSettings rendererSettings;
		rendererSettings.decorationLevel = RendererSettings::decorationAntiClickbait;
		ren->ApplySettings(rendererSettings);
		ren->ClearAccumulation();
		ren->Clear();
		ren->ApproximateAccumulation();
		ren->RenderSimulation();
	}
	else
	{
		ren->Clear();
		int w = Graphics::TextSize("Save file invalid").X + 15, x = (XRES-w)/2, y = (YRES-24)/2;
		ren->DrawRect(RectSized(Vec2{ x, y }, Vec2{ w, 24 }), 0xC0C0C0_rgb);
		ren->BlendText({ x+8, y+8 }, "Save file invalid", 0xC0C0F0_rgb .WithAlpha(255));
	}

	auto &video = ren->GetVideo();
	VideoBuffer screenBuffer = VideoBuffer(video.data(), RES, video.Size().X);

	if (auto data = screenBuffer.ToPNG())
		Platform::WriteFile(*data, pngFilename);

	if (auto data = screenBuffer.ToPTI())
		Platform::WriteFile(*data, ptiFilename);

	screenBuffer.Resize(1.0f/3.0f, true);

	if (auto data = screenBuffer.ToPNG())
		Platform::WriteFile(*data, pngSmallFilename);

	if (auto data = screenBuffer.ToPTI())
		Platform::WriteFile(*data, ptiSmallFilename);
}
