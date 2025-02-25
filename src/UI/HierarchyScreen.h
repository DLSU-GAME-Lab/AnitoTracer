#pragma once
#include <unordered_set>

#include "AUIScreen.h"
#include "From-GDGRAP2/GameObject.h"

class HierarchyScreen :    public AUIScreen
{
public:
	HierarchyScreen();
	~HierarchyScreen();
private:
	virtual void drawUI() override;
	void updateObjectList(const char* filter) const;
	void drawObjectNode(GameObject* obj) const;

	mutable std::unordered_set<std::string> openNodes;  

	friend class UIManager;
};

