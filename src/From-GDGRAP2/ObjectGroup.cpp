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