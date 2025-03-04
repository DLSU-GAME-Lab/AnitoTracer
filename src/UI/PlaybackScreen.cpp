#include "PlaybackScreen.h"

//#include "EngineBackend.h"
#include "imgui.h"
//#include "LogUtils.h"

namespace gdeng03
{
	PlaybackScreen::PlaybackScreen() : AUIScreen("PlaybackScreen")
	{
		//LogUtils::log(this, "Initialized");
	}

	PlaybackScreen::~PlaybackScreen()
	{
	}

	void PlaybackScreen::drawUI()
	{
		// EngineBackend* backend = EngineBackend::get();
		 std::string playButton;
		// if (backend->getMode() == EngineBackend::EDITOR)
		 	playButton = "Play";
		// else 
		// 	playButton = "Stop";
		//
		 std::string pauseButton;
		// if (backend->getMode() == EngineBackend::PAUSED)
		// 	pauseButton = "Unpause";
		// else 
		 	pauseButton = "Pause";

		ImGui::Begin("Playback", &enabled);

		if (ImGui::Button(playButton.c_str()))
		{
			// if (backend->getMode() == EngineBackend::EDITOR)
			// {
			// 	backend->setMode(EngineBackend::PLAY);
			// }
			// else
			// {
			// 	backend->setMode(EngineBackend::EDITOR);
			// }

		}
		if (ImGui::Button(pauseButton.c_str()))
		{
			// if (backend->getMode() == EngineBackend::PLAY)
			// {
			// 	backend->setMode(EngineBackend::PAUSED);
			// }
			// else if (backend->getMode() == EngineBackend::PAUSED)
			// {
			// 	backend->setMode(EngineBackend::PLAY);
			// }
		}
		if (ImGui::Button("Step"))
		{
			// if (backend->getMode() == EngineBackend::PAUSED)
			// {
			// 	backend->startFrameStep();
			// }
		}

		ImGui::End();
	}
}
