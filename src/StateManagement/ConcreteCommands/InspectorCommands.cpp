#include "InspectorCommands.hpp"
#include "From-GDGRAP2/GameObject.h"

AlterTransformCommand::AlterTransformCommand(GameObject* object, Setter setter, Variant oldValue, Variant newValue)
	: gameObject(object), apply(setter), oldValue(oldValue), newValue(newValue)
{

}

void AlterTransformCommand::execute()
{
	this->apply(gameObject, newValue);
}

void AlterTransformCommand::undo()
{
	this->apply(gameObject, oldValue);
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


