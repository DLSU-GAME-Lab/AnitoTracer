#pragma once

#include <string>
#include "imgui.h"

class DragAndDropUtils {
public:
	static void createFullPanelDummy();

	static void attachModelInstantiateSource(std::string sourcePath);
	static void attachModelInstantiateTarget();
	static void attachModelInstantiateTargetToViewport(ImGuiViewport* viewport);
	static void attachFileMoveTarget(std::string destPath);
};