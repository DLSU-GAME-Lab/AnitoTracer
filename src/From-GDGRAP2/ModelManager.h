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

	typedef std::vector<std::shared_ptr<ObjectGroup>> ObjectGroupList;

	using ModelList = std::vector<Assets::Model>;
	typedef std::vector<Assets::LightProperties> LightPropsList;

	static ModelManager* getInstance();
	static void initialize();
	static void destroy();

	std::vector<GameObject*> getAllObjects() const;
	std::vector<GameObject*> getAllActiveObjects() const;
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

	ModelList getAllObjectModels() const;
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

private:
	ModelManager();
	~ModelManager();
	ModelManager(ModelManager const&) {};             // copy constructor is private
	ModelManager& operator=(ModelManager const&) {};  // assignment operator is private*/
	static ModelManager* sharedInstance;

	GameObjectList sceneGraph;

	ObjectGroupList objectGroupList;
	LightList lightList;

	GameObject* selectedObject = nullptr;
	GameObjectPtr copiedObject = nullptr;

	GameObjectPtr removeInSubtree(GameObject* parent, GameObject* target);
};

