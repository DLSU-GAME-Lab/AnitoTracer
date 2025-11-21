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

	void HierarchyMenuPopup();
	void CreatePrimitive(GameObject::PrimitiveType type, String name);
	void CreateObjectPopup();
	void updateObjectList(const char* filter);
	void drawObjectNode(GameObject* obj);

	mutable std::unordered_set<std::string> openNodes;

	bool isDragging = false;

	int tempId = 0;

	friend class UIManager;
};

