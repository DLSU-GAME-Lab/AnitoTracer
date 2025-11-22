#pragma once

#include <string>

class DragAndDropUtils {
public:
	static void createFullPanelDummy();

	static void attachModelInstantiateSource(std::string sourcePath);
	static void attachModelInstantiateTarget();
	static void attachFileMoveTarget(std::string destPath);
};