#pragma once

#include <string>

class DragAndDropUtils {
public:
	static void createFullPanelDummy();

	static void attachModelInstantiateSource(std::string filePath, std::string fileName);
	static void attachModelInstantiateTarget();
};