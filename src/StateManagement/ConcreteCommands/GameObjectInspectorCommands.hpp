#pragma once 
#include "../ICommand.hpp"
#include <string>
#include <glm/glm.hpp>

class GameObject;

class RenameCommand : public ICommand
{
public:
	RenameCommand(GameObject* object, std::string newName);
	~RenameCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	std::string newName;
	std::string oldName;
};

class MoveObjectCommand : public ICommand
{
public:
	using vec3 = glm::vec3;

	MoveObjectCommand(GameObject* object, vec3 position);
	~MoveObjectCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	vec3 newPosition;
	vec3 oldPosition;
};

class RotateObjectCommand : public ICommand
{
public:
	using vec3 = glm::vec3;

	RotateObjectCommand(GameObject* object, vec3 rotation);
	~RotateObjectCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	vec3 newRotation;
	vec3 oldRotation;
};

class ScaleObjectCommand : public ICommand
{
public:
	using vec3 = glm::vec3;

	ScaleObjectCommand(GameObject* object, vec3 scale);
	~ScaleObjectCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	vec3 newScale;
	vec3 oldScale;
};

/* Used for Gizmos */
class TransformObjectCommand : public ICommand
{
public:
	using vec3 = glm::vec3;

	TransformObjectCommand(GameObject* object, vec3 oldPosition, vec3 oldRotation, vec3 oldScale, vec3 newPosition, vec3 newRotation, vec3 newScale);
	~TransformObjectCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	vec3 newPosition;
	vec3 newRotation;
	vec3 newScale;

	vec3 oldPosition;
	vec3 oldRotation;
	vec3 oldScale;
};

class ToggleActiveGameObject : public ICommand
{
public:
	ToggleActiveGameObject(GameObject* object, bool isActive);
	~ToggleActiveGameObject() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	bool newActiveState;
	bool oldActiveState;
};