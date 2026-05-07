#pragma once
#include <string>
#include "imgui.h"
#include "ImGuizmo.h"

struct UIConfig
{
	bool inspectorUniformScaling;
	std::string consoleTextLog;
	int  consoleLogCount;
	ImGuizmo::OPERATION currentGizmoOperation;

	bool isInspectorEnabled;
	bool isHierarchyEnabled;
	bool isProjectEnabled;
	bool isConsoleEnabled;
	bool isProfilerEnabled;
	bool isMaterialEditorEnabled;
	bool isSettingsEnabled;

	UIConfig()
		: inspectorUniformScaling(false)
		, consoleTextLog("")
		, consoleLogCount(0)
		, currentGizmoOperation(ImGuizmo::TRANSLATE)
		, isInspectorEnabled(true)
		, isHierarchyEnabled(true)
		, isProjectEnabled(true)
		, isConsoleEnabled(true)
		, isProfilerEnabled(true)
		, isMaterialEditorEnabled(true)
		, isSettingsEnabled(false)
	{
	}
};