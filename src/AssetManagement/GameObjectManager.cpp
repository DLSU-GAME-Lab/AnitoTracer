#include "GameObjectManager.hpp"

#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"
#include "StateManagement/ConcreteCommands/HierarchyCommands.hpp"
#include "HotkeySystem/HotkeySystem.hpp"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "Engine/CameraSystem/CameraManager.h"
#include "GameObjectFactory.hpp"
#include "GameObject.hpp"

GameObjectManager* GameObjectManager::sharedInstance = nullptr;

GameObjectManager* GameObjectManager::getInstance()
{
	return sharedInstance;
}

void GameObjectManager::initialize()
{
	sharedInstance = new GameObjectManager();
}

void GameObjectManager::destroy()
{
	sharedInstance->m_rootObjects.clear();
	delete sharedInstance;
}

GameObjectManager::GameObjectManager()
{
	HotkeySystem::getInstance()->addListener(this);
}

GameObjectManager::~GameObjectManager()
{
	HotkeySystem::getInstance()->removeListener(this);
}

std::vector<GameObject*> GameObjectManager::GetAllObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->m_rootObjects)
	{
		objectList.push_back(gameObject.get());

		auto descendants = gameObject->GetChildrenRecursive();

		objectList.insert(objectList.end(), descendants.begin(), descendants.end());
	}

	return objectList;
}

std::vector<GameObject*> GameObjectManager::GetAllActiveObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->m_rootObjects)
	{
		if (!gameObject->IsActive()) continue;

		objectList.push_back(gameObject.get());

		auto descendants = gameObject->GetChildrenRecursive();

		for(auto descendant : descendants)
		{
			if (descendant->IsActive())
				objectList.push_back(descendant);
		}
	}

	return objectList;
}

std::vector<GameObject*> GameObjectManager::GetAllActiveAndVisibleObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->m_rootObjects)
	{
		if (!gameObject->IsActive() || !gameObject->IsVisible()) continue;

		objectList.push_back(gameObject.get());

		auto descendants = gameObject->GetChildrenRecursive();

		for (auto descendant : descendants)
		{
			if (descendant->IsActive() || descendant->IsVisible())
				objectList.push_back(descendant);
		}
	}

	return objectList;
}

std::vector<GameObject*> GameObjectManager::GetAllRootObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->m_rootObjects)
	{
		objectList.push_back(gameObject.get());
	}

	return objectList;
}

void GameObjectManager::RegisterToMap(GameObject* gameObject)
{
	auto it = this->m_objectMap.find(gameObject->GetID());

	if (it != this->m_objectMap.end())
	{
		Debug::Log("Warning:GameObject with ID " + std::to_string(gameObject->GetID()) + " is already registered in ModelManager map. Overriting");
	}

	this->m_objectMap[gameObject->GetID()] = gameObject;
}

void GameObjectManager::UnregisterFromMap(GameObject* gameObject)
{
	this->m_objectMap.erase(gameObject->GetID());
}

GameObject* GameObjectManager::FindObjectByID(uint32_t id) const
{
	auto it = this->m_objectMap.find(id);

	return (it != this->m_objectMap.end()) ? it->second : nullptr;
}

void GameObjectManager::AddObject(GameObjectManager::GameObjectPtr gameObject)
{
	std::string message = "Added game object to root: " + gameObject->GetName();
	Debug::Log(message);

	this->m_rootObjects.push_back(std::move(gameObject));
}

void GameObjectManager::AddObjectAtIndex(GameObjectPtr gameObject, int index)
{
	// Clamp index to valid range [0, sceneGraph.size()]
	size_t idx = 0;
	if (index > 0)
		idx = static_cast<size_t>(index);
	if (idx > this->m_rootObjects.size()) idx = this->m_rootObjects.size();

	std::string message = "Added game object to root: " + gameObject->GetName() + " at index " + std::to_string(idx);
	Debug::Log(message);

	this->m_rootObjects.insert(this->m_rootObjects.begin() + idx, std::move(gameObject));
}

std::unique_ptr<GameObject> GameObjectManager::RemoveObject(GameObject* target)
{
	if (!target) return nullptr;

	for (auto it = this->m_rootObjects.begin(); it != this->m_rootObjects.end(); it++)
	{
		if (it->get() == target)
		{
			std::unique_ptr<GameObject> removed = std::move(*it);
			this->m_rootObjects.erase(it);
			return removed;
		}
	
		std::unique_ptr<GameObject> result = removeInSubtree(it->get(), target);

		if (result)
		{
			UnregisterFromMap(result.get());
			GameObjectFactory::ReleaseId(result->GetID());
			return result;
		}
			
	}

	return nullptr;
}

GameObjectManager::GameObjectPtr GameObjectManager::removeInSubtree(GameObject* parent, GameObject* target)
{
	std::vector<GameObject*> children = parent->GetChildren();

	for (auto child : children)
	{
		if (child == target)
		{
			return parent->RemoveChild(child);
		}
		std::unique_ptr<GameObject> result = removeInSubtree(child, target);

		if (result)
		{
			return result;
		}
	}

}

void GameObjectManager::SetSelectedObject(GameObject* gameObject)
{
	this->selectedObject = gameObject;
}

GameObject* GameObjectManager::GetSelectedObject()
{
	return this->selectedObject;
}

void GameObjectManager::ClearAllObjects()
{
	this->m_rootObjects.clear();
	this->m_objectMap.clear();
}

int GameObjectManager::GetObjectIndex(GameObject* gameObject) const
{
	if(gameObject == nullptr) return -1;

	for (size_t i = 0; i < this->m_rootObjects.size(); i++)
	{
		if (this->m_rootObjects[i].get() == gameObject)
			return static_cast<int>(i);
	}

	return -1;
}

int GameObjectManager::GetRootCount() const
{
	return this->m_rootObjects.size();
}

void GameObjectManager::OnActionPressed(Hotkey::Action action)
{
	/* paste only needs valid copied object */
	if (action == Hotkey::Action::GameObject_Paste)	PasteObject();

	if (!this->selectedObject) return; // all actions involve selected object

	if (action == Hotkey::Action::GameObject_ToggleActive)
	{
		auto currentState = this->selectedObject->IsActive();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->SetActive(std::get<bool>(v)); },
				currentState,
				!currentState
			));

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

	if (action == Hotkey::Action::GameObject_Delete)	DeleteSelectedObject();
	if (action == Hotkey::Action::GameObject_Duplicate)	DuplicateSelectedObject();
	if (action == Hotkey::Action::GameObject_Copy)		CopySelectedObject();
	if (action == Hotkey::Action::GameObject_Cut)		CutSelectedObject();

	if (action == Hotkey::Action::GameObject_SetAsFirstSibling)
	{
		auto parent = this->selectedObject->GetParent();

		CommandManager::getInstance()->executeCommand(
			new ReparentCommand(
				this->selectedObject,
				parent,
				parent ? parent->GetChildIndex(this->selectedObject) : this->GetObjectIndex(this->selectedObject),
				parent,
				0
			)
		);
	}

	if (action == Hotkey::Action::GameObject_SetAsLastSibling)
	{
		auto parent = this->selectedObject->GetParent();

		CommandManager::getInstance()->executeCommand(
			new ReparentCommand(
				this->selectedObject,
				parent,
				parent ? parent->GetChildIndex(this->selectedObject) : this->GetObjectIndex(this->selectedObject),
				parent,
				parent ? parent->GetChildren().size() : this->GetRootCount()
			)
		);
	}

	if (action == Hotkey::Action::GameObject_TogglePickabilityWithDescendants)
	{
		auto currentState = this->selectedObject->IsPickable();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->SetPickable(std::get<bool>(v)); },
				currentState,
				!currentState
			));
	}

	if (action == Hotkey::Action::GameObject_ToggleVisibilityWithDescendants)
	{
		auto currentState = this->selectedObject->IsVisible();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->SetVisible(std::get<bool>(v)); },
				currentState,
				!currentState
			));

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

void GameObjectManager::CutSelectedObject()
{
	this->copiedObject = GameObjectFactory::CreateGameObjectCopy(this->selectedObject);
	CommandManager::getInstance()->executeCommand(
		new DeleteObjectCommand(this->selectedObject)
	);
	this->selectedObject = nullptr;
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void GameObjectManager::CopySelectedObject()
{
	this->copiedObject = GameObjectFactory::CreateGameObjectCopy(this->selectedObject);
}

void GameObjectManager::DuplicateSelectedObject()
{
	auto duplicate = GameObjectFactory::CreateGameObjectCopy(this->selectedObject);

	glm::vec3 offset = { 10.0f, 10.0f, 10.0 }; //offset spawn
	duplicate->SetLocalPosition(this->selectedObject->GetLocalPosition() + offset);

	CommandManager::getInstance()->executeCommand(
		new AddObjectCommand(std::move(duplicate))
	);

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void GameObjectManager::DeleteSelectedObject()
{
	CommandManager::getInstance()->executeCommand(
		new DeleteObjectCommand(this->selectedObject)
	);
	this->selectedObject = nullptr;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); //AnitoTracer Specific
}

/* Where the object is spawned needs to be decided  (world origin vs infront of camera vs beside copy) */
void GameObjectManager::PasteObject()
{
	if (!this->copiedObject) return;

	auto sceneCamera = CameraManager::getInstance()->getActiveCamera();

	this->copiedObject->SetLocalPosition(sceneCamera->GetForward() * 500.0f);

	CommandManager::getInstance()->executeCommand(
		new AddObjectCommand(std::move(this->copiedObject))
	);

	this->copiedObject = nullptr;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}




