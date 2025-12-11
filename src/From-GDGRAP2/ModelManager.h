#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ObjectGroup.h"
#include "Assets/Texture.hpp"
#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"
#include "HotkeySystem/HotkeyListener.hpp"

/**
 * \brief Similar to the game object manager, this class stores model instances
 */
class ModelManager : public HotkeyListener
{
public:
	using vec3 = glm::vec3;
	using String = std::string;

	using GameObjectPtr = std::unique_ptr<GameObject>;
	using GameObjectList = std::vector<GameObjectPtr>;

	using LightPtr = std::unique_ptr<Light>;
	using LightList = std::vector<Light*>;

	typedef std::vector<Assets::LightProperties> LightPropsList;

	static ModelManager* getInstance();
	static void initialize();
	static void destroy();

	std::vector<GameObject*> getAllObjects() const;
	std::vector<GameObject*> GetAllActiveAndVisibleObjects() const;
	std::vector<GameObject*> getSceneGraph() const;
	
	int activeObjectsCount() const;

	void addObject(GameObjectPtr gameObject);
	void addObjectAtIndex(GameObjectPtr gameObject, int index);
	GameObjectPtr removeObject(GameObject* target);
	void deleteObject(GameObject* gameObject);

	GameObjectPtr CreateCopyOfObject(GameObject* original);

	void addLightObject(LightPtr lightObj);
	LightPtr removeLightObject(Light* gameObject);

	void setSelectedObject(GameObject* gameObject);
	GameObject* getSelectedObject();

	void clearAllObjects();

	std::vector<Assets::Model> getAllObjectModels() const;
	LightPropsList getAllLightProperties() const;

	int getObjectIndex(GameObject* gameObject) const;
	int getSceneGraphRootSize() const;

	void createObject(GameObject::PrimitiveType type);
	void createPrimitiveFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation, vec3 scale, std::vector<Assets::Material> mats);
	void createLightFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation, vec3 scale, std::vector<Assets::Material> mats, Assets::LightProperties props);
	void createObjectFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation, vec3 scale); 
	void createObjectGroupFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation, vec3 scale);
	void createSponza();

	void OnActionPressed(Hotkey::Action action) override;

	/* For Hierarchy Actions and Scene View Menu */
	void PasteObject();
	void CutSelectedObject();
	void CopySelectedObject();
	void DuplicateSelectedObject();
	void DeleteSelectedObject();

	void ClearInstanceToObjectMap();
	void RegisterInstance(uint32_t instanceId, GameObject* gameObject);
	GameObject* FindGameObject(uint32_t instanceId) const;

private:
	ModelManager();
	~ModelManager();
	ModelManager(ModelManager const&) {};             // copy constructor is private
	ModelManager& operator=(ModelManager const&) {};  // assignment operator is private*/
	static ModelManager* sharedInstance;

	GameObjectList sceneGraph;
	LightList lightList;

	GameObject* selectedObject = nullptr;
	GameObjectPtr copiedObject = nullptr;

	std::unordered_map<uint32_t, GameObject*> instanceIdToGameObjectMap;

	GameObjectPtr removeInSubtree(GameObject* parent, GameObject* target);
};

