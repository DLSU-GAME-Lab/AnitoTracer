#include "ModelManager.h"

#include <iostream>
#include <glm/gtx/euler_angles.hpp>

#include "Debug.h"
#include "Utilities/FileUtils.h"
#include "HotkeySystem/HotkeySystem.hpp"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"
#include "StateManagement/ConcreteCommands/HierarchyCommands.hpp"
#include "Assets/GameObjectFactory.hpp"
#include "Engine/CameraSystem/CameraManager.h"
#include "Engine/CameraSystem/Camera.h"

ModelManager* ModelManager::sharedInstance = nullptr;

ModelManager* ModelManager::getInstance()
{
	return sharedInstance;
}

void ModelManager::initialize()
{
	sharedInstance = new ModelManager();
}

void ModelManager::destroy()
{
	sharedInstance->sceneGraph.clear();
	sharedInstance->lightList.clear();
	sharedInstance->objectGroupList.clear();

	delete sharedInstance;
}

ModelManager::ModelManager()
{
	HotkeySystem::getInstance()->addListener(this);
}

ModelManager::~ModelManager()
{
	HotkeySystem::getInstance()->removeListener(this);
}

std::vector<GameObject*> ModelManager::getAllObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->sceneGraph)
	{
		objectList.push_back(gameObject.get());

		auto descendants = gameObject->getChildrenRecursive();

		objectList.insert(objectList.end(), descendants.begin(), descendants.end());
	}

	return objectList;
}

std::vector<GameObject*> ModelManager::getAllActiveObjects() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->sceneGraph)
	{
		if (!gameObject->isActive()) continue;

		objectList.push_back(gameObject.get());

		auto descendants = gameObject->getChildrenRecursive();

		for(auto descendant : descendants)
		{
			if (descendant->isActive())
				objectList.push_back(descendant);
		}
	}

	return objectList;
}

std::vector<GameObject*> ModelManager::getSceneGraph() const
{
	std::vector<GameObject*> objectList;

	for (const auto& gameObject : this->sceneGraph)
	{
		objectList.push_back(gameObject.get());
	}

	return objectList;
}

int ModelManager::activeObjectsCount() const
{
	auto activeObjects = this->getAllActiveObjects();

	return static_cast<int>(activeObjects.size());
}

void ModelManager::addObject(ModelManager::GameObjectPtr gameObject)
{
	std::string message = "Added game object to root: " + gameObject->getName();
	Debug::Log(message);

	this->sceneGraph.push_back(std::move(gameObject));
}

void ModelManager::addObjectAtIndex(GameObjectPtr gameObject, int index)
{
	// Clamp index to valid range [0, sceneGraph.size()]
	size_t idx = 0;
	if (index > 0)
		idx = static_cast<size_t>(index);
	if (idx > this->sceneGraph.size()) idx = this->sceneGraph.size();

	std::string message = "Added game object to root: " + gameObject->getName() + " at index " + std::to_string(idx);
	Debug::Log(message);

	this->sceneGraph.insert(this->sceneGraph.begin() + idx, std::move(gameObject));
}

std::unique_ptr<GameObject> ModelManager::removeObject(GameObject* target)
{
	if (!target) return nullptr;

	for (auto it = this->sceneGraph.begin(); it != this->sceneGraph.end(); it++)
	{
		if (it->get() == target)
		{
			std::unique_ptr<GameObject> removed = std::move(*it);
			this->sceneGraph.erase(it);
			return removed;
		}
	
		std::unique_ptr<GameObject> result = removeInSubtree(it->get(), target);
		if (result)	return result;
	}

	return nullptr;
}

ModelManager::GameObjectPtr ModelManager::removeInSubtree(GameObject* parent, GameObject* target)
{
	for (auto it = parent->children.begin(); it != parent->children.end(); it++)
	{
		if (it->get() == target)
		{
			std::unique_ptr<GameObject> removed = std::move(*it);
			parent->children.erase(it);
			return removed;
		}

		std::unique_ptr<GameObject> result = removeInSubtree(it->get(), target);
		if (result)	return result;
	}

	return nullptr;
}

/* Only Searches Root */
void ModelManager::deleteObject(GameObject* gameObject)
{
	if (!gameObject) return;

	if (auto* lightPtr = dynamic_cast<Light*>(gameObject))
	{
		auto itLight = std::find(this->lightList.begin(), this->lightList.end(), lightPtr);
		if (itLight != this->lightList.end())
		{
			this->lightList.erase(itLight);
		}
	}

	auto it = std::find_if(this->sceneGraph.begin(), this->sceneGraph.end(),
		[gameObject](const GameObjectPtr& obj)
		{
			return obj.get() == gameObject;
		});
	if (it != this->sceneGraph.end())
	{
		this->sceneGraph.erase(it);
	}

}

ModelManager::GameObjectPtr ModelManager::CreateCopyOfObject(GameObject* original)
{
	auto copyObject = [&](GameObject* gameObject) -> std::unique_ptr<GameObject>
		{
			auto type = gameObject->getType();
			auto name = gameObject->getName();
			auto position = gameObject->getLocalPosition();
			auto rotation = gameObject->getLocalRotation();
			auto scale = gameObject->getLocalScale();
			auto active = gameObject->isActive();
			auto visible = gameObject->isVisible();
			auto pickable = gameObject->isPickable();
			auto parent = gameObject->getParent();
			auto material = gameObject->getModel()->getMaterial(0);

			// Copy Material
			std::shared_ptr<Assets::Material> copiedMat = std::make_shared<Assets::Material>();

			copiedMat->Diffuse = material->Diffuse;
			copiedMat->DiffuseTextureId = material->DiffuseTextureId;
			copiedMat->Fuzziness = material->Fuzziness;
			copiedMat->RefractionIndex = material->RefractionIndex;
			copiedMat->MaterialModel = material->MaterialModel;

			std::unique_ptr<GameObject> resultCopy;

			switch (type)
			{
			case GameObject::CUBE:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::SPHERE:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::PLANE:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::CYLINDER:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::CAPSULE:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::CORNELL_BOX:
				resultCopy = GameObjectFactory::getInstance()->CreatePrimitive(type, name);
				break;

			case GameObject::MESH:
				resultCopy = GameObjectFactory::getInstance()->CreateFromModelFile(original->getModel()->filepath, name);
				break;

			default:
				Debug::Log("[ERROR] unable to load game object!");
				return nullptr;
			}
			
			if (!resultCopy) return nullptr;
	
			resultCopy->setName(name);
			resultCopy->setLocalPosition(position);
			resultCopy->setLocalRotation(rotation);
			resultCopy->setLocalScale(scale);
			resultCopy->setActive(active);
			resultCopy->setVisible(visible);
			resultCopy->setPickable(pickable);
			resultCopy->setParent(parent);
			resultCopy->getModel()->SetMaterial(*copiedMat);

			return resultCopy;
		};

	auto parent = std::move(copyObject(original));

	for (const auto& child : original->getChildrenRecursive())
	{
		std::unique_ptr<GameObject> childCopy = copyObject(child);
		auto childCopyRef = childCopy.get();
		childCopyRef->getParent()->addChild(std::move(childCopy));
	}

	return std::move(parent);
}

void ModelManager::addLightObject(LightPtr lightObj)
{
	std::string message = "Added light to root: " + lightObj->getName();
	Debug::Log(message);

	this->lightList.push_back(lightObj.get());
	this->sceneGraph.push_back(std::move(lightObj));
}

ModelManager::LightPtr ModelManager::removeLightObject(Light* light)
{
	if (!light) return nullptr;

	// Remove raw pointer from lightList if present
	auto itLight = std::find(this->lightList.begin(), this->lightList.end(), light);
	if (itLight != this->lightList.end())
	{
		this->lightList.erase(itLight);
	}

	// Find the owning unique_ptr in sceneGraph
	auto it = std::find_if(this->sceneGraph.begin(), this->sceneGraph.end(),
		[light](const GameObjectPtr& obj)
		{
			return obj.get() == light;
		});

	if (it == this->sceneGraph.end()) return nullptr;

	// Ensure the object is a Light, then extract and return a unique_ptr<Light>
	if (dynamic_cast<Light*>(it->get()) == nullptr) return nullptr;

	GameObjectPtr ownedObj = std::move(*it);
	this->sceneGraph.erase(it);

	// Transfer ownership from unique_ptr<GameObject> to unique_ptr<Light>
	LightPtr result(static_cast<Light*>(ownedObj.release()));
	return result;
}

void ModelManager::setSelectedObject(GameObject* gameObject)
{
	this->selectedObject = gameObject;
}

GameObject* ModelManager::getSelectedObject()
{
	return this->selectedObject;
}

void ModelManager::clearAllObjects()
{
	this->sceneGraph.clear();
	this->objectGroupList.clear();
	this->lightList.clear();
}

/**
 * \brief Returns associated model representations of objects added.
 * \return
 */
std::vector<Assets::Model> ModelManager::getAllObjectModels() const
{
	ModelList modelList;

	for (const auto& gameObject : this->sceneGraph)
	{
		if (gameObject->isActive() && gameObject->isVisible())
		{
			gameObject->updateWorldMatrix();

			auto model = gameObject->getModel();

			if (model) // lights and emptyies have no models
				modelList.push_back(*model);

			auto descendants = gameObject->getChildrenRecursive();

			for (auto descendant : descendants)
			{
				if (descendant->isActive() && descendant->isVisible())
				{
					descendant->updateWorldMatrix();
					if (descendant->getModel())
						modelList.push_back(*descendant->getModel());
				}

			}
		}
	}

	return modelList;
}

ModelManager::LightPropsList ModelManager::getAllLightProperties() const
{
	LightPropsList lights;
	for (int i = 0; i < this->lightList.size(); i++)
	{
		if (this->lightList[i])
			lights.push_back(this->lightList[i]->Properties());
	}

	return lights;
}

int ModelManager::getObjectIndex(GameObject* gameObject) const
{
	if(gameObject == nullptr) return -1;

	for (size_t i = 0; i < this->sceneGraph.size(); i++)
	{
		if (this->sceneGraph[i].get() == gameObject)
			return static_cast<int>(i);
	}

	return -1;
}

int ModelManager::getSceneGraphRootSize() const
{
	return this->sceneGraph.size();
}

void ModelManager::createObject(GameObject::PrimitiveType type)
{
	switch (type) {
	case GameObject::CAMERA:
		break;
	case GameObject::CUBE:
	{
		Assets::Model cubeModel = Assets::Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::unique_ptr<GameObject> cube = std::make_unique<GameObject>("Cube", GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(cubeModel));
		addObject(std::move(cube));

		break;
	}
	case GameObject::OBJECT_GROUP:
		break;
	case GameObject::QUAD:
		break;
	case GameObject::SPHERE:
	{
		Assets::Model sphereModel = Assets::Model::CreateSphere(vec3(0), 50, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)), false);
		std::unique_ptr<GameObject> sphere = std::make_unique<GameObject>("Sphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Assets::Model>(sphereModel));
		addObject(std::move(sphere));
	}
	break;
	case GameObject::PLANE:
	{
		Assets::Model planeModel = Assets::Model::CreatePlane(vec3(0, 0, -100), vec3(100, -100, 0), *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::unique_ptr<GameObject> plane = std::make_unique<GameObject>("Plane", GameObject::PrimitiveType::PLANE, std::make_shared<Assets::Model>(planeModel));
		addObject(std::move(plane));
	}
	break;
	case GameObject::CYLINDER:
	{
		Assets::Model cylinderModel = Assets::Model::CreateCylinder(25, 50, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::unique_ptr<GameObject> cylinder = std::make_unique<GameObject>("Cylinder", GameObject::PrimitiveType::CYLINDER, std::make_shared<Assets::Model>(cylinderModel));
		addObject(std::move(cylinder));

	}
	break;
	case GameObject::CAPSULE:
	{
		Assets::Model capsuleModel = Assets::Model::CreateCapsule(25, 100, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::unique_ptr<GameObject> capsule = std::make_unique<GameObject>("Capsule", GameObject::PrimitiveType::CAPSULE, std::make_shared<Assets::Model>(capsuleModel));
		addObject(std::move(capsule));
	}
		break;
	case GameObject::POINT_LIGHT:
	{
		std::unique_ptr<Light> pl = std::make_unique<Light>("Light Source", Light::LightType::PointLight);
		addLightObject(std::move(pl));
	}
	break;
	case GameObject::DIRECTIONAL_LIGHT:
	{
		std::unique_ptr<Light> dl = std::make_unique<Light>("Light Source", Light::LightType::DirectionalLight);
		dl->setLocalRotation(-180, 0, 0);
		addLightObject(std::move(dl));
	}
	break;
	case GameObject::SPOT_LIGHT:
	{
		std::unique_ptr<Light> sl = std::make_unique<Light>("Light Source", Light::LightType::SpotLight);
		addLightObject(std::move(sl));
	}
		break;
	case GameObject::NONE:
		break;
	}
}

void ModelManager::createPrimitiveFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation,
	vec3 scale, std::vector<Assets::Material> mats)
{
	std::unique_ptr<GameObject> obj = nullptr;

	switch (type) {
		case GameObject::CUBE:
		{
			Assets::Model cubeModel = Assets::Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), mats[0]);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(cubeModel));
			break;
		}
		case GameObject::SPHERE:
		{
			Assets::Model sphereModel = Assets::Model::CreateSphere(vec3(0), 50, mats[0], false);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::SPHERE, std::make_shared<Assets::Model>(sphereModel));
			break;
		}
		case GameObject::PLANE:
		{
			Assets::Model planeModel = Assets::Model::CreatePlane(vec3(0, 0, -100), vec3(100, -100, 0), mats[0]);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::PLANE, std::make_shared<Assets::Model>(planeModel));
			break;
		}
		case GameObject::CYLINDER:
		{
			Assets::Model cylinderModel = Assets::Model::CreateCylinder(25, 50, mats[0]);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::CYLINDER, std::make_shared<Assets::Model>(cylinderModel));
			break;
		}
		case GameObject::CAPSULE:
		{
			Assets::Model capsuleModel = Assets::Model::CreateCapsule(25, 100, mats[0]);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::CAPSULE, std::make_shared<Assets::Model>(capsuleModel));
			break;
		}
		case GameObject::CORNELL_BOX:
		{
			Assets::Model cornellBoxModel = Assets::Model::CreateCornellBox(555);
			obj = std::make_unique<GameObject>(name, GameObject::PrimitiveType::CORNELL_BOX, std::make_shared<Assets::Model>(cornellBoxModel));
			break;
		}

		default: break;
	}

	if (obj)
	{
		obj->setLocalPosition(position);
		obj->setLocalRotation(rotation);
		obj->setLocalScale(scale);
		obj->setActive(active);
		addObject(std::move(obj));
	}
}

void ModelManager::createLightFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position,
	vec3 rotation, vec3 scale, std::vector<Assets::Material> mats, Assets::LightProperties props)
{
	std::unique_ptr<Light> light = nullptr;

	switch (type) {
		case GameObject::POINT_LIGHT:
		{
			light = std::make_unique<Light>(name.c_str(), Light::LightType::PointLight,
											position, props.AmbientColor, props.LightColor);
			break;
		}
		case GameObject::DIRECTIONAL_LIGHT:
		{
			light = std::make_unique<Light>(name.c_str(), Light::LightType::DirectionalLight,
											position, props.AmbientColor, props.LightColor);
			break;
		}
		case GameObject::SPOT_LIGHT:
		{
			light = std::make_unique<Light>(name.c_str(), Light::LightType::SpotLight,
											position, props.AmbientColor, props.LightColor);
			break;
		}
		default: break;
	}

	if (light)
	{
		light->setName(name);
		light->setLocalPosition(position);
		light->setLocalRotation(rotation);
		light->setLocalScale(scale);
		light->setActive(active);
		addLightObject(std::move(light));
	}
}

void ModelManager::createObjectFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation,
                                        vec3 scale)
{
	std::string meshFilePath;
	std::string fileName;

	if (!FileUtils::getModelFilePath(meshFilePath, fileName))
	{
		Debug::Log("Cancelled loading OBJ from path: " + meshFilePath);

		return;
	}

	if (!meshFilePath.empty()) {
		Debug::Log("Loading OBJ from path: " + meshFilePath);
	}

	auto model = Assets::Model::LoadModel(meshFilePath);
	std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(name, type, std::make_shared<Assets::Model>(model));
	gameObject->setLocalPosition(position);
	gameObject->setLocalRotation(rotation);
	gameObject->setLocalScale(scale);
	addObject(std::move(gameObject));
}

void ModelManager::createObjectGroupFromFile(String name, GameObject::PrimitiveType type, vec3 position, vec3 rotation, vec3 scale)
{
	std::string meshFilePath;
	std::string fileName;

	if (!FileUtils::getModelFilePath(meshFilePath, fileName))
	{
		Debug::Log("Cancelled loading OBJ from path: " + meshFilePath);

		return;
	}

	if (!meshFilePath.empty()) {
		Debug::Log("Loading OBJ from path: " + meshFilePath);
	}

	// load all models of the group into a list
	std::vector<Assets::Model> models = Assets::Model::LoadModelGroup(meshFilePath);
	
	//create a game object for each model
	for (int i = 0; i < models.size(); i++) 
	{
		std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(name + "_1", type, std::make_shared<Assets::Model>(models[i]));
		gameObject->setLocalPosition(position);
		gameObject->setLocalRotation(rotation);
		gameObject->setLocalScale(scale);
		addObject(std::move(gameObject));
	}
}

void ModelManager::createSponza()
{
	std::vector<Assets::Model> models = Assets::Model::LoadModelGroup(FileUtils::getAssetsFolderPath().generic_string() + "/models/sponza.obj");

	//create a game object for each model
	for (int i = 0; i < models.size(); i++)
	{
		std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>("Sponza " + i, GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(models[i]));
		gameObject->setLocalPosition(0,0,0);
		gameObject->setLocalRotation(0,0,0);
		gameObject->setLocalScale(1,1,1);
		addObject(std::move(gameObject));
	}
}

void ModelManager::OnActionPressed(Hotkey::Action action)
{
	/* paste only needs valid copied object */
	if (action == Hotkey::Action::GameObject_Paste)	PasteObject();

	if (!this->selectedObject) return; // all actions involve selected object

	if (action == Hotkey::Action::GameObject_ToggleActive)
	{
		auto currentState = this->selectedObject->isActive();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->setActive(std::get<bool>(v)); },
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
		auto parent = this->selectedObject->getParent();

		CommandManager::getInstance()->executeCommand(
			new ReparentCommand(
				this->selectedObject,
				parent,
				parent ? parent->getChildIndex(this->selectedObject) : this->getObjectIndex(this->selectedObject),
				parent,
				0
			)
		);
	}

	if (action == Hotkey::Action::GameObject_SetAsLastSibling)
	{
		auto parent = this->selectedObject->getParent();

		CommandManager::getInstance()->executeCommand(
			new ReparentCommand(
				this->selectedObject,
				parent,
				parent ? parent->getChildIndex(this->selectedObject) : this->getObjectIndex(this->selectedObject),
				parent,
				parent ? parent->getChildren().size() : this->getSceneGraphRootSize()
			)
		);
	}

	if (action == Hotkey::Action::GameObject_TogglePickabilityWithDescendants)
	{
		auto currentState = this->selectedObject->isPickable();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->setPickable(std::get<bool>(v)); },
				currentState,
				!currentState
			));
	}

	if (action == Hotkey::Action::GameObject_ToggleVisibilityWithDescendants)
	{
		auto currentState = this->selectedObject->isVisible();

		CommandManager::getInstance()->executeCommand(
			new AlterTransformCommand(
				this->selectedObject,
				[](GameObject* g, AlterTransformCommand::Variant v) { g->setVisible(std::get<bool>(v)); },
				currentState,
				!currentState
			));

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

void ModelManager::CutSelectedObject()
{
	this->copiedObject = ModelManager::getInstance()->CreateCopyOfObject(this->selectedObject);
	CommandManager::getInstance()->executeCommand(
		new DeleteObjectCommand(this->selectedObject)
	);
	this->selectedObject = nullptr;
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void ModelManager::CopySelectedObject()
{
	this->copiedObject = ModelManager::getInstance()->CreateCopyOfObject(this->selectedObject);
}

void ModelManager::DuplicateSelectedObject()
{
	auto duplicate = ModelManager::getInstance()->CreateCopyOfObject(this->selectedObject);

	glm::vec3 offset = { 10.0f, 10.0f, 10.0 }; //offset spawn
	duplicate->setLocalPosition(this->selectedObject->getLocalPosition() + offset);

	CommandManager::getInstance()->executeCommand(
		new AddObjectCommand(std::move(duplicate))
	);

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void ModelManager::DeleteSelectedObject()
{
	CommandManager::getInstance()->executeCommand(
		new DeleteObjectCommand(this->selectedObject)
	);
	this->selectedObject = nullptr;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); //AnitoTracer Specific
}

void ModelManager::ClearInstanceToObjectMap()
{
	this->instanceIdToGameObjectMap.clear();
}

void ModelManager::RegisterInstance(uint32_t instanceId, GameObject* gameObject)
{
	this->instanceIdToGameObjectMap[instanceId] = gameObject;
}

GameObject* ModelManager::FindGameObject(uint32_t instanceId) const
{
	auto it = this->instanceIdToGameObjectMap.find(instanceId);
	return (it != this->instanceIdToGameObjectMap.end()) ? it->second : nullptr;
}

/* Where the object is spawned needs to be decided  (world origin vs infront of camera vs beside copy) */
void ModelManager::PasteObject()
{
	if (!this->copiedObject) return;

	auto sceneCamera = CameraManager::getInstance()->getActiveCamera();

	this->copiedObject->setLocalPosition(sceneCamera->getForward() * 500.0f);

	CommandManager::getInstance()->executeCommand(
		new AddObjectCommand(std::move(this->copiedObject))
	);

	this->copiedObject = nullptr;

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}




