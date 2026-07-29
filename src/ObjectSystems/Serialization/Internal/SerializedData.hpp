#pragma once

#include <unordered_map>

namespace gbe {
	struct SerializedData {
		std::unordered_map<std::string, std::string> serialized_variables;
	};
}