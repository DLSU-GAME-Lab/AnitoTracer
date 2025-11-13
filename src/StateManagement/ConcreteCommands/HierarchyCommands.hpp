#pragma once 
#include "StateManagement/ICommand.hpp"
#include <string>
#include <glm/glm.hpp>

class GameObject;

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