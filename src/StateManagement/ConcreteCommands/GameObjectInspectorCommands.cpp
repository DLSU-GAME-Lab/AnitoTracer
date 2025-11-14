#include "GameObjectInspectorCommands.hpp"
#include "From-GDGRAP2/GameObject.h"

RenameCommand::RenameCommand(GameObject* object, std::string newName) : gameObject(object), newName(newName)
{
	this->oldName = object->getName();
}

void RenameCommand::execute()
{
	this->gameObject->setName(this->newName);
}

void RenameCommand::undo()
{
	this->gameObject->setName(this->oldName);
}

MoveObjectCommand::MoveObjectCommand(GameObject* object, VectorUtils::vec3 position) : gameObject(object), newPosition(position)
{
	this->oldPosition = object->getLocalPosition();
}

void MoveObjectCommand::execute()
{
	this->gameObject->setLocalPosition(this->newPosition);
}

void MoveObjectCommand::undo()
{
	this->gameObject->setLocalPosition(this->oldPosition);
}

RotateObjectCommand::RotateObjectCommand(GameObject* object, vec3 rotation) : gameObject(object), newRotation(rotation)
{
	this->oldRotation = object->getLocalRotation();
}

void RotateObjectCommand::execute()
{
	this->gameObject->setLocalRotation(this->newRotation);
}

void RotateObjectCommand::undo()
{
	this->gameObject->setLocalRotation(this->oldRotation);
}

ScaleObjectCommand::ScaleObjectCommand(GameObject* object, vec3 scale) : gameObject(object), newScale(scale)
{
	this->oldScale = object->getLocalScale();
}

void ScaleObjectCommand::execute()
{
	this->gameObject->setLocalScale(this->newScale);
}

void ScaleObjectCommand::undo()
{
	this->gameObject->setLocalScale(this->oldScale);
}

ToggleActiveGameObject::ToggleActiveGameObject(GameObject* object, bool isActive) : gameObject(object), newActiveState(isActive)
{
	this->oldActiveState = this->gameObject->isActive();
}

void ToggleActiveGameObject::execute()
{
	this->gameObject->setActive(this->newActiveState);
}

void ToggleActiveGameObject::undo()
{
	this->gameObject->setActive(this->oldActiveState);
}

TransformObjectCommand::TransformObjectCommand(GameObject* object, vec3 oldPosition, vec3 oldRotation, vec3 oldScale, vec3 newPosition, vec3 newRotation, vec3 newScale) 
	: gameObject(object), oldPosition(oldPosition), oldRotation(oldRotation), oldScale(oldScale), newPosition(newPosition), newRotation(newRotation), newScale(newScale) 
{
}

void TransformObjectCommand::execute()
{
	this->gameObject->setLocalPosition(newPosition);
	this->gameObject->setLocalRotation(newRotation);
	this->gameObject->setLocalScale(newScale);
}

void TransformObjectCommand::undo()
{
	this->gameObject->setLocalPosition(oldPosition);
	this->gameObject->setLocalRotation(oldRotation);
	this->gameObject->setLocalScale(oldScale);
}