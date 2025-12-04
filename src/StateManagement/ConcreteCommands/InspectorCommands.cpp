#include "InspectorCommands.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/EventBroadcaster.h"

AlterTransformCommand::AlterTransformCommand(GameObject* object, Setter setter, Variant oldValue, Variant newValue)
	: gameObject(object), apply(setter), oldValue(oldValue), newValue(newValue)
{

}

void AlterTransformCommand::execute()
{
	this->apply(gameObject, newValue);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void AlterTransformCommand::undo()
{
	this->apply(gameObject, oldValue);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

TransformObjectCommand::TransformObjectCommand(GameObject* object, vec3 oldPosition, vec3 oldRotation, vec3 oldScale, vec3 newPosition, vec3 newRotation, vec3 newScale) 
	: gameObject(object), oldPosition(oldPosition), oldRotation(oldRotation), oldScale(oldScale), newPosition(newPosition), newRotation(newRotation), newScale(newScale) 
{
}

void TransformObjectCommand::execute()
{
	this->gameObject->setLocalPosition(newPosition);
	this->gameObject->SetLocalRotation(newRotation);
	this->gameObject->SetLocalScale(newScale);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void TransformObjectCommand::undo()
{
	this->gameObject->setLocalPosition(oldPosition);
	this->gameObject->SetLocalRotation(oldRotation);
	this->gameObject->SetLocalScale(oldScale);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}


