#pragma once

#include "SerializedData.hpp"

#include <list>
#include <functional>
#include <filesystem>

#include "GUID.hpp"

namespace gbe {
	struct IAutoSerializer;

	class ISerializable {
	private:
		GUID m_guid = GUID::Generate();
	public:

		GUID GetGUID() const { return m_guid; }
		inline virtual std::string GetLabel() { return ""; }

		//SERIALIZATION
		virtual SerializedData Serialize();
		virtual void Deserialize(SerializedData& data);
		inline void DeserializeFromFile(std::string absolute_path) { DeserializeFromFile(std::filesystem::path(absolute_path)); }
		void DeserializeFromFile(std::filesystem::path absolute_path);
		inline void SerializeToFile(std::string absolute_path) { SerializeToFile(std::filesystem::path(absolute_path)); }
		void SerializeToFile(std::filesystem::path absolute_path);
		//DESERIALIZATION
		SerializedData* mostRecentData = nullptr;
		ISerializable(SerializedData& data);
		ISerializable();
		~ISerializable();

		//INSPECTOR + SERIALIZATION
		std::vector<IAutoSerializer*> properties;
		void RegisterProperty(IAutoSerializer*);
		void UnRegisterProperty(IAutoSerializer*);
	};
}