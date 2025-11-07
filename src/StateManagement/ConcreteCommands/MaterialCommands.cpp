#include "MaterialCommands.hpp"

ModifyMaterialPropertyCommand::ModifyMaterialPropertyCommand(Assets::Material* material, Setter setter, Variant oldValue, Variant newValue)
	: material(material), apply(setter), oldValue(oldValue), newValue(newValue)
{

}

void ModifyMaterialPropertyCommand::execute()
{
	this->apply(material, newValue);
}

void ModifyMaterialPropertyCommand::undo()
{
	this->apply(material, oldValue);
}