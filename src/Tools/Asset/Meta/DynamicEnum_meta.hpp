#pragma once

#include <glaze/glaze.hpp>
#include "IAsset.hpp"

// =========================================================================
// GLAZE METADATA FOR gbe::DynamicEnum<Tag>
// Teaches Glaze to serialize gbe::DynamicEnum as its underlying string
// =========================================================================
namespace glz {

    template <typename Tag>
    struct meta<gbe::DynamicEnum<Tag>> {
        using T = gbe::DynamicEnum<Tag>;

        static constexpr auto read = [](T& self, const std::string& str) {
            self = T::register_value(str);
            };

        static constexpr auto write = [](const T& self) -> const std::string& {
            return self.str();
            };

        static constexpr auto value = glz::custom<read, write>;
    };

} // namespace glz