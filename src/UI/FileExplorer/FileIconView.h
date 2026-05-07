#pragma once

#include "imgui.h"
#include "Utilities/FileExplorer/FileTreeNode.h"
#include <unordered_map>
#include <memory>

class FileIconView {
public:
	static FileIconView* getInstance();

	void drawUI();
	void setCurrentNode(FileTreeNode* node);
	FileTreeNode* getCurrentNode() { return currentNode; };

	void initButtonTexture();

private:
	static FileIconView* instance;

	FileIconView();

	std::string chooseIconBasedOnExtension(const std::string& filename);
	std::string chooseIconCode(const FileTreeNode& node);

	void renderCurrentNodeChildrenIcons();

	FileTreeNode* currentNode;
	ImTextureID currTexId;
	std::unordered_map <std::string, std::shared_ptr<ImTextureID>> iconMap;

	void renderDeleteConfirmationPrompt(FileTreeNode& toDelete);
	void renderNewFolderSetupPrompt(FileTreeNode& targetNode);
};