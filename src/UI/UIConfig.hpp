#pragma once
#include <string>

struct UIConfig
{
	bool inspectorUniformScaling;
	std::string consoleTextLog;
	int  consoleLogCount;

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