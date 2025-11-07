#include "MaterialCommands.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

ModifyColorCommand::ModifyColorCommand(Assets::Material* material, glm::vec4 color) : material(material), newColor(color)
{
	this->oldColor = material->Diffuse;
}

void ModifyColorCommand::execute()
{
	this->material->SetAlbedoColor(this->newColor);
}

void ModifyColorCommand::undo()
{
	this->material->SetAlbedoColor(this->oldColor);
}

ChangeMapCommand::ChangeMapCommand(Assets::Material* material, int textureId) : material(material), newTextureId(textureId)
{
	this->oldTextureId = material->DiffuseTextureId;
}

void ChangeMapCommand::execute()
{
	this->material->SetAlbedoTexture(newTextureId);
}

void ChangeMapCommand::undo()
{
	this->material->SetAlbedoTexture(oldTextureId);
}

ModifyFuzzinessCommand::ModifyFuzzinessCommand(Assets::Material* material, float value) : material(material), newValue(value)
{
	this->oldValue = this->material->Fuzziness;
}

void ModifyFuzzinessCommand::execute()
{
	this->material->SetFuzziness(this->newValue);
}

void ModifyFuzzinessCommand::undo()
{
	this->material->SetFuzziness(this->oldValue);
}

ModifyRefractionIndexCommand::ModifyRefractionIndexCommand(Assets::Material* material, float value) : material(material), newValue(value)
{
	this->oldValue = this->material->RefractionIndex;
}

void ModifyRefractionIndexCommand::execute()
{
	this->material->SetRefractionIndex(this->newValue);
}

void ModifyRefractionIndexCommand::undo()
{
	this->material->SetRefractionIndex(this->oldValue);
}
