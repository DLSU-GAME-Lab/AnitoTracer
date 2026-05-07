#pragma once
#include "From-GDGRAP2/ModelManager.h"

#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "Utilities/FileUtils.h"
#include "../../From-GDGRAP2/GameObject.h"
#include "../../From-GDGRAP2/Debug.h"

using namespace nlohmann;
class SceneIO {
public:
	typedef std::vector<json> SceneList;
	typedef std::unordered_map<std::string, json> SceneMap;

private:
	SceneIO() = default;
	~SceneIO() = default;
	SceneIO(SceneIO const&) {};             // copy constructor is private
	SceneIO& operator=(SceneIO const&) {};  // assignment operator is private*/
	static SceneIO* sharedInstance;

	SceneList scenes;
	SceneMap map;

public:
	static SceneIO* getInstance();
	static void initialize();
	static void destroy();

private:
	void AddScene(json scene, std::string sceneName)
	{
		scenes.push_back(scene);
		map[sceneName] = scene;
	}

	void WriteToFile(json scene, std::string sceneName) {
		// Implement file writing logic here
		std::ofstream file(sceneName + ".json");

		if (file.is_open()) {
			file << std::setw(4) << scene << std::endl; // Use setw for pretty printing
			file.close();
			std::cout << "Scene saved to " + sceneName + ".json" << std::endl;
		}
		else {
			std::cerr << "Error opening file for writing" << std::endl;
		}
	}

	std::stringstream OBJToString(std::string filename)
	{

		const std::ifstream file(filename, std::ios::binary);
		std::stringstream returnBytes;
		returnBytes << file.rdbuf();

		return returnBytes;
	}

	void BytesToOBJ(std::string bytes, std::string objName)
	{
		//load the bytes into a new obj file
		std::string modelLocation = FileUtils::getAssetsFolderPath().generic_string() + "/models/";
		std::string filePath = modelLocation + objName + ".obj";
		//convert path to wchar_t
		std::wstring widestr = std::wstring(filePath.begin(), filePath.end());
		const wchar_t* charPath = widestr.c_str();

		std::ofstream file(filePath, std::ios::binary);
		file << bytes;
	}

public:
	void SaveCurrentScene(std::string sceneName = "New Scene") {
		if (sceneName == "New Scene") sceneName = "New Scene " + std::to_string(scenes.size());
		else if (map[sceneName] != nullptr) { /* Already exists */ }

		auto objects = ModelManager::getInstance()->getObjectList();
		ModelManager::LightPropsList lights = ModelManager::getInstance()->getAllLightProperties();
		int lightIndex = 0;

		json scene;
		scene["scene_name"] = sceneName;
		scene["objects"] = json::array();

		for (auto obj : objects)
		{
			json objJson;

			// 1. Identification
			objJson["name"] = obj->getName();
			objJson["type"] = obj->getType();
			objJson["enabled"] = obj->isActive();

			objJson["position"] = { obj->getWorldPosition().x, obj->getWorldPosition().y, obj->getWorldPosition().z };
			objJson["rotation"] = { obj->getWorldRotationEuler().x, obj->getWorldRotationEuler().y, obj->getWorldRotationEuler().z };
			objJson["scale"] = { obj->getWorldScale().x, obj->getWorldScale().y, obj->getWorldScale().z };

			// 2. Model
			std::shared_ptr<Assets::Model> modelRef = obj->getModel();
			// First the shape.
			if (modelRef) {
				if (obj->getType() == GameObject::PrimitiveType::MESH)
					objJson["modelPath"] = modelRef->FilePath();
				else
					objJson["modelPath"] = "";
			}
			// Second the material.
			if (obj->getType() == GameObject::PrimitiveType::POINT_LIGHT ||
				obj->getType() == GameObject::PrimitiveType::DIRECTIONAL_LIGHT ||
				obj->getType() == GameObject::PrimitiveType::SPOT_LIGHT) // 2.5 Light Properties
			{
				json lightProps;
				lightProps["lightDir"] = { lights[0].LightDir.x, lights[0].LightDir.y, lights[0].LightDir.z };
				lightProps["ambientColor"] = { lights[0].AmbientColor.x, lights[0].AmbientColor.y, lights[0].AmbientColor.z, lights[0].AmbientColor.w};
				lightProps["lightColor"] = { lights[0].LightColor.x, lights[0].LightColor.y, lights[0].LightColor.z, lights[0].LightColor.w};
				objJson["lightProps"] = lightProps;
			} else
			{
				objJson["modelName"] = modelRef->GetName();
				objJson["materials"] = json::array();
				for (Assets::Material mat : modelRef->materials_) {
					json matJson;
					matJson["diffuse"] = { mat.Diffuse.x, mat.Diffuse.y, mat.Diffuse.z, mat.Diffuse.a };
					matJson["diffuseTextureId"] = mat.DiffuseTextureId;
					matJson["fuzziness"] = mat.Fuzziness;
					matJson["refractionIndex"] = mat.RefractionIndex;
					matJson["model"] = mat.MaterialModel;
					objJson["materials"].push_back(matJson);
				}
			}

			// 3. Family lol
			objJson["parent"] = obj->getParent() ? obj->getParent()->getName() : "";
			objJson["children"] = json::array();
			for (GameObject* child : obj->getChildren()) {
				objJson["children"].push_back(child->getName());
			}

			scene["objects"].push_back(objJson);
		}

		AddScene(scene, sceneName);

		WriteToFile(scene, sceneName);
	}

	void LoadScene(std::string name) {
		for (json obj : map[name]["objects"]) {
			glm::vec3 pos = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
			glm::vec3 rot = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
			glm::vec3 scale = glm::vec3(obj["scale"][0], obj["scale"][1], obj["scale"][2]);

			// Mesh objects are created here.
			if (obj["type"] == GameObject::PrimitiveType::MESH || 
				obj["type"] == GameObject::PrimitiveType::OBJECT_GROUP)
			{
				// 1. Load Mesh from path.
				Assets::Model model = Assets::Model::LoadModel(obj["modelPath"]);
				model.SetName(obj["modelName"]);

				// 2. Set materials.
				std::vector<Assets::Material> materials = LoadMaterials(obj);
				model.SetMaterials(materials);

				// 3. Create the object.
				std::unique_ptr<GameObject> object = std::make_unique<GameObject>(obj["name"], GameObject::PrimitiveType::MESH, std::make_shared<Assets::Model>(model));
				object->setLocalPosition(pos);
				object->setLocalRotationEuler(rot);
				object->setLocalScale(scale);
				ModelManager::getInstance()->addObject(std::move(object));
				// 4. Family TODO
			}
			// Primitives, Lighting, and Camera Objects are created here.
			else if (obj["type"] == GameObject::PrimitiveType::CUBE ||
					obj["type"] == GameObject::PrimitiveType::SPHERE ||
					obj["type"] == GameObject::PrimitiveType::PLANE || 
					obj["type"] == GameObject::PrimitiveType::CYLINDER || 
					obj["type"] == GameObject::PrimitiveType::CAPSULE ||
					obj["type"] == GameObject::PrimitiveType::CORNELL_BOX)
			{
				// 2. Get materials.
				std::vector<Assets::Material> materials = LoadMaterials(obj);

				// 3. Create the object.
				ModelManager::getInstance()->createPrimitiveFromScene(
					obj["name"], obj["type"], obj["enabled"],
					pos, rot, scale, materials);

				// 4. Family TODO
			}
			else if (obj["type"] == GameObject::PrimitiveType::POINT_LIGHT ||
					obj["type"] == GameObject::PrimitiveType::DIRECTIONAL_LIGHT ||
					obj["type"] == GameObject::PrimitiveType::SPOT_LIGHT)
			{
				// 2. Get materials.
				std::vector<Assets::Material> materials = LoadMaterials(obj);
				glm::vec3 lightDir = glm::vec3(obj["lightProps"]["lightDir"][0], obj["lightProps"]["lightDir"][1], obj["lightProps"]["lightDir"][2]);
				glm::vec4 ambientCol = glm::vec4(obj["lightProps"]["ambientColor"][0], obj["lightProps"]["ambientColor"][1], obj["lightProps"]["ambientColor"][2], obj["lightProps"]["ambientColor"][3]);
				glm::vec4 lightCol = glm::vec4(obj["lightProps"]["lightColor"][0], obj["lightProps"]["lightColor"][1], obj["lightProps"]["lightColor"][2], obj["lightProps"]["lightColor"][3]);
				Assets::LightProperties props = { pos, lightDir, ambientCol, lightCol, Assets::LightProperties::Enum::PointLight };

				// 3. Create the object.
				ModelManager::getInstance()->createLightFromScene(
					obj["name"], obj["type"], obj["enabled"],
					pos, rot, scale, materials, props);

				// 4. Family TODO
			}
		}
	}

	void ReadFromDirectory()
	{
		scenes.clear();
		this->map.clear();

		for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
			std::string msg = "File: " + entry.path().string() + std::filesystem::current_path().string();
			Debug::Log(msg);

			if (entry.path().extension() == ".json") {
				std::ifstream file(entry.path());
				if (file.is_open()) {
					try {
						json scene;
						file >> scene;
						std::string sceneName = scene["scene_name"];
						AddScene(scene, sceneName);

						std::string msg1 = "Loaded scene: " + sceneName;
						Debug::Log(msg1);
					}
					catch (const std::exception& e) {
						std::string msg1 = "Failed to parse " + entry.path().filename().string() + ": " + e.what();
						Debug::Log(msg1);
					}
				}
				else {
					std::string msg1 = "Failed to open file: " + entry.path().filename().string();
					Debug::Log(msg1);
				}
			}
		}
	}

	std::vector<std::string> getSceneNames() const
	{
		std::vector<std::string> sceneNames;

		for (json scene : scenes)
		{
			sceneNames.push_back(scene["scene_name"]);
		}
		return sceneNames;
	}

	std::vector<Assets::Material> LoadMaterials(json obj)
	{
		// 2. Set materials.
		std::vector<Assets::Material> materials;
		for (json mat : obj["materials"]) {
			Assets::Material material;
			material.Diffuse = glm::vec4(mat["diffuse"][0], mat["diffuse"][1], mat["diffuse"][2], mat["diffuse"][3]);
			material.DiffuseTextureId = mat["diffuseTextureId"];
			material.Fuzziness = mat["fuzziness"];
			material.RefractionIndex = mat["refractionIndex"];
			material.MaterialModel = mat["model"];

			materials.push_back(material);
		}

		return materials;
	}
};