#pragma once

#include "Utilities/FileExplorer/FileTreeNode.h"

class FileIconView {
public:
	FileIconView();

	static void drawUI();
	static std::string getRootNodeRelPath();
	static void setCurrentNode(FileTreeNode& node);

private:
	static std::string chooseIconBasedOnExtension(const std::string& filename);
	static std::string chooseIconCode(const FileTreeNode& node);

	static void renderCurrentNode();

	static FileTreeNode currentNode;
};