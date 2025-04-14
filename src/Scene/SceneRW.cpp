#include "SceneRW.h"

#include <fstream>
#include <filesystem>
#include "Utilities/FileUtils.h"
#include "jsoncpp/json/json.h"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"

#include "Utilities/Glm.hpp"

using namespace glm;

SceneRW::SceneRW()
{
}

SceneRW::~SceneRW()
{
}

void SceneRW::exportScene()
{
	//std::string fileDirectory;
	//std::string fileName;
 //
	//FileUtils::getScenePath(fileDirectory, fileName);

	//if (fileDirectory.find(".level") != std::string::npos) //check if .level or .txt
	//{

	//}

	//std::fstream sceneFile;
	//sceneFile.open(fileDirectory, std::ios::out);

	//Logger::log("Selected File Name : " + fileDirectory);

	//Json::Value root;

	//GameObjectManager::GameObjectList objectList = GameObjectManager::getInstance()->getAllObjects();

	//for (AGameObject* gameObject : objectList)
	//{
	//	std::string guid = gameObject->getGuidString();

	//	root[guid];
	//	root[guid]["name"] = gameObject->getName();
	//	root[guid]["type"] = gameObject->getType();

	//	Vector3D position = gameObject->getLocalPosition();
	//	Vector3D rotation = gameObject->getLocalRotation();
	//	Vector3D scale = gameObject->getLocalScale();

	//	root[guid]["position"]["x"] = position.x;
	//	root[guid]["position"]["y"] = position.y;
	//	root[guid]["position"]["z"] = position.z;

	//	root[guid]["rotation"]["x"] = rotation.x;
	//	root[guid]["rotation"]["y"] = rotation.y;
	//	root[guid]["rotation"]["z"] = rotation.z;

	//	root[guid]["scale"]["x"] = scale.x;
	//	root[guid]["scale"]["y"] = scale.y;
	//	root[guid]["scale"]["z"] = scale.z;

	//	AGameObject::ComponentList physicsList = gameObject->getComponentsOfType(AComponent::ComponentType::Physics);
	//	for (AComponent* component : physicsList)
	//	{
	//		PhysicsComponent* physicsComponent = dynamic_cast<PhysicsComponent*>(component);
	//		std::string componentGuid = component->getGuidString();

	//		root[guid]["components"][componentGuid];
	//		root[guid]["components"][componentGuid]["name"] = physicsComponent->getName();
	//		root[guid]["components"][componentGuid]["class"] = physicsComponent->getClassType();
	//		root[guid]["components"][componentGuid]["type"] = physicsComponent->getType();

	//		root[guid]["components"][componentGuid]["mass"] = physicsComponent->getMass();
	//		root[guid]["components"][componentGuid]["gravity"] = physicsComponent->getUseGravity();
	//		root[guid]["components"][componentGuid]["body_type"] = static_cast<int>(physicsComponent->getBodyType());
	//		root[guid]["components"][componentGuid]["linear_drag"] = physicsComponent->getLinearDrag();
	//		root[guid]["components"][componentGuid]["angular_drag"] = physicsComponent->getAngularDrag();
	//		root[guid]["components"][componentGuid]["constraints"] = physicsComponent->getConstraints();
	//	}
	//	AGameObject::ComponentList texList = gameObject->getComponentsOfType(AComponent::ComponentType::Tex);
	//	for (AComponent* component : texList)
	//	{
	//		std::string componentGuid = component->getGuidString();
	//		root[guid]["components"][componentGuid];
	//		root[guid]["components"][componentGuid]["name"] = component->getName();
	//		root[guid]["components"][componentGuid]["class"] = component->getClassType();
	//		root[guid]["components"][componentGuid]["type"] = component->getType();
	//		if (component->getClassType() == typeid(TextureComponent).raw_name())
	//		{
	//			TextureComponent* textureComponent = dynamic_cast<TextureComponent*>(component);
	//			root[guid]["components"][componentGuid]["texture_name"] = textureComponent->getTexName();
	//		}
	//	}

	//	AGameObject::ComponentList rendererList = gameObject->getComponentsOfType(AComponent::ComponentType::Renderer);
	//	for (AComponent* component : rendererList)
	//	{
	//		std::string componentGuid = component->getGuidString();
	//		root[guid]["components"][componentGuid];
	//		root[guid]["components"][componentGuid]["name"] = component->getName();
	//		root[guid]["components"][componentGuid]["class"] = component->getClassType();
	//		root[guid]["components"][componentGuid]["type"] = component->getType();

	//		if (component->getClassType() == typeid(MeshRenderer).raw_name())
	//		{
	//			MeshRenderer* meshRenderer = dynamic_cast<MeshRenderer*>(component);
	//			root[guid]["components"][componentGuid]["file_path"] = meshRenderer->getMesh()->getFilePath();
	//		}
	//	}
	//}

	//std::cout << root << "\n";

	//Json::StyledWriter styledWriter;
	//sceneFile << styledWriter.write(root);
	//sceneFile.close();
}

bool SceneRW::loadScene()
{
	std::string fileDirectory;
	std::string fileName;

	FileUtils::getScenePath(fileDirectory, fileName);

	if (fileDirectory.find(".level") != std::string::npos)
	{

	}
	else
	{
		return false;
	}

	std::ifstream sceneFile(fileDirectory, std::ifstream::binary);
	Json::Value scene;

	sceneFile >> scene;

	std::vector<std::string> guidList;

	for (std::string id : scene.getMemberNames()) {
		guidList.push_back(id);
	}

	for (std::string guid : guidList)
	{
		std::string name = scene[guid]["name"].asString();
		std::string type = scene[guid]["type"].asString();
		GameObject::PrimitiveType ptype;

		if (type == "Cube")
			ptype = GameObject::PrimitiveType::CUBE;
		else if (type == "Sphere")
			ptype = GameObject::PrimitiveType::SPHERE;
		else if (type == "Cylinder")
			ptype = GameObject::PrimitiveType::CYLINDER;
		else if (type == "Plane")
			ptype = GameObject::PrimitiveType::PLANE;
		else if (type == "Model")
			ptype = GameObject::PrimitiveType::NONE; //need to make separate type for models
		else
			ptype = GameObject::PrimitiveType::NONE;

		vec3 position;
		position.x = scene[guid]["position"]["x"].asFloat();
		position.y = scene[guid]["position"]["y"].asFloat();
		position.z = scene[guid]["position"]["z"].asFloat();

		vec3 rotation;
		rotation.x = scene[guid]["rotation"]["x"].asFloat();
		rotation.y = scene[guid]["rotation"]["y"].asFloat();
		rotation.z = scene[guid]["rotation"]["z"].asFloat();

		vec3 scale;
		scale.x = scene[guid]["scale"]["x"].asFloat();
		scale.y = scene[guid]["scale"]["y"].asFloat();
		scale.z = scene[guid]["scale"]["z"].asFloat();

		ModelManager::getInstance()->createObjectFromFile(name, ptype, position, rotation, scale);

	}
	
	return true;
}
