#include "ProfilerScreen.h"

ProfilerScreen::ProfilerScreen():AUIScreen(UINames::PLAYBACK_SCREEN), cpuGraph(300), gpuGraph(300)
{
	stopProfiling = false;
	frameOffset = 0;
	frameWidth = 3;
	frameSpacing = 1;
	useColoredLegendText = true;
	prevFpsFrameTime = std::chrono::system_clock::now();
	fpsFramesCount = 0;
	avgFrameTime = 1.0f;
}

ProfilerScreen::~ProfilerScreen()
{
	AUIScreen::~AUIScreen();
}

void ProfilerScreen::drawUI()
{
	ImGui::Begin("Profiler");
	ImGui::Text("Frame rate: %.1f FPS", ImGui::GetIO().Framerate);
	ImGui::End();

	////cpuGraph.LoadFrameData();
	//fpsFramesCount++;
	//auto currFrameTime = std::chrono::system_clock::now();
	//{
	//	float fpsDeltaTime = std::chrono::duration<float>(currFrameTime - prevFpsFrameTime).count();
	//	if (fpsDeltaTime > 0.5f)
	//	{
	//		this->avgFrameTime = fpsDeltaTime / float(fpsFramesCount);
	//		fpsFramesCount = 0;
	//		prevFpsFrameTime = currFrameTime;
	//	}
	//}

	//std::string title = "Profiler [" + std::to_string(1.0f / avgFrameTime) + "fps\t" + std::to_string(avgFrameTime * 1000.0f) + "ms]";
	//
	////###AnimatedTitle
	////ImGui::Begin(title.c_str(), &enabled, ImGuiWindowFlags_NoScrollbar);
	//ImGui::Begin("Profiler");
	//ImGui::Text("Frame rate: %.1f FPS", ImGui::GetIO().Framerate);
	//ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	//ImGui::Text("Canvas Size: %.1f, %.1f", canvasSize.x, canvasSize.y);

	//int sizeMargin = int(ImGui::GetStyle().ItemSpacing.y);
	//int maxGraphHeight = 300;
	//int availableGraphHeight = (int(canvasSize.y) - sizeMargin) / 2;
	//int graphHeight = std::min(maxGraphHeight, availableGraphHeight);
	//int legendWidth = 200;
	//int graphWidth = int(canvasSize.x) - legendWidth;
	//gpuGraph.RenderTimings(graphWidth, legendWidth, graphHeight, frameOffset);
	//cpuGraph.RenderTimings(graphWidth, legendWidth, graphHeight, frameOffset);

	//if (graphHeight * 2 + sizeMargin + sizeMargin < canvasSize.y)
	//{
	//	ImGui::Columns(2);
	//	size_t textSize = 50;
	//	ImGui::Checkbox("Stop profiling", &stopProfiling);
	//	ImGui::Checkbox("Colored legend text", &useColoredLegendText);
	//	ImGui::DragInt("Frame offset", &frameOffset, 1.0f, 0, 400);
	//	ImGui::NextColumn();

	//	ImGui::SliderInt("Frame width", &frameWidth, 1, 4);
	//	ImGui::SliderInt("Frame spacing", &frameSpacing, 0, 2);
	//	ImGui::SliderFloat("Transparency", &ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w, 0.0f, 1.0f);
	//	ImGui::Columns(1);
	//}
	//if (!stopProfiling)
	//	frameOffset = 0;

	//gpuGraph.frameWidth = frameWidth;
	//gpuGraph.frameSpacing = frameSpacing;
	//gpuGraph.useColoredLegendText = useColoredLegendText;
	//cpuGraph.frameWidth = frameWidth;
	//cpuGraph.frameSpacing = frameSpacing;
	//cpuGraph.useColoredLegendText = useColoredLegendText;

	//ImGui::End();
}
