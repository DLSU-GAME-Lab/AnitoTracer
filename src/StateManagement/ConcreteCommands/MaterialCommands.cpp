#include "MaterialCommands.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

ModifyMaterialPropertyCommand::ModifyMaterialPropertyCommand(Assets::Material* material, Setter setter, Variant oldValue, Variant newValue)
	: material(material), apply(setter), oldValue(oldValue), newValue(newValue)
{

}

void ModifyMaterialPropertyCommand::execute()
{
	this->apply(material, newValue);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void ModifyMaterialPropertyCommand::undo()
{
	this->apply(material, oldValue);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}