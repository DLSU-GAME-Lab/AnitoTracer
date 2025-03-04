#include "ObjectGroup.h"
#include <iostream>

#include "EventBroadcaster.h"
#include "EventNames.h"
#include "Debug.h"

ObjectGroup::ObjectGroup(String name) : GameObject(name, PrimitiveType::OBJECT_GROUP)
{

}

void ObjectGroup::addModel(std::shared_ptr<Assets::Model> model)
{
	this->modelList.push_back(model);
}

void ObjectGroup::clearModelGroup()
{
	this->modelList.clear();
}

int ObjectGroup::getSize() const
{
	return this->modelList.size();
}

std::shared_ptr<Assets::Model> ObjectGroup::getModelAt(int index)
{
	return this->modelList[index];
}

void ObjectGroup::performModelTransform()
{
	mat4 translateOp = glm::translate(mat4(1), this->worldPosition);

	for (auto& model : this->modelList)
	{
		if (model)
			model->Transform(translateOp);
	}

	Debug::Log("Performed group model transform on object: " + this->name + "\n");
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void ObjectGroup::performModelRotate()
{
	mat4 rotateXOp = glm::rotate(mat4(1), glm::radians(this->worldRotation.x), vec3(1, 0, 0));
	mat4 rotateYOp = glm::rotate(mat4(1), glm::radians(this->worldRotation.y), vec3(0, 1, 0));
	mat4 rotateZOp = glm::rotate(mat4(1), glm::radians(this->worldRotation.z), vec3(0, 0, 1));

	for (auto& model : this->modelList)
	{
		if (model)
		{
			model->Transform(rotateXOp);
			model->Transform(rotateYOp);
			model->Transform(rotateZOp);
		}
	}

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void ObjectGroup::performModelScale()
{
	mat4 scaleOp = glm::scale(mat4(1), this->worldScale);

	for (auto& model : this->modelList)
	{
		if (model)
			model->Transform(scaleOp);
	}

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}
