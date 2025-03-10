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
};

class UISettings
{
public:
	static constexpr int GlobalWindowFlags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoFocusOnAppearing 
	;
};

class AUIScreen
{
protected:
	typedef std::string String;

	AUIScreen(const String& name);
	~AUIScreen();

	String getName();
	virtual void drawUI() = 0;
	void setEnabled(bool flag);
	void toggleEnabled();

	String name;
	bool enabled = true;
	friend class UIManager;
};

