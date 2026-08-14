#pragma once

#include <unordered_map>
#include <string>

namespace gbe {
	struct SerializedData {
		std::string label = ""; //usually optional, for debugging purposes
		std::unordered_map<std::string, std::string> serialized_variables;
	};
}