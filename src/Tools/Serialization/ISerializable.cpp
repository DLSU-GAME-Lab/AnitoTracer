#include "ISerializable.hpp"
#include "File/Parser.hpp"
#include "IAutoSerializer.hpp"
#include "SceneRegistry.hpp"

gbe::ISerializable::ISerializable()
{
	// Automatically register default generated GUID
	SceneRegistry::GetInstance().Register(m_guid, this);
}

gbe::ISerializable::ISerializable(gbe::SerializedData& data)
{
	// Register before deserialization
	SceneRegistry::GetInstance().Register(m_guid, this);
	Deserialize(data);
}

gbe::ISerializable::~ISerializable()
{
	// Cleanup registration on destruction
	SceneRegistry::GetInstance().Unregister(m_guid);
}

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
	// Unregister initial temporary GUID
	SceneRegistry::GetInstance().Unregister(m_guid);

	for (auto& prop : this->properties)
	{
		prop->Deserialize(data);
	}

	// Register restored persistent GUID loaded from file
	SceneRegistry::GetInstance().Register(m_guid, this);
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