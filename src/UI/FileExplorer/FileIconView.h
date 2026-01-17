#pragma once

#include "Utilities/FileExplorer/FileTreeNode.h"

class FileIconView {
public:
	static FileIconView* getInstance();

	void drawUI();
	void setCurrentNode(FileTreeNode* node);
	FileTreeNode* getCurrentNode() { return currentNode; };

private:
	static FileIconView* instance;

	FileIconView();

	std::string chooseIconBasedOnExtension(const std::string& filename);
	std::string chooseIconCode(const FileTreeNode& node);

	void renderCurrentNodeChildrenIcons();

	FileTreeNode* currentNode;

	void renderDeleteConfirmationPrompt(FileTreeNode& toDelete);
	void renderNewFolderSetupPrompt(FileTreeNode& targetNode);
};