#include "HierarchyCommands.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"

ReparentCommand::ReparentCommand(GameObject* child, GameObject* oldParent, int oldIndex, GameObject* newParent, int newIndex)
	: child(child), oldParent(oldParent), oldIndex(oldIndex), newParent(newParent), newIndex(newIndex)
{

}

void ReparentCommand::execute()
{
	std::unique_ptr<GameObject> childPtr;

	if (oldParent == nullptr) //nullptr means root
	{
		childPtr = std::move(ModelManager::getInstance()->removeObject(child));
	}
	else
	{
		childPtr = std::move(oldParent->removeChild(child));
	}

	if(newParent == nullptr)
	{
		ModelManager::getInstance()->addObjectAtIndex(std::move(childPtr), newIndex);
	}
	else
	{
		newParent->addChildAtIndex(std::move(childPtr), newIndex);
	}

}

void ReparentCommand::undo()
{
	std::unique_ptr<GameObject> childPtr;

	if (newParent == nullptr)
	{
		childPtr = std::move(ModelManager::getInstance()->removeObject(child));
	}
	else
	{
		childPtr = std::move(newParent->removeChild(child));
	}

	if (oldParent == nullptr)
	{
		ModelManager::getInstance()->addObjectAtIndex(std::move(childPtr), oldIndex);
	}
	else
	{
		oldParent->addChildAtIndex(std::move(childPtr), oldIndex);
	}
}
