#pragma once
#include "From-GDGRAP2/ModelManager.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cstring>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "Utilities/FileUtils.h"
#include "../../From-GDGRAP2/GameObject.h"
#include "../../From-GDGRAP2/Debug.h"
#include "../../From-GDGRAP2/TextureLibrary.h"
#include "../../Assets/Texture.hpp"
#include "../../Assets/Procedural.hpp"

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


	template<typename T>
	std::vector<unsigned char> ObjectToBytes(const T& object)
	{
		std::cout << "Converting object of type " << typeid(T).name() << " to bytes..." << std::endl;
		std::vector<unsigned char> bytes(sizeof(T));

		std::memcpy(bytes.data(), &object, sizeof(T));

		return bytes;
	}


	template<typename T>
	T BytesToObject(const std::vector<unsigned char>& bytes)
	{
		std::cout << "Converting bytes to object of type " << typeid(T).name() << "..." << std::endl;
		if (bytes.size() != sizeof(T))
		{
			throw std::runtime_error("Byte size does not match object size.");
		}

		T object;

		std::memcpy(&object, bytes.data(), sizeof(T));

		return object;
	}

	std::string ModelToString(std::string filename)
	{

		std::ifstream file(filename, std::ios::binary);

		if (!file)
		{
			throw std::runtime_error("Failed to open file: " + filename);
		}

		// Move to end to get file size
		file.seekg(0, std::ios::end);
		std::streamsize fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		// Read bytes into string
		std::string data;
		data.resize(static_cast<size_t>(fileSize));

		if (!file.read(&data[0], fileSize))
		{
			throw std::runtime_error("Failed to read file data.");
		}

		return data;
	}

	std::string BytesToModel(const std::string& data, const std::string& outputPath)
	{
		std::ofstream file(outputPath, std::ios::binary);

		if (!file)
		{
			throw std::runtime_error("Failed to create file: " + outputPath);
		}

		file.write(data.data(), data.size());

		if (!file)
		{
			throw std::runtime_error("Failed to write file data.");
		}

		return outputPath;
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

		//Texture Library
		TextureLibrary* texLib = TextureLibrary::getInstance();
		scene["textures"] = json::array();
		
		for (std::shared_ptr<Assets::Texture> tex : texLib->getTextureList()) 
		{
			json textureJson;
			textureJson["textureName"] = tex->Name();
			textureJson["textureWidth"] = tex->Width();
			textureJson["textureHeight"] = tex->Height();
			textureJson["textureChannels"] = tex->Channels();
			textureJson["texturePixels"] = *tex->Pixels();
			scene["textures"].push_back(textureJson);
		}

		//Objects
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

			json modelJson;
			std::shared_ptr<Assets::Model> modelRef = obj->getModel();
			modelJson["modelName"] = modelRef ? modelRef->GetName() : "";

			modelJson["originalVertices"] = json::array();
			for (Assets::Vertex v : modelRef->OriginalVertices()) 
			{
				modelJson["originalVertices"].push_back({
					v.Position.x, v.Position.y, v.Position.z,
					v.Normal.x, v.Normal.y, v.Normal.z,
					v.TexCoord.x, v.TexCoord.y,
					v.MaterialIndex
					});
			}
			modelJson["transformedVertices"] = json::array();
			for (Assets::Vertex v : modelRef->TransformedVertices())
			{
				modelJson["transformedVertices"].push_back({
					v.Position.x, v.Position.y, v.Position.z,
					v.Normal.x, v.Normal.y, v.Normal.z,
					v.TexCoord.x, v.TexCoord.y,
					v.MaterialIndex
					});
			}
			modelJson["vertices"] = json::array();
			for (Assets::Vertex v : modelRef->Vertices())
			{
				modelJson["vertices"].push_back({
					v.Position.x, v.Position.y, v.Position.z,
					v.Normal.x, v.Normal.y, v.Normal.z,
					v.TexCoord.x, v.TexCoord.y,
					v.MaterialIndex
					});
			}
			modelJson["indices"] = json::array();
			for (uint32_t i : modelRef->Indices())
			{
				modelJson["indices"].push_back(i);
			}
			modelJson["worldMatrix"] = {
				modelRef->GetWorldMatrix()[0][0], modelRef->GetWorldMatrix()[0][1], modelRef->GetWorldMatrix()[0][2], modelRef->GetWorldMatrix()[0][3],
				modelRef->GetWorldMatrix()[1][0], modelRef->GetWorldMatrix()[1][1], modelRef->GetWorldMatrix()[1][2], modelRef->GetWorldMatrix()[1][3],
				modelRef->GetWorldMatrix()[2][0], modelRef->GetWorldMatrix()[2][1], modelRef->GetWorldMatrix()[2][2], modelRef->GetWorldMatrix()[2][3],
				modelRef->GetWorldMatrix()[3][0], modelRef->GetWorldMatrix()[3][1], modelRef->GetWorldMatrix()[3][2], modelRef->GetWorldMatrix()[3][3]
			};
			modelJson["filePath"] = modelRef->FilePath();
			modelJson["origin"] = { modelRef->GetOrigin().x, modelRef->GetOrigin().y, modelRef->GetOrigin().z };

			objJson["model"] = modelJson;
				

			// Materials
			if (obj->getType() == GameObject::PrimitiveType::POINT_LIGHT ||
				obj->getType() == GameObject::PrimitiveType::DIRECTIONAL_LIGHT ||
				obj->getType() == GameObject::PrimitiveType::SPOT_LIGHT) // 2.5 Light Properties
			{
				json lightProps;
				lightProps["lightDir"] = { lights[0].LightDir.x, lights[0].LightDir.y, lights[0].LightDir.z };
				lightProps["ambientColor"] = { lights[0].AmbientColor.x, lights[0].AmbientColor.y, lights[0].AmbientColor.z, lights[0].AmbientColor.w};
				lightProps["lightColor"] = { lights[0].LightColor.x, lights[0].LightColor.y, lights[0].LightColor.z, lights[0].LightColor.w};
				objJson["lightProps"] = lightProps;
			} 
			else
			{
				objJson["modelName"] = modelRef->GetName();
				objJson["materials"] = json::array();
				for (Assets::Material mat : modelRef->materials_) 
				{
					json matJson;
					matJson["diffuse"] = { mat.Diffuse.x, mat.Diffuse.y, mat.Diffuse.z, mat.Diffuse.a };
					matJson["diffuseTextureId"] = mat.DiffuseTextureId;
					matJson["fuzziness"] = mat.Fuzziness;
					matJson["refractionIndex"] = mat.RefractionIndex;
					matJson["model"] = mat.MaterialModel;
					objJson["materials"].push_back(matJson);
				}
			}

			// Parenting
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
			if (obj["type"] == GameObject::PrimitiveType::MESH || obj["type"] == GameObject::PrimitiveType::OBJECT_GROUP)
			{
				Assets::Model model = BytesToObject<Assets::Model>(obj["meshData"]);

				// 3. Create the object.
				std::unique_ptr<GameObject> object = std::make_unique<GameObject>(obj["name"], GameObject::PrimitiveType::MESH, std::make_shared<Assets::Model>(model));
				object->setLocalPosition(pos);
				object->setLocalRotationEuler(rot);
				object->setLocalScale(scale);
				ModelManager::getInstance()->addObject(std::move(object));
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

			}
			// FAMILY TODO

		}
	}

	void LoadSceneFromFile(std::string filePath) {

		//Load the imported json file
		std::ifstream file(filePath);

		if (!file.is_open())
		{
			throw std::runtime_error(
				"Failed to open JSON file: " + filePath);
		}

		nlohmann::json sceneObj;

		try
		{
			file >> sceneObj;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			throw std::runtime_error(
				std::string("JSON parse error: ") + e.what());
		}

		//Texture Library first
		//LoadTextures(sceneObj);

		//Start looping through each object
		for (json obj : sceneObj["objects"]) {
			glm::vec3 pos = glm::vec3(obj["position"][0], obj["position"][1], obj["position"][2]);
			glm::vec3 rot = glm::vec3(obj["rotation"][0], obj["rotation"][1], obj["rotation"][2]);
			glm::vec3 scale = glm::vec3(obj["scale"][0], obj["scale"][1], obj["scale"][2]);

			// Mesh objects are created here.
			if (obj["type"] == GameObject::PrimitiveType::MESH || obj["type"] == GameObject::PrimitiveType::OBJECT_GROUP)
			{
				Assets::Model model = LoadModel(obj);

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
		//Set materials.
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

	void LoadTextures(json scene)
	{
		//Add textures back to Texture Library.
		TextureLibrary::getInstance()->Reset();

		for (json tex : scene["textures"]) {
			std::string name = tex["textureName"];
			int width = tex["textureWidth"];
			int height = tex["textureHeight"];
			int channels = tex["textureChannels"];
			unsigned char pixels = tex["texturePixels"].get<unsigned char>();
			Assets::Texture texture = Assets::Texture(width, height, channels, &pixels, name);

			TextureLibrary::getInstance()->addTexture(name,texture);
		}

	}

	Assets::Model LoadModel(json objJson) 
	{
		Assets::Model model;
		model.name = objJson["model"]["modelName"];
		model.filepath = objJson["model"]["filePath"];
		//vertices
		std::vector<Assets::Vertex> originalVertices;
		for (json v : objJson["model"]["originalVertices"]) {
			Assets::Vertex vertex;
			vertex.Position = glm::vec3(v[0], v[1], v[2]);
			vertex.Normal = glm::vec3(v[3], v[4], v[5]);
			vertex.TexCoord = glm::vec2(v[6], v[7]);
			vertex.MaterialIndex = v[8];
			originalVertices.push_back(vertex);
		}
		std::vector<Assets::Vertex> transformedVertices;
		for (json v : objJson["model"]["transformedVertices"]) {
			Assets::Vertex vertex;
			vertex.Position = glm::vec3(v[0], v[1], v[2]);
			vertex.Normal = glm::vec3(v[3], v[4], v[5]);
			vertex.TexCoord = glm::vec2(v[6], v[7]);
			vertex.MaterialIndex = v[8];
			transformedVertices.push_back(vertex);
		}
		std::vector<Assets::Vertex> vertices;
		for (json v : objJson["model"]["vertices"]) {
			Assets::Vertex vertex;
			vertex.Position = glm::vec3(v[0], v[1], v[2]);
			vertex.Normal = glm::vec3(v[3], v[4], v[5]);
			vertex.TexCoord = glm::vec2(v[6], v[7]);
			vertex.MaterialIndex = v[8];
			vertices.push_back(vertex);
		}

		model.indices_ = objJson["model"]["indices"].get<std::vector<uint32_t>>();
		model.worldMatrix_ = glm::mat4(
			(float)objJson["model"]["worldMatrix"][0], (float)objJson["model"]["worldMatrix"][1], (float)objJson["model"]["worldMatrix"][2], (float)objJson["model"]["worldMatrix"][3],
			(float)objJson["model"]["worldMatrix"][4], (float)objJson["model"]["worldMatrix"][5], (float)objJson["model"]["worldMatrix"][6], (float)objJson["model"]["worldMatrix"][7],
			(float)objJson["model"]["worldMatrix"][8], (float)objJson["model"]["worldMatrix"][9], (float)objJson["model"]["worldMatrix"][10], (float)objJson["model"]["worldMatrix"][11],
			(float)objJson["model"]["worldMatrix"][12], (float)objJson["model"]["worldMatrix"][13], (float)objJson["model"]["worldMatrix"][14], (float)objJson["model"]["worldMatrix"][15]
		);
		model.origin = glm::vec3(objJson["model"]["origin"][0], objJson["model"]["origin"][1], objJson["model"]["origin"][2]);

		return model;
	}
};