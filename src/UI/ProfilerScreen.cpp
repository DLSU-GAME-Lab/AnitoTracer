#include "ProfilerScreen.h"

#include "UIManager.h"

ProfilerScreen::ProfilerScreen():AUIScreen(UINames::PLAYBACK_SCREEN)
{
}

ProfilerScreen::~ProfilerScreen()
{
	AUIScreen::~AUIScreen();
}

void ProfilerScreen::drawUI()
{
	ImGui::Begin("Profiler");
	ImGui::Text("Frame rate: %.1f FPS", ImGui::GetIO().Framerate);

	profiler = UIManager::getInstance()->profiler; // This function is from your UIManager class
	if (profiler)
		profiler->DrawImGui(); // This function is from your GpuCpuProfiler class

	ImGui::End();
}
