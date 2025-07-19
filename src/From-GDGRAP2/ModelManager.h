#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ObjectGroup.h"
#include "Assets/Texture.hpp"
#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"

/**
 * \brief Similar to the game object manager, this class stores model instances
 */
class ModelManager
{
public:
	typedef glm::vec3 vec3;
	typedef std::string String;
	typedef std::vector<std::shared_ptr<GameObject>> List;
	typedef std::vector<std::shared_ptr<Light>> LightList;
	typedef std::unordered_map<std::string, std::shared_ptr<Light>> LightTable;
	typedef std::unordered_map<String, std::shared_ptr<GameObject>> HashTable;

	typedef std::vector<std::shared_ptr<ObjectGroup>> ObjectGroupList;

	typedef std::vector<Assets::Model> ModelList;
	typedef std::vector<Assets::LightProperties> LightPropsList;

	static ModelManager* getInstance();
	static void initialize();
	static void destroy();

	std::shared_ptr<GameObject> findObjectByName(String name);
	std::shared_ptr<Light> findLightObjectByName(String name);
	List getAllObjects() const;
	
	int activeObjects() const;
	std::shared_ptr<GameObject> getLastObject();
	void addLightObject(std::shared_ptr<Light> lightObj);
	void addObject(std::shared_ptr<GameObject> gameObject);
	void addObject(std::shared_ptr<ObjectGroup> objectGroup);
	void createObject(GameObject::PrimitiveType type);
	void createPrimitiveFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation, vec3 scale, std::vector<Assets::Material> mats);
	void createLightFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation, vec3 scale, std::vector<Assets::Material> mats, Assets::LightProperties props);
	void createObjectFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation, vec3 scale); 
	void createObjectGroupFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation, vec3 scale);
	void createSponza();
	void deleteObject(std::shared_ptr<GameObject> gameObject);
	void deleteObjectByName(String name);
	void setSelectedObject(String name);
	void setSelectedObject(std::shared_ptr<GameObject> gameObject);
	std::shared_ptr<GameObject> getSelectedObject();

	void clearAllObjects();

	ModelList getAllObjectModels() const;
	LightPropsList getAllLightProperties() const;

private:
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(ModelManager const&) {};             // copy constructor is private
	ModelManager& operator=(ModelManager const&) {};  // assignment operator is private*/
	static ModelManager* sharedInstance;

	HashTable gameObjectMap;
	List gameObjectList;
	ObjectGroupList objectGroupList;
	LightList lightList;
	LightTable lightTable;

	std::shared_ptr<GameObject> selectedObject = nullptr;

};

