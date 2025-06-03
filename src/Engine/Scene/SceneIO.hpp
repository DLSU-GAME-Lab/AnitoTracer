#pragma once
#include "From-GDGRAP2/ModelManager.h"

#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

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

public:
	void SaveCurrentScene(std::string sceneName = "New Scene") {
		if (sceneName == "New Scene") sceneName = "New Scene " + std::to_string(scenes.size());
		else if (map[sceneName] != nullptr) { /* Already exists */ }

		ModelManager::List objects = ModelManager::getInstance()->getAllObjects();

		json scene;
		scene["scene_name"] = sceneName;
		scene["objects"] = json::array();

		for (std::shared_ptr<GameObject> obj : objects)
		{
			json objJson;

			objJson["name"] = obj->getName();
			objJson["type"] = obj->getType();
			objJson["enabled"] = obj->isEnabled();

			objJson["position"] = { obj->getWorldPosition().x, obj->getWorldPosition().y, obj->getWorldPosition().z };
			objJson["rotation"] = { obj->getWorldRotation().x, obj->getWorldRotation().y, obj->getWorldRotation().z };
			objJson["scale"] = { obj->getWorldScale().x, obj->getWorldScale().y, obj->getWorldScale().z };

			scene["objects"].push_back(objJson);
		}

		AddScene(scene, sceneName);

		WriteToFile(scene, sceneName);
	}

	void LoadScene(std::string name) {
		for (json obj : map[name]["objects"])
		{
			glm::vec3 pos = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
			glm::vec3 rot = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
			glm::vec3 scale = glm::vec3(obj["scale"][0], obj["scale"][1], obj["scale"][2]);
			ModelManager::getInstance()->createObjectFromScene(
				obj["name"],
				obj["type"],
				obj["enabled"],
				pos,
				rot,scale);

		}
	}

	void LoadFromFile() {
		// Implement file reading logic here
		// Populate objects with the loaded data
		/*for (const auto& obj : objects) {
			SceneIO::getInstance()->addObject(obj);
		}*/
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
};