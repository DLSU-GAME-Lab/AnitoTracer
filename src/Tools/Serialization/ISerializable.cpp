#include "ISerializable.hpp"
#include "File/Parser.hpp"

gbe::SerializedData gbe::ISerializable::Serialize() {
	
	SerializedData data = {};

	for (const auto& prop : this->properties)
	{
		prop->Serialize(data);
	}

	return data;
}

void gbe::ISerializable::Deserialize(SerializedData& data)
{
	for (auto& prop : this->properties)
	{
		prop->Deserialize(data);
	}
}

void gbe::ISerializable::DeserializeFromFile(std::filesystem::path absolute_path)
{
	SerializedData data = {};
	Parser::PopulateClass(data, absolute_path);
	this->Deserialize(data);
}

void gbe::ISerializable::SerializeToFile(std::filesystem::path absolute_path)
{
	SerializedData data = Serialize();
	Parser::ExportClass(data, absolute_path);
}

gbe::ISerializable::ISerializable(gbe::SerializedData& data)
{
	Deserialize(data);
}

gbe::ISerializable::ISerializable()
{
}

void gbe::ISerializable::RegisterProperty(IAutoSerializer* newprop)
{
	this->properties.push_back(newprop);
}

void gbe::ISerializable::UnRegisterProperty(IAutoSerializer* prop)
{
	for (size_t i = 0; i < this->properties.size(); i++)
	{
		if (this->properties[i] == prop) {
			this->properties.erase(this->properties.begin() + i);
			break;
		}
	}
}