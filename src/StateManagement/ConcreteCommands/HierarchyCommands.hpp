#pragma once 
#include "StateManagement/ICommand.hpp"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "AssetManagement/GameObject.hpp"
#include "Engine/LightSystem/Light.h"

class GameObject;

class ReparentCommand : public ICommand
{
public:
	ReparentCommand(GameObject* child, GameObject* oldParent, int oldIndex, GameObject* newParent, int newIndex);
	~ReparentCommand();

	void execute() override;
	void undo() override;

private:
	GameObject* child;
	GameObject* oldParent;
	int oldIndex;
	GameObject* newParent;
	int newIndex;
};

class CreateObjectCommand : public ICommand
{
public:
	CreateObjectCommand(glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 sca = glm::vec3(1));
	virtual ~CreateObjectCommand();

	void execute() override;
	void undo() override;

protected:
	virtual std::unique_ptr<GameObject> createObject() = 0;

	virtual void applyPostCreation(GameObject* obj);

	std::unique_ptr<GameObject> createdObjectStorage;
	GameObject* createdObjectRef = nullptr;

	glm::vec3 storedPosition;
	glm::vec3 storedRotation;
	glm::vec3 storedScale;
};

/* For Preloaded meshes */
class CreatePrimitiveCommand : public CreateObjectCommand 
{
public:
	CreatePrimitiveCommand(GameObject::PrimitiveType type, std::string name, glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 sca = glm::vec3(1));
	~CreatePrimitiveCommand() = default;

protected:
	std::unique_ptr<GameObject> createObject() override;
	void applyPostCreation(GameObject* obj) override {}

private:
	GameObject::PrimitiveType type;
	std::string name;
};

class CreateMeshCommand : public CreateObjectCommand
{
public:
	CreateMeshCommand(std::string filePath, std::string name, glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 sca = glm::vec3(1));
	~CreateMeshCommand() = default;

protected:
	std::unique_ptr<GameObject> createObject() override;

private:
	std::string filePath;
	std::string name;
};

class CreateLightCommand : public ICommand
{
public:
	CreateLightCommand(Light::LightType type, std::string name, 
		glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 sca = glm::vec3(1), 
		glm::vec4 lightCol = glm::vec4(1.0f), glm::vec4 ambientCol = glm::vec4(1.0f));
	~CreateLightCommand();

	void execute() override;
	void undo() override;

private:
	std::unique_ptr<Light> createdObjectStorage;
	Light* createdObjectRef = nullptr;

	glm::vec3 storedPosition;
	glm::vec3 storedRotation;
	glm::vec3 storedScale;

	Light::LightType type;
	std::string name;

	glm::vec4 lightColor;
	glm::vec4 ambientColor;
};

/* For Existing Objects */
class AddObjectCommand : public ICommand
{
public:
	AddObjectCommand(std::unique_ptr<GameObject> gameObject);
	~AddObjectCommand();

	void execute() override;
	void undo() override;

private:
	std::unique_ptr<GameObject> objectStorage;
	GameObject* objectRef = nullptr;
};

class DeleteObjectCommand : public ICommand
{
public:
	DeleteObjectCommand(GameObject* objectToDelete);
	~DeleteObjectCommand();

	void execute() override;
	void undo() override;

private:
	std::unique_ptr<GameObject> objectStorage;
	GameObject* objectRef = nullptr;
};