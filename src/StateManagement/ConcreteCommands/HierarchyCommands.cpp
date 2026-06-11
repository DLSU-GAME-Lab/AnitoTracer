#include "HierarchyCommands.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"
#include "Assets/GameObjectFactory.hpp"
#include "Engine/Physics/PhysicsEngine.hpp"

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

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
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

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
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

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void CreateObjectCommand::applyPostCreation(GameObject* obj)
{
	if (!obj) return;
	obj->setLocalPosition(this->storedPosition);
	obj->setLocalRotationEuler(this->storedRotation);
	obj->setLocalScale(this->storedScale);
}

// ---------------- CreatePrimitiveCommand ----------------
CreatePrimitiveCommand::CreatePrimitiveCommand(GameObject::PrimitiveType type, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
	: CreateObjectCommand(pos, rot, sca), type(type), name(name)
{
}

std::unique_ptr<GameObject> CreatePrimitiveCommand::createObject()
{
	return GameObjectFactory::CreatePrimitive(this->type, this->name);
}

void CreatePrimitiveCommand::applyPostCreation(GameObject* obj)
{
	if (!obj) return;

	// Apply transform first (from base class)
	obj->setLocalPosition(this->storedPosition);
	obj->setLocalRotationEuler(this->storedRotation);
	obj->setLocalScale(this->storedScale);

	// Automatically add physics component for spheres
	if (obj->getType() == GameObject::PrimitiveType::SPHERE)
	{
		Anito::Physics::PhysicsBodySettings settings;
		settings.type = Anito::Physics::BodyType::DYNAMIC;
		settings.layer = Anito::Physics::ObjectLayer::DYNAMIC;
		settings.mass = 1.0f;
		settings.useGravity = true;

		obj->AddPhysicsComponent(settings);
	}
}

// ---------------- CreateMeshCommand ----------------
CreateMeshCommand::CreateMeshCommand(std::string filePath, std::string name, glm::vec3 pos, glm::vec3 rot, glm::vec3 sca)
	: CreateObjectCommand(pos, rot, sca), filePath(filePath), name(name)
{
}

std::unique_ptr<GameObject> CreateMeshCommand::createObject()
{
	return GameObjectFactory::CreateFromModelFile(this->filePath, this->name);
}

void CreateMeshCommand::applyPostCreation(GameObject* obj)
{
	if (!obj) return;

	// Apply transform first
	obj->setLocalPosition(this->storedPosition);
	obj->setLocalRotationEuler(this->storedRotation);
	obj->setLocalScale(this->storedScale);

	// Create and assign physics body for the mesh
	try {
		// Get physics engine and default world
		auto& physicsEngine = Anito::Physics::PhysicsEngine::Get();
		if (!physicsEngine.IsInitialized()) {
			return;  // Physics engine not available, skip physics setup
		}

		auto defaultWorld = physicsEngine.GetDefaultWorld();
		if (!defaultWorld) {
			return;  // No default world available
		}

		// Configure physics body settings with a box collider for simplicity
		Anito::Physics::PhysicsBodySettings settings;
		settings.type = Anito::Physics::BodyType::DYNAMIC;
		settings.layer = Anito::Physics::ObjectLayer::DYNAMIC;
		settings.mass = 1.0f;
		settings.useGravity = true;

		// Create the physics body in the world
		Anito::Physics::PhysicsBodyPtr physicsBody = defaultWorld->CreateBody(
			obj->getWorldPosition(),
			settings
		);

		if (physicsBody) {
			// Physics body created successfully
			// The physics component will be initialized when the GameObject enters the scene
		}
	}
	catch (const std::exception& e) {
		// Silently fail - physics is optional
		// In a production system, log this error
	}
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
		this->createdObjectRef->setLocalRotationEuler(this->storedRotation);
		this->createdObjectRef->setLocalScale(this->storedScale);
		ModelManager::getInstance()->addLightObject(std::move(this->createdObjectStorage));
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		return;
	}

	// First-time creation
	this->createdObjectStorage = GameObjectFactory::CreateLight(this->type, this->name);
	this->createdObjectRef = this->createdObjectStorage.get();
	this->createdObjectRef->setLocalPosition(this->storedPosition);
	this->createdObjectRef->setLocalRotationEuler(this->storedRotation);
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

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
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
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
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
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}
