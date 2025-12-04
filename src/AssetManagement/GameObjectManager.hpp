#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "HotkeySystem/HotkeyListener.hpp"

class GameObject;

class GameObjectManager : public HotkeyListener
{
public:
	using vec3 = glm::vec3;
	using String = std::string;

	using GameObjectPtr = std::unique_ptr<GameObject>;
	using GameObjectList = std::vector<GameObjectPtr>;
	using GameObjectMap = std::unordered_map<uint32_t, GameObject*>;

	static GameObjectManager* getInstance();
	static void initialize();
	static void destroy();

	std::vector<GameObject*> GetAllObjects() const;
	std::vector<GameObject*> GetAllActiveObjects() const;
	std::vector<GameObject*> GetAllActiveAndVisibleObjects() const;
	std::vector<GameObject*> GetAllRootObjects() const;

	void RegisterToMap(GameObject* gameObject);
	void UnregisterFromMap(GameObject* gameObject);
	GameObject* FindObjectByID(uint32_t id) const;

	void AddObject(GameObjectPtr gameObject);
	void AddObjectAtIndex(GameObjectPtr gameObject, int index);
	GameObjectPtr RemoveObject(GameObject* target);

	void SetSelectedObject(GameObject* gameObject);
	GameObject* GetSelectedObject();

	void ClearAllObjects();

	int GetObjectIndex(GameObject* gameObject) const;
	int GetRootCount() const;

	void OnActionPressed(Hotkey::Action action) override;

private:
	GameObjectManager();
	~GameObjectManager();
	GameObjectManager(GameObjectManager const&) {};             // copy constructor is private
	GameObjectManager& operator=(GameObjectManager const&) {};  // assignment operator is private*/
	static GameObjectManager* sharedInstance;

	GameObjectList m_rootObjects;
	GameObjectMap m_objectMap;

	GameObject* selectedObject = nullptr;
	GameObjectPtr copiedObject = nullptr;

	/* Delete Helper */
	GameObjectPtr removeInSubtree(GameObject* parent, GameObject* target);

	/* For Hierarchy Actions and Scene View Menu */
	void PasteObject();
	void CutSelectedObject();
	void CopySelectedObject();
	void DuplicateSelectedObject();
	void DeleteSelectedObject();
};

