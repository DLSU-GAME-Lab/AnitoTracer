#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"
#include "HotkeySystem/HotkeyListener.hpp"

#include "Vulkan/Buffer.hpp"
#include "Vulkan/DeviceMemory.hpp"

/**
 * \brief Similar to the game object manager, this class stores model instances
 */
class ModelManager : public HotkeyListener
{
public:
	struct InstancePair
	{
		GameObject* obj;
		VkAccelerationStructureInstanceKHR instance;
	};

	using vec3 = glm::vec3;
	using String = std::string;

	using GameObjectPtr = std::unique_ptr<GameObject>;
	using GameObjectList = std::vector<GameObjectPtr>;
	using GameObjectMap = std::unordered_map<uint32_t, GameObject*>;
	using LightPtr = std::unique_ptr<Light>;
	using LightList = std::vector<Light*>;
	using TLASInstanceMap = std::unordered_map<uint32_t, InstancePair>;
	using DirtyInstancesMap = std::unordered_map<uint32_t, VkAccelerationStructureInstanceKHR>;

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
	void registerIfLight(GameObject* obj);
	void unregisterIfLight(GameObject* obj);
	void registerSubtree(GameObject* root);
	void unregisterSubtree(GameObject* root);
	GameObjectPtr removeObject(GameObject* target);

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

	void OnActionPressed(Hotkey::Action action) override;

	/* For Hierarchy Actions and Scene View Menu */
	void PasteObject();
	void CutSelectedObject();
	void CopySelectedObject();
	void DuplicateSelectedObject();
	void DeleteSelectedObject();

	GameObject* findObjectByID(uint32_t id) const;

	void ClearTLASInstances();
	void RegisterTLASInstance(uint32_t objectId, GameObject* obj, VkAccelerationStructureInstanceKHR instance);
	std::vector<VkAccelerationStructureInstanceKHR> GetTLASInstances(Vulkan::CommandPool& commandPool);

	VkBuffer GetDirtyInstancesBuffer() const;

	uint32_t GetDirtyInstancesCount() { return static_cast<uint32_t>(dirtyInstanceIds.size()); }

private:
	ModelManager();
	~ModelManager();
	ModelManager(ModelManager const&) {};             // copy constructor is private
	ModelManager& operator=(ModelManager const&) {};  // assignment operator is private*/
	static ModelManager* sharedInstance;

	GameObjectList sceneGraph;
	LightList lightList;

	GameObjectMap gameObjectMap;
	TLASInstanceMap tlasInstanceMap;

	GameObject* selectedObject = nullptr;
	GameObjectPtr copiedObject = nullptr;

	std::vector<uint32_t> dirtyInstanceIds;
	std::unique_ptr<Vulkan::Buffer> dirtyInstancesBuffer;
	std::unique_ptr<Vulkan::DeviceMemory> dirtyInstancesMemory;

	GameObjectPtr removeInSubtree(GameObject* parent, GameObject* target);
};

