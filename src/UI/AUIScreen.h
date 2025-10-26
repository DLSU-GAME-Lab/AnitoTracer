#pragma once

#include <imgui.h>
#include <string>

typedef std::string String;
class UIManager;

class UINames {
public:
	static constexpr char PROFILER_SCREEN[] = "PROFILER_SCREEN";
	static constexpr char MENU_SCREEN[] = "MENU_SCREEN";
	static constexpr char INSPECTOR_SCREEN[] = "INSPECTOR_SCREEN";
	static constexpr char HIERARCHY_SCREEN[] = "HIERARCHY_SCREEN";
	static constexpr char PLAYBACK_SCREEN[] = "PLAYBACK_SCREEN";
	static constexpr char ACTION_SCREEN[] = "ACTION_SCREEN";
	static constexpr char CONSOLE_SCREEN[] = "CONSOLE_SCREEN";
	static constexpr char MATERIAL_SCREEN[] = "MATERIAL_SCREEN";
	static constexpr char VIEWPORT_SCREEN[] = "VIEWPORT_SCREEN";
	static constexpr char MATERIAL_EDITOR_SCREEN[] = "MATERIAL_EDITOR_SCREEN";
	static constexpr char ASSET_EXPLORER_SCREEN[] = "ASSET_EXPLORER_SCREEN";
	static constexpr char SETTINGS_SCREEN[] = "SETTINGS_SCREEN";
	static constexpr char PROJECT_SCREEN[] = "PROJECT_SCREEN";
};

class UISettings
{
public:
	static constexpr int GlobalWindowFlags =
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoFocusOnAppearing 
	;
	static constexpr int MainWindowFlags = 
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBackground;
};

class AUIScreen
{
protected:
	enum class ScreenAlign : uint8_t
	{
		TOP_LEFT,
		TOP_RIGHT,
		TOP_CENTER,
		CENTER_LEFT,
		CENTER_RIGHT,
		CENTER,
		BOT_LEFT,
		BOT_RIGHT,
		BOT_CENTER
	};

	typedef std::string String;

	AUIScreen(const String& name);
	~AUIScreen();

	String getName();
	virtual void drawUI() = 0;

	static void setWindowAlignment(const ScreenAlign& alignment, const ImGuiCond& condition = ImGuiCond_Appearing);
	void setEnabled(bool flag);
	void toggleEnabled();

	String name;
	bool enabled = true;
	friend class UIManager;
};

