#pragma once

#include <string>
#include "imgui.h"
#include "Utilities/FileExplorer/FileTreeNode.h"

class DragAndDropUtils {
public:
	static void createFullPanelDummy();

	static void attachFileTreeNodeSource(FileTreeNode* srcNode);
	static void attachModelInstantiateTarget();
	static void attachModelInstantiateTargetToViewport(ImGuiViewport* viewport);
	static void attachFileMoveTarget(FileTreeNode* destNode);
};