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
	sharedInstance->gameObjectMap.clear();
	sharedInstance->gameObjectList.clear();
	sharedInstance->lightList.clear();
	delete sharedInstance;
}

std::shared_ptr<GameObject> ModelManager::findObjectByName(String name)
{
	if (this->gameObjectMap[name] != nullptr) {
		return this->gameObjectMap[name];
	}
	else {
		return nullptr;
	}
}

std::shared_ptr<Light> ModelManager::findLightObjectByName(String name)
{
	if (this->lightTable[name] != nullptr) {
		return this->lightTable[name];
	}
	else {
		return nullptr;
	}
}

ModelManager::List ModelManager::getAllPickableObjects() const
{
	ModelManager::List objectList;
	for (int i = 0; i < this->gameObjectList.size(); i++)
	{
		if(this->gameObjectList[i]->isPickable())
			objectList.push_back(this->gameObjectList[i]);
	}

	for (int i = 0; i < this->objectGroupList.size(); i++)
	{
		if (this->objectGroupList[i]->isPickable())
			objectList.push_back(this->objectGroupList[i]);
	}

	return objectList;
}

ModelManager::List ModelManager::getAllObjects() const
{
	ModelManager::List objectList;
	for (int i = 0; i < this->gameObjectList.size(); i++)
	{
		objectList.push_back(this->gameObjectList[i]);
	}

	for (int i = 0; i < this->objectGroupList.size(); i++)
	{
		objectList.push_back(this->objectGroupList[i]);
	}

	return objectList;
}

//brief Returns associated model representations of objects added.
//return

ModelManager::ModelList ModelManager::getAllObjectModels() const
{
	ModelList models;
	for (int i = 0; i < this->gameObjectList.size(); i++)
	{
		if (this->gameObjectList[i]->getModel() && this->gameObjectList[i]->isActive() && this->gameObjectList[i]->isVisible())
			models.push_back(*this->gameObjectList[i]->getModel());
	}

	for (int i = 0; i < this->objectGroupList.size(); i++)
	{
		for (int j = 0; j < this->objectGroupList[i]->getSize(); j++)
		{
			if(this->objectGroupList[i]->isActive() && this->objectGroupList[i]->isVisible())
				models.push_back(*this->objectGroupList[i]->getModelAt(j));
		}
	}

	return models;
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

int ModelManager::activeObjects() const
{
	return this->gameObjectList.size();
}

std::shared_ptr<GameObject> ModelManager::getLastObject()
{
	return this->gameObjectList[this->activeObjects() - 1];
}

void ModelManager::addLightObject(std::shared_ptr<Light> lightObj)
{
	this->lightList.push_back(lightObj);
	this->lightTable[lightObj->getName()] = lightObj;

	this->addObject(lightObj);
}

void ModelManager::addObject(std::shared_ptr<GameObject> gameObject)
{
	if (this->gameObjectMap[gameObject->getName()] != nullptr) {
		int count = 1;
		String revisedString = gameObject->getName() + " " + "(" + std::to_string(count) + ")";
		while (this->gameObjectMap[revisedString] != nullptr) {
			count++;
			revisedString = gameObject->getName() + " " + "(" + std::to_string(count) + ")";
		}
		gameObject->name = revisedString;
		this->gameObjectMap[revisedString] = gameObject;
	}
	else {
		this->gameObjectMap[gameObject->getName()] = gameObject;
	}
	this->gameObjectList.push_back(gameObject);

	std::string message = "Added game object in manager: " + gameObject->getName();
	Debug::Log(message);
}

void ModelManager::addObject(std::shared_ptr<ObjectGroup> objectGroup)
{
	if (this->gameObjectMap[objectGroup->getName()] != nullptr) {
		int count = 1;
		String revisedString = objectGroup->getName() + " " + "(" + std::to_string(count) + ")";
		while (this->gameObjectMap[revisedString] != nullptr) {
			count++;
			revisedString = objectGroup->getName() + " " + "(" + std::to_string(count) + ")";
		}
		objectGroup->name = revisedString;
		this->gameObjectMap[revisedString] = objectGroup;
	}
	else {
		this->gameObjectMap[objectGroup->getName()] = objectGroup;
	}
	this->objectGroupList.push_back(objectGroup);

	std::string message = "Added object group in manager: " + objectGroup->getName();
	Debug::Log(message);
}

void ModelManager::createObject(GameObject::PrimitiveType type)
{
	switch (type) {
	case GameObject::CAMERA:
		break;
	case GameObject::CUBE:
	{
		Assets::Model cubeModel = Assets::Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::shared_ptr<GameObject> cube = std::make_shared<GameObject>("Cube", GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(cubeModel));
		addObject(cube);

		break;
	}
	case GameObject::OBJECT_GROUP:
		break;
	case GameObject::QUAD:
		break;
	case GameObject::SPHERE:
	{
		Assets::Model sphereModel = Assets::Model::CreateSphere(vec3(0), 50, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)), false);
		std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>("Sphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Assets::Model>(sphereModel));
		addObject(sphere);
	}
	break;
	case GameObject::PLANE:
	{
		Assets::Model planeModel = Assets::Model::CreatePlane(vec3(0, 0, -100), vec3(100, -100, 0), *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::shared_ptr<GameObject> plane = std::make_shared<GameObject>("Plane", GameObject::PrimitiveType::PLANE, std::make_shared<Assets::Model>(planeModel));
		addObject(plane);
	}
	break;
	case GameObject::CYLINDER:
	{
		Assets::Model cylinderModel = Assets::Model::CreateCylinder(25, 50, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::shared_ptr<GameObject> cylinder = std::make_shared<GameObject>("Cylinder", GameObject::PrimitiveType::CYLINDER, std::make_shared<Assets::Model>(cylinderModel));
		addObject(cylinder);

	}
	break;
	case GameObject::CAPSULE:
	{
		Assets::Model capsuleModel = Assets::Model::CreateCapsule(25, 100, *Assets::Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)));
		std::shared_ptr<GameObject> capsule = std::make_shared<GameObject>("Capsule", GameObject::PrimitiveType::CAPSULE, std::make_shared<Assets::Model>(capsuleModel));
		addObject(capsule);
	}
		break;
	case GameObject::POINT_LIGHT:
	{
		std::shared_ptr<Light> pl = std::make_shared<Light>("Light Source", Light::LightType::PointLight);
		addLightObject(pl);
	}
	break;
	case GameObject::DIRECTIONAL_LIGHT:
	{
		std::shared_ptr<Light> dl = std::make_shared<Light>("Light Source", Light::LightType::DirectionalLight);
		dl->setLocalRotation(-180, 0, 0);
		addLightObject(dl);
	}
	break;
	case GameObject::SPOT_LIGHT:
	{
		std::shared_ptr<Light> sl = std::make_shared<Light>("Light Source", Light::LightType::SpotLight);
		addLightObject(sl);
	}
		break;
	case GameObject::NONE:
		break;
	}
}

void ModelManager::createPrimitiveFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position, vec3 rotation,
	vec3 scale, std::vector<Assets::Material> mats)
{
	std::shared_ptr<GameObject> obj = nullptr;

	switch (type) {
		case GameObject::CUBE:
		{
			Assets::Model cubeModel = Assets::Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), mats[0]);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(cubeModel));
			break;
		}
		case GameObject::SPHERE:
		{
			Assets::Model sphereModel = Assets::Model::CreateSphere(vec3(0), 50, mats[0], false);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::SPHERE, std::make_shared<Assets::Model>(sphereModel));
			break;
		}
		case GameObject::PLANE:
		{
			Assets::Model planeModel = Assets::Model::CreatePlane(vec3(0, 0, -100), vec3(100, -100, 0), mats[0]);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::PLANE, std::make_shared<Assets::Model>(planeModel));
			break;
		}
		case GameObject::CYLINDER:
		{
			Assets::Model cylinderModel = Assets::Model::CreateCylinder(25, 50, mats[0]);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CYLINDER, std::make_shared<Assets::Model>(cylinderModel));
			break;
		}
		case GameObject::CAPSULE:
		{
			Assets::Model capsuleModel = Assets::Model::CreateCapsule(25, 100, mats[0]);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CAPSULE, std::make_shared<Assets::Model>(capsuleModel));
			break;
		}
		case GameObject::CORNELL_BOX:
		{
			Assets::Model cornellBoxModel = Assets::Model::CreateCornellBox(555);
			obj = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CORNELL_BOX, std::make_shared<Assets::Model>(cornellBoxModel));
			break;
		}

		default: break;
	}

	if (obj)
	{
		addObject(obj);
		obj->setLocalPosition(position);
		obj->setLocalRotation(rotation);
		obj->setLocalScale(scale);
		obj->setActive(active);
	}
}

void ModelManager::createLightFromScene(String name, GameObject::PrimitiveType type, bool active, vec3 position,
	vec3 rotation, vec3 scale, std::vector<Assets::Material> mats, Assets::LightProperties props)
{
	std::shared_ptr<Light> light = nullptr;

	switch (type) {
		case GameObject::POINT_LIGHT:
		{
			light = std::make_shared<Light>(name.c_str(), Light::LightType::PointLight, 
											position, props.AmbientColor, props.LightColor);
			break;
		}
		case GameObject::DIRECTIONAL_LIGHT:
		{
			light = std::make_shared<Light>(name.c_str(), Light::LightType::DirectionalLight,
											position, props.AmbientColor, props.LightColor);
			break;
		}
		case GameObject::SPOT_LIGHT:
		{
			light = std::make_shared<Light>(name.c_str(), Light::LightType::SpotLight,
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
		addLightObject(light);
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
	std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(name, type, std::make_shared<Assets::Model>(model));
	gameObject->setLocalPosition(position);
	gameObject->setLocalRotation(rotation);
	gameObject->setLocalScale(scale);
	addObject(gameObject);
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
		std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(name + "_1", type, std::make_shared<Assets::Model>(models[i]));
		gameObject->setLocalPosition(position);
		gameObject->setLocalRotation(rotation);
		gameObject->setLocalScale(scale);
		addObject(gameObject);
	}

}

void ModelManager::createSponza()
{
	std::vector<Assets::Model> models = Assets::Model::LoadModelGroup(FileUtils::getAssetsFolderPath().generic_string() + "/models/sponza.obj");

	//create a game object for each model
	for (int i = 0; i < models.size(); i++)
	{
		std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>("Sponza " + i, GameObject::PrimitiveType::CUBE, std::make_shared<Assets::Model>(models[i]));
		gameObject->setLocalPosition(0,0,0);
		gameObject->setLocalRotation(0,0,0);
		gameObject->setLocalScale(1,1,1);
		addObject(gameObject);
	}
}

void ModelManager::deleteObject(std::shared_ptr<GameObject> gameObject)
{
	if (gameObject->getType() == GameObject::POINT_LIGHT || gameObject->getType() == GameObject::DIRECTIONAL_LIGHT || gameObject->getType() == GameObject::SPOT_LIGHT)
		this->lightTable.erase(gameObject->getName());

	this->gameObjectMap.erase(gameObject->getName());

	int index = -1;
	for (int i = 0; i < this->gameObjectList.size(); i++) {
		if (this->gameObjectList[i] == gameObject) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		this->gameObjectList.erase(this->gameObjectList.begin() + index);
	}

	index = -1;
	for (int i = 0; i < this->lightList.size(); i++) {
		if (this->lightList[i] == gameObject) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		this->lightList.erase(this->lightList.begin() + index);
	}
}

void ModelManager::deleteObjectByName(String name)
{
	std::shared_ptr<GameObject> object = this->findObjectByName(name);

	if (object != nullptr)
	{
		this->deleteObject(object);
	}
}

void ModelManager::setSelectedObject(String name)
{
	if (this->gameObjectMap[name] != nullptr) {
		this->setSelectedObject(this->gameObjectMap[name]);
	}
}

void ModelManager::setSelectedObject(std::shared_ptr<GameObject> gameObject)
{
	this->selectedObject = gameObject;
}

void ModelManager::clearSelectedObject()
{
	this->selectedObject = nullptr;
}

std::shared_ptr<GameObject> ModelManager::getSelectedObject()
{
	return this->selectedObject;
}

std::vector<std::shared_ptr<GameObject>> ModelManager::createDuplicateObject(std::shared_ptr<GameObject> gameObject)
{
	auto copyObject = [&](GameObject* gameObject) -> std::shared_ptr<GameObject>
		{
			auto type = gameObject->getType();
			auto name = gameObject->getName();
			auto position = gameObject->getLocalPosition();
			auto rotation = gameObject->getLocalRotation();
			auto scale = gameObject->getLocalScale();
			auto active = gameObject->isActive();
			auto parent = gameObject->getParent();
			auto material = gameObject->getModel()->getMaterial(0);

			// Copy Material
			std::shared_ptr<Assets::Material> copiedMat = std::make_shared<Assets::Material>();

			copiedMat->Diffuse = material->Diffuse;
			copiedMat->DiffuseTextureId = material->DiffuseTextureId;
			copiedMat->Fuzziness = material->Fuzziness;
			copiedMat->RefractionIndex = material->RefractionIndex;
			copiedMat->MaterialModel = material->MaterialModel;

			std::shared_ptr<GameObject> resultCopy;

			switch (type)
			{
			case GameObject::CUBE:
				{
					Assets::Model model = Assets::Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *copiedMat);
					resultCopy = std::make_shared<GameObject>(name, type, std::make_shared<Assets::Model>(model));
					break;
				}

			case GameObject::SPHERE:
				{
					Assets::Model model = Assets::Model::CreateSphere(vec3(0), 50, *copiedMat, false);
					resultCopy = std::make_shared<GameObject>(name, GameObject::PrimitiveType::SPHERE, std::make_shared<Assets::Model>(model));
					break;
				}

			case GameObject::PLANE:
				{
					Assets::Model model = Assets::Model::CreatePlane(vec3(0, 0, -100), vec3(100, -100, 0), *copiedMat);
					resultCopy = std::make_shared<GameObject>(name, GameObject::PrimitiveType::PLANE, std::make_shared<Assets::Model>(model));
					break;
				}

			case GameObject::CYLINDER:
				{
					Assets::Model model = Assets::Model::CreateCylinder(25, 50, *copiedMat);
					resultCopy = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CYLINDER, std::make_shared<Assets::Model>(model));
					break;
				}

			case GameObject::CAPSULE:
				{
					Assets::Model model = Assets::Model::CreateCapsule(25, 100, *copiedMat);
					resultCopy = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CAPSULE, std::make_shared<Assets::Model>(model));
					break;
				}

			case GameObject::CORNELL_BOX:
				{
					Assets::Model model = Assets::Model::CreateCornellBox(555);
					resultCopy = std::make_shared<GameObject>(name, GameObject::PrimitiveType::CORNELL_BOX, std::make_shared<Assets::Model>(model));
					break;
				}

			default:
				Debug::Log("[ERROR] unable to load custom models!");
				return nullptr;
			}

			resultCopy->setLocalPosition(position);
			resultCopy->setLocalRotation(rotation);
			resultCopy->setLocalScale(scale);
			resultCopy->setParent(parent);

			return resultCopy;
		};

	std::vector<std::shared_ptr<GameObject>> result;

	auto parent = copyObject(gameObject.get());
	result.push_back(parent);

	for (const auto& child : gameObject->getChildrenRecursive())
	{
		std::shared_ptr<GameObject> childCopy = copyObject(child);
		parent->addChild(childCopy.get());
		result.push_back(childCopy);
	}

	return result;
}

void ModelManager::setCopiedObject(std::vector<std::shared_ptr<GameObject>> gameObjectList)
{
	this->copiedObject = gameObjectList;
}

std::vector<std::shared_ptr<GameObject>> ModelManager::getCopiedObject()
{
	return this->copiedObject;
}

void ModelManager::clearAllObjects()
{
	this->gameObjectList.clear();
	this->gameObjectMap.clear();
	this->objectGroupList.clear();
	this->lightList.clear();
	this->lightTable.clear();
}
