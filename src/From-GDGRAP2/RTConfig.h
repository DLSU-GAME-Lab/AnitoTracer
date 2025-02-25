#pragma once
#include <filesystem>
#include <string>

struct ApplicationConfig
{
	static constexpr float ASPECT_RATIO = 16.0f / 10.0f;
	static const int APP_WINDOW_WIDTH = 1920;
	static constexpr int APP_WINDOW_HEIGHT = static_cast<int>(APP_WINDOW_WIDTH / ASPECT_RATIO);

	static const inline std::string DEFAULT_UI_LAYOUT_PATH = ([] {
		std::filesystem::path slnDir = SOLUTION_DIR; 
		return (slnDir / "src/imgui_default_layout.ini").string();
		})();
};

struct RTConfig_Debug
{
	static const int RAY_SAMPLES_PER_PIXEL = 5;
	static const int BOUNCES = 2;

	static const int WIDTH = 256;
	static constexpr float ASPECT_RATIO = 3.0f / 2.0f;
	// static constexpr float ASPECT_RATIO = 1.0f;
	static const int HEIGHT = static_cast<int>(WIDTH / ASPECT_RATIO);
	static const int NUM_CORES = 24;

	static const int WIDTH_WINDOW = static_cast<int>(WIDTH / NUM_CORES);
	static const int HEIGHT_WINDOW = HEIGHT;

	static const inline std::string SAVE_IMAGE_PATH = "X:/GithubProjects/RayTracingInVulkan/ImageRender.png";
};

struct RTConfig_Production
{
	static const int RAY_SAMPLES_PER_PIXEL = 250;
	static const int BOUNCES = 10;

	// static const int WIDTH = 1920;
	static const int WIDTH = 1024;
	static constexpr float ASPECT_RATIO = 3.0f / 2.0f;
	// static constexpr float ASPECT_RATIO = 1.0f;
	static const int HEIGHT = static_cast<int>(WIDTH / ASPECT_RATIO);
	static const int NUM_CORES = 24;

	static const int WIDTH_WINDOW = static_cast<int>(WIDTH / NUM_CORES);
	static const int HEIGHT_WINDOW = HEIGHT;

	static const inline std::string SAVE_IMAGE_PATH = "X:/GithubProjects/RayTracingInVulkan/ImageRender.png";
};