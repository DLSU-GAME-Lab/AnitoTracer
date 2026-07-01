#pragma once 
#include "StateManagement/ICommand.hpp"
#include <string>
#include <glm/glm.hpp>
#include <variant>

class GameObject;

class AlterTransformCommand : public ICommand
{
public:
	using Variant = std::variant<glm::vec3, std::string, bool>;
	using Setter = std::function<void(GameObject* gameObject, const Variant&)>;

	AlterTransformCommand(GameObject* object, Setter setter, Variant oldValue, Variant newValue);
	~AlterTransformCommand() = default;

	void execute() override;
	void undo() override;

private:
	GameObject* gameObject;
	Setter apply;
	Variant oldValue;
	Variant newValue;

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