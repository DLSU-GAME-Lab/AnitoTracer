#include "AssetPipeline.hpp"

#define ASSETTYPE_MODEL "MODEL"
#define ASSETTYPE_TEXTURE "TEXTURE"

AssetPipeline::AssetPipeline() {
	gbe::AssetType::register_value(ASSETTYPE_MODEL);
	gbe::AssetType::register_value(ASSETTYPE_TEXTURE);
}