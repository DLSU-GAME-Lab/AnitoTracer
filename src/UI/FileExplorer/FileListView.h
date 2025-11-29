#pragma once

#include "Utilities/FileExplorer/FileTreeNode.h"

class FileListView {
public:
	static FileListView* getInstance();

	void drawUI();

private:
	static FileListView* instance;

	FileListView();

	std::string chooseIconBasedOnExtension(const std::string& filename);
	std::string chooseIconCode(const FileTreeNode& node);

	void renderDescendants(FileTreeNode& root);
	void renderRootNode(FileTreeNode& root);

	void renderDeleteConfirmationPrompt(FileTreeNode& toDelete);
};