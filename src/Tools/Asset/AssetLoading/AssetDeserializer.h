#pragma once

#include "../BaseAsset.h"
#include <string>

namespace gbe {
	class AssetDeserializer {
	public:
		IAsset* DeserializeFile(std::string path);
	};
}