#pragma once

#include "Utilities/FileExplorer/FileTreeNode.h"

class FileListView {
public:
	FileListView();

	static void drawUI();

private:
	static std::string chooseIconBasedOnExtension(const std::string& filename);
	static std::string chooseIconCode(const FileTreeNode& node);

	static void renderDescendants(FileTreeNode& root);
	static void renderRootNode(FileTreeNode& root);

	static void renderDeleteConfirmationPrompt(FileTreeNode& toDelete);
};