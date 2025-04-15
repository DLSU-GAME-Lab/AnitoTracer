#include "SceneRW.h"

#include <fstream>
#include <filesystem>
#include "Utilities/FileUtils.h"
#include "jsoncpp/json/json.h"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Utilities/Glm.hpp"

using namespace glm;

void SceneRW::exportScene()
{
	std::filesystem::path path = FileUtils::getSceneSavePath();
	std::string fileDirectory = path.string();
	fileDirectory.append("/scene.level");


	if (fileDirectory.find(".level") != std::string::npos) //check if .level or .txt
	{

	}

	std::fstream sceneFile;
	sceneFile.open(fileDirectory, std::ios::out);

	//Logger::log("Selected File Name : " + fileDirectory);

	Json::Value root;

	ModelManager::List objectList = ModelManager::getInstance()->getAllObjects();

	for (int i = 0; i < objectList.size(); i++)
	{
		std::shared_ptr<GameObject> gameObject = objectList[i];

		std::string guid = gameObject->getName();

		root[guid];
		root[guid]["name"] = gameObject->getName();

		GameObject::PrimitiveType ptype = GameObject::PrimitiveType::NONE;
		ptype = gameObject->getType();
		std::string type = "";

		if (ptype == GameObject::PrimitiveType::CUBE)
			type = "Cube";
		else if (ptype == GameObject::PrimitiveType::SPHERE)
			type = "Sphere";
		else if (ptype == GameObject::PrimitiveType::CAPSULE)
			type = "Capsule";
		else if (ptype == GameObject::PrimitiveType::CAMERA)
			type = "Camera";
		else if (ptype == GameObject::PrimitiveType::CYLINDER)
			type = "Cylinder";
		else if (ptype == GameObject::PrimitiveType::PLANE)
			type = "Plane";
		else if (ptype == GameObject::PrimitiveType::POINT_LIGHT)
			type = "PointLight";
		else if (ptype == GameObject::PrimitiveType::DIRECTIONAL_LIGHT)
			type = "DirectionalLight";
		else if (ptype == GameObject::PrimitiveType::QUAD)
			type = "Quad";
		else
			type = "Cube";

		root[guid]["type"] = type;

		vec3 position = gameObject->getLocalPosition();
		vec3 rotation = gameObject->getLocalRotation();
		vec3 scale = gameObject->getLocalScale();

		root[guid]["position"]["x"] = position.x;
		root[guid]["position"]["y"] = position.y;
		root[guid]["position"]["z"] = position.z;

		root[guid]["rotation"]["x"] = rotation.x;
		root[guid]["rotation"]["y"] = rotation.y;
		root[guid]["rotation"]["z"] = rotation.z;

		root[guid]["scale"]["x"] = scale.x;
		root[guid]["scale"]["y"] = scale.y;
		root[guid]["scale"]["z"] = scale.z;
	}

	std::cout << root << "\n";

	Json::StyledWriter styledWriter;
	sceneFile << styledWriter.write(root);
	sceneFile.close();
}

bool SceneRW::loadScene()
{
	std::string fileDirectory;
	std::string fileName;

	FileUtils::getScenePath(fileDirectory, fileName);

	if (fileDirectory.find(".level") != std::string::npos)
	{
		ModelManager::getInstance()->clearAllObjects();
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
		else if (type == "Capsule")
			ptype = GameObject::PrimitiveType::CAPSULE;
		else if (type == "Cylinder")
			ptype = GameObject::PrimitiveType::CYLINDER;
		else if (type == "Camera")
			ptype = GameObject::PrimitiveType::CAMERA;
		else if (type == "Plane")
			ptype = GameObject::PrimitiveType::PLANE;
		else if (type == "PointLight")
			ptype = GameObject::PrimitiveType::POINT_LIGHT;
		else if (type == "DirectionalLight")
			ptype = GameObject::PrimitiveType::DIRECTIONAL_LIGHT;
		else if (type == "Quad")
			ptype = GameObject::PrimitiveType::QUAD;
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

		ModelManager::getInstance()->createObject(name, ptype, position, rotation, scale);

	}
	
	return true;
}
