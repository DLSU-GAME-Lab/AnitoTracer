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
	
	void popupWindowNode(FileTreeNode& node);
};