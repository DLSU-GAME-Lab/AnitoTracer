#pragma once

#include "AUIScreen.h"
#include "Utilities/FileExplorer/FileTreeNode.h"

class ProjectScreen : public AUIScreen
{

public:
	ProjectScreen();
	~ProjectScreen();

private:
	virtual void drawUI() override;

	void renderDescendants(FileTreeNode& root);
	std::string chooseIconBasedOnExtension(const std::string& filename);
	std::string getFileExtension(const std::string& filename);
	std::string chooseIconCode(const FileTreeNode& node);
	void popupWindowNode(FileTreeNode& node);

	void renderRootNode(FileTreeNode& root);
};