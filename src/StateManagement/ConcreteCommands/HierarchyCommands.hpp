#pragma once 
#include "StateManagement/ICommand.hpp"
#include <string>
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


class CreatePrimitiveCommand : public ICommand
{
public:
	CreatePrimitiveCommand(GameObject::PrimitiveType type, std::string name);
	~CreatePrimitiveCommand() = default;

	void execute() override;
	void undo() override;

private:
	std::unique_ptr<GameObject> createdObjectStorage;
	GameObject* createdObjectRef;
	GameObject::PrimitiveType type;
	std::string name;
};