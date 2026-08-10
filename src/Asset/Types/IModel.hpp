#pragma once

#include "IAsset.hpp"
#include "Meta/IAsset_meta.hpp"

#include "TypeConstants.hpp"

#include "glaze/glaze.hpp"

class IModel : public gbe::IAsset{
public:
	IModel() { 
		this->SetAssetType(gbe::AssetType(ASSETTYPE_MODEL));
	}
	virtual ~IModel() = default;
};

// =========================================================================
// GLAZE METADATA FOR IAsset
// =========================================================================
namespace glz {

    template <>
	struct meta<IModel> {
		using T = IModel;
		static constexpr auto value = meta<gbe::IAsset>::value;
	};
} // namespace glz