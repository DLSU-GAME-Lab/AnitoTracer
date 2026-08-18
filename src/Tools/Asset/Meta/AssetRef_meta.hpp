#pragma once

#include <glaze/glaze.hpp>
#include "AssetRef.hpp"
#include "Meta/GUID_meta.hpp"

// =========================================================================
// GLAZE METADATA FOR gbe::AssetRef<T>
// Serializes any AssetRef<T> transparently as its underlying GUID
// =========================================================================
namespace glz {

    template <typename T>
    struct meta<gbe::AssetRef<T>> {
        using Ref = gbe::AssetRef<T>;

        static constexpr auto read = [](Ref& self, const gbe::GUID& guid) {
            self.SetGUID(guid);
            };

        static constexpr auto write = [](const Ref& self) -> gbe::GUID {
            return self.GetGUID();
            };

        static constexpr auto value = glz::custom<read, write>;
    };

} // namespace glz