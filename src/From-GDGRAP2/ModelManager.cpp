#include "ModelManager.h"

#include <iostream>
#include <glm/gtx/euler_angles.hpp>

#include "Debug.h"
#include "Utilities/FileUtils.h"

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

/* Can be used for deletes, but might leave invalid pointers */
std::unique_ptr<GameObject> ModelManager::removeObject(GameObject* gameObject)
{
	if (!gameObject) return nullptr;

	auto found = std::find_if(sceneGraph.begin(), sceneGraph.end(),
		[gameObject](const GameObjectPtr& child)
		{
			return child.get() == gameObject;
		});

	if (found == sceneGraph.end())	return nullptr;

	GameObjectPtr result = std::move(*found);

	sceneGraph.erase(found);

	return result;
}

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

void ModelManager::addLightObject(LightPtr lightObj)
{
	std::string message = "Added light to root: " + lightObj->getName();
	Debug::Log(message);

	this->lightList.push_back(lightObj.get());
	this->sceneGraph.push_back(std::move(lightObj));
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
		if (!gameObject->isActive()) continue;

		gameObject->updateWorldMatrix();
		modelList.push_back(*gameObject->getModel().get());

		auto descendants = gameObject->getChildrenRecursive();

		for (auto descendant : descendants)
		{
			if (descendant->isActive())
				modelList.push_back(*descendant->getModel().get());
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




