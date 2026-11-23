#pragma once

#include "IAsset.hpp"
#include "Meta/IAsset_meta.hpp"

#include "TypeConstants.hpp"

#include "glaze/glaze.hpp"

class IAudioClip : public gbe::IAsset{
public:
	IAudioClip() { 
		this->SetAssetType(gbe::AssetType(ASSETTYPE_AUDIO));
	}
	virtual ~IAudioClip() = default;
};

// =========================================================================
// GLAZE METADATA FOR IAsset
// =========================================================================
namespace glz {

    template <>
	struct meta<IAudioClip> {
		using T = IAudioClip;
		static constexpr auto value = meta<gbe::IAsset>::value;
	};
}