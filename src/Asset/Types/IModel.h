#pragma once

#include "IAsset.hpp"
#include "TypeConstants.hpp"

#include "glaze/glaze.hpp"

class IModel : public gbe::IAsset{
public:
	IModel() { 
		this->SetAssetType(gbe::AssetType(ASSETTYPE_MODEL));
	}
	virtual ~IModel() = default;
};