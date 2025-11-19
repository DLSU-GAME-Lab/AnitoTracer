#include "HierarchyCommands.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"
#include "Assets/GameObjectFactory.hpp"

ReparentCommand::ReparentCommand(GameObject* child, GameObject* oldParent, int oldIndex, GameObject* newParent, int newIndex)
	: child(child), oldParent(oldParent), oldIndex(oldIndex), newParent(newParent), newIndex(newIndex)
{
}

ReparentCommand::~ReparentCommand()
{
	if(child) delete child;
	if(oldParent) delete oldParent;
	if(newParent) delete newParent;
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

// ---------------- CreateObjectCommand (common create/undo/redo) ----------------
CreateObjectCommand::CreateObjectCommand(glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
	: storedPosition(pos), storedRotation(rot), storedScale(sca)
{
	this->createdObjectRef = nullptr;
}

CreateObjectCommand::~CreateObjectCommand()
{
	delete createdObjectRef;
}

void CreateObjectCommand::execute()
{
	// Redo path: if we already own the object (from undo), re-add it preserving identity
	if (this->createdObjectStorage)
	{
		this->createdObjectRef = this->createdObjectStorage.get();
		applyPostCreation(this->createdObjectRef);
		ModelManager::getInstance()->addObject(std::move(this->createdObjectStorage));
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		return;
	}

	// First-time creation
	this->createdObjectStorage = createObject();
	this->createdObjectRef = this->createdObjectStorage.get();
	applyPostCreation(this->createdObjectRef);
	ModelManager::getInstance()->addObject(std::move(this->createdObjectStorage));
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void CreateObjectCommand::undo()
{
	this->createdObjectStorage = ModelManager::getInstance()->removeObject(this->createdObjectRef);
	// Keep the raw pointer in sync with the storage (or null if removal failed).
	if (this->createdObjectStorage)
		this->createdObjectRef = this->createdObjectStorage.get();
	else
		this->createdObjectRef = nullptr;
}

void CreateObjectCommand::applyPostCreation(GameObject* obj)
{
	if (!obj) return;
	obj->setLocalPosition(this->storedPosition);
	obj->setLocalRotation(this->storedRotation);
	obj->setLocalScale(this->storedScale);
}

// ---------------- CreatePrimitiveCommand ----------------
CreatePrimitiveCommand::CreatePrimitiveCommand(GameObject::PrimitiveType type, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
	: CreateObjectCommand(pos, rot, sca), type(type), name(name)
{
}

std::unique_ptr<GameObject> CreatePrimitiveCommand::createObject()
{
	return GameObjectFactory::getInstance()->CreatePrimitive(this->type, this->name);
}

// ---------------- CreateMeshCommand ----------------
CreateMeshCommand::CreateMeshCommand(std::string filePath, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
	: CreateObjectCommand(pos, rot, sca), filePath(filePath), name(name)
{
}

std::unique_ptr<GameObject> CreateMeshCommand::createObject()
{
	return GameObjectFactory::getInstance()->CreateFromModelFile(this->filePath, this->name);
}

// ---------------- CreateLightCommand ----------------
CreateLightCommand::CreateLightCommand(Light::LightType type, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 sca, glm::vec4 lightCol, glm::vec4 ambientCol)
	: type(type), name(name), storedPosition(pos), storedRotation(rot), storedScale(sca), lightColor(lightCol), ambientColor(ambientCol)
{
}

CreateLightCommand::~CreateLightCommand()
{
	delete createdObjectRef;
}

void CreateLightCommand::execute()
{
	// Redo path: if we already own the object (from undo), re-add it preserving identity
	if (this->createdObjectStorage)
	{
		this->createdObjectRef = this->createdObjectStorage.get();
		this->createdObjectRef->setLocalPosition(this->storedPosition);
		this->createdObjectRef->setLocalRotation(this->storedRotation);
		this->createdObjectRef->setLocalScale(this->storedScale);
		ModelManager::getInstance()->addLightObject(std::move(this->createdObjectStorage));
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		return;
	}

	// First-time creation
	this->createdObjectStorage = GameObjectFactory::getInstance()->CreateLight(this->type, this->name);
	this->createdObjectRef = this->createdObjectStorage.get();
	this->createdObjectRef->setLocalPosition(this->storedPosition);
	this->createdObjectRef->setLocalRotation(this->storedRotation);
	this->createdObjectRef->setLocalScale(this->storedScale);
	ModelManager::getInstance()->addLightObject(std::move(this->createdObjectStorage));
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void CreateLightCommand::undo()
{
	this->createdObjectStorage = ModelManager::getInstance()->removeLightObject(this->createdObjectRef);
	// Keep the raw pointer in sync with the storage (or null if removal failed).
	if (this->createdObjectStorage)
		this->createdObjectRef = this->createdObjectStorage.get();
	else
		this->createdObjectRef = nullptr;
}

DeleteObjectCommand::DeleteObjectCommand(GameObject* objectToDelete) : objectRef(objectToDelete)
{

}

DeleteObjectCommand::~DeleteObjectCommand()
{
	if(objectRef) delete objectRef;
}

void DeleteObjectCommand::execute()
{
	if (!objectRef) return;
	auto objectPtr = ModelManager::getInstance()->removeObject(this->objectRef);
	this->objectStorage = std::move(objectPtr);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void DeleteObjectCommand::undo()
{
	if (!this->objectStorage) return;
	this->objectRef = this->objectStorage.get();
	ModelManager::getInstance()->addObject(std::move(this->objectStorage));
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

AddObjectCommand::AddObjectCommand(std::unique_ptr<GameObject> gameObject)
{
	this->objectStorage = std::move(gameObject);
	this->objectRef = this->objectStorage.get();
}

AddObjectCommand::~AddObjectCommand()
{
	if (objectRef) delete objectRef;
}

void AddObjectCommand::execute()
{
	if (!this->objectStorage) return;

	if (!this->objectRef->getParent())
	{
		ModelManager::getInstance()->addObject(std::move(this->objectStorage));
	}
	else
	{
		this->objectStorage->getParent()->addChild(std::move(this->objectStorage));
	}

	this->objectStorage = nullptr;
} 

void AddObjectCommand::undo()
{
	if (!this->objectRef) return;

	if (!this->objectRef->getParent())
	{
		this->objectStorage = ModelManager::getInstance()->removeObject(this->objectRef);
	}
	else
	{
		this->objectStorage = this->objectRef->getParent()->removeChild(this->objectRef);
	}
}
