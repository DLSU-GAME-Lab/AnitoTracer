#pragma once

#include <glaze/glaze.hpp>
#include "IAsset.hpp"

// =========================================================================
// GLAZE METADATA FOR IAsset
// =========================================================================
namespace glz {

    template <>
    struct meta<gbe::IAsset> {
        using T = gbe::IAsset;
        static constexpr auto value = object(
            "guid", glz::custom<&T::SetGUID, &T::GetGUID>,
            "path", glz::custom<&T::SetPath, &T::GetPath>,
            "meta_path", glz::custom<&T::SetMetaPath, &T::GetMetaPath>,
            "asset_type", glz::custom<&T::SetAssetType, &T::GetAssetType>
        );
    };

} // namespace glz