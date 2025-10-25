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

	static void RenderDescendants(FileTreeNode& root);
	static std::string chooseIconBasedOnExtension(const std::string& filename);
	static std::string getFileExtension(const std::string& filename);
	static std::string chooseIconCode(const FileTreeNode& node);
	static void popupWindowNode(FileTreeNode& node);

	static void RenderRootNode(FileTreeNode& root, const std::string& driveName);
};