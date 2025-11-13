#include "HierarchyCommands.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"
#include "Assets/GameObjectFactory.hpp"

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

CreatePrimitiveCommand::CreatePrimitiveCommand(GameObject::PrimitiveType type, std::string name) : type(type), name(name)
{
	this->createdObjectRef = nullptr;
}

void CreatePrimitiveCommand::execute()
{
	// If we already hold the created object (after an undo), re-add it (redo).
	if (this->createdObjectStorage)
	{
		this->createdObjectRef = this->createdObjectStorage.get();
		ModelManager::getInstance()->addObject(std::move(this->createdObjectStorage));
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
	// First-time execute: create and add.
	else
	{
		this->createdObjectStorage = GameObjectFactory::getInstance()->CreatePrimitive(this->type);
		this->createdObjectRef = this->createdObjectStorage.get();
		ModelManager::getInstance()->addObject(std::move(this->createdObjectStorage));
	}
}

void CreatePrimitiveCommand::undo()
{
	this->createdObjectStorage = ModelManager::getInstance()->removeObject(this->createdObjectRef);
	// Keep the raw pointer in sync with the storage (or null if removal failed).
	if (this->createdObjectStorage)
		this->createdObjectRef = this->createdObjectStorage.get();
	else
		this->createdObjectRef = nullptr;
}
