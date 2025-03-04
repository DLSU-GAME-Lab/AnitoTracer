#include "ModelManager.h"

#include <iostream>
#include <glm/gtx/euler_angles.hpp>

#include "Debug.h"
#include "Utilities/FileUtils.h"

uint32_t ModelManager::nextID = 0;

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
		std::cout << "Object " << name << " not found!";
		return nullptr;
	}
}

std::shared_ptr<Light> ModelManager::findLightObjectByName(String name)
{
	if (this->lightTable[name] != nullptr) {
		return this->lightTable[name];
	}
	else {
		std::cout << "Object " << name << " not found!";
		return nullptr;
	}
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

/**
 * \brief Returns associated model representations of objects added.
 * \return
 */
ModelManager::ModelList ModelManager::getAllObjectModels() const
{
	ModelList models;
	for (int i = 0; i < this->gameObjectList.size(); i++)
	{
		if (this->gameObjectList[i]->getModel())
			models.push_back(*this->gameObjectList[i]->getModel());
	}

	for (int i = 0; i < this->objectGroupList.size(); i++)
	{
		for (int j = 0; j < this->objectGroupList[i]->getSize(); j++)
		{
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
	gameObject->setID(nextID++);
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
	std::cout << "Added game object in manager: " + gameObject->getName() << std::endl;
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
	std::cout << "Added object group in manager: " + objectGroup->getName() << std::endl;
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

std::shared_ptr<GameObject> ModelManager::getSelectedObject()
{
	return this->selectedObject;
}

void ModelManager::clearAllObjects()
{
	this->gameObjectList.clear();
	this->gameObjectMap.clear();
	this->objectGroupList.clear();
	this->lightList.clear();
	this->lightTable.clear();
}
