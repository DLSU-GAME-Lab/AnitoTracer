#pragma once

#include "Utilities/FileExplorer/FileTreeNode.h"

class FileIconView {
public:
	FileIconView();

	static void drawUI();
	static std::string getRootNodeRelPath();

private:
	static std::string chooseIconBasedOnExtension(const std::string& filename);
	static std::string chooseIconCode(const FileTreeNode& node);

	static void renderDescendants(FileTreeNode& root);
	static void renderRootNode(FileTreeNode& root);

	static std::string currentPath;
};