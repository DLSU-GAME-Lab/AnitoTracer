#pragma once

#include "AutoSerializer.hpp"
#include "SerializedData.hpp"

#include <list>
#include <functional>

namespace gbe {
	class ISerializable {
	public:

		//SERIALIZATION
		virtual SerializedData Serialize();
		virtual void Deserialize(SerializedData& data);
		void DeserializeFromFile(std::string absolute_path);
		void SerializeToFile(std::string absolute_path);
		//DESERIALIZATION
		SerializedData* mostRecentData = nullptr;
		ISerializable(SerializedData& data);
		ISerializable();

		//INSPECTOR + SERIALIZATION
		std::vector<IAutoSerializer*> properties;
		void RegisterProperty(IAutoSerializer*);
		void UnRegisterProperty(IAutoSerializer*);
	};
}