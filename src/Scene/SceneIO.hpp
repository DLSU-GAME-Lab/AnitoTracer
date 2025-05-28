#pragma once
#include "ModelManager.h"

class SceneIO {
private:
	Scene() = default;
	~Scene() = default;

	std::string sceneName;
	ModelManager::List objects;

public:
	SaveCurrentScene() {
		objects = ModelManager::getInstance()->getAllObjects();
	}

	LoadScene() {

	}

	WriteToFile() {
		// Implement file writing logic here
	}

	LoadFromFile() {
		// Implement file reading logic here
		// Populate objects with the loaded data
		for (const auto& obj : objects) {
			ModelManager::getInstance()->addObject(obj);
		}
};