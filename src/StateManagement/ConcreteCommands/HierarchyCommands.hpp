#pragma once 
#include "StateManagement/ICommand.hpp"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "From-GDGRAP2/GameObject.h"

class ReparentCommand : public ICommand
{
public:
	ReparentCommand(GameObject* child, GameObject* oldParent, int oldIndex, GameObject* newParent, int newIndex);
	~ReparentCommand() = default;

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
	virtual ~CreateObjectCommand() = default;

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