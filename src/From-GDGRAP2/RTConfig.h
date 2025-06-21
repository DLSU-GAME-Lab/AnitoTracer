#pragma once
#include <filesystem>
#include <string>

struct ApplicationConfig
{
	static constexpr float ASPECT_RATIO = 16.0f / 10.0f;
	static const int APP_WINDOW_WIDTH = 1920;
	static constexpr int APP_WINDOW_HEIGHT = static_cast<int>(APP_WINDOW_WIDTH / ASPECT_RATIO);

	static const inline std::string IMGUI_INI_PATH = ([] {
		std::filesystem::path slnDir = SOLUTION_DIR;
		return (slnDir / "src/imgui.ini").string();
		})();

	static const inline std::string IMGUI_DYNAMIC_INI_PATH = ([] {
		std::filesystem::path slnDir = SOLUTION_DIR;
		return (slnDir / "src/imgui_dynamic.ini").string();
		})();

	static const inline std::string DEFAULT_UI_LAYOUT_PATH = ([] {
		std::filesystem::path slnDir = SOLUTION_DIR;
		return (slnDir / "src/imgui_default_layout.ini").string();
		})();
};