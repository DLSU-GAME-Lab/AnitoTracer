#pragma once

#include <glaze/glaze.hpp>
#include "GUID.hpp"

namespace glz {

    template <>
    struct meta<gbe::GUID> {
        using T = gbe::GUID;

        static constexpr auto read = [](T& self, const std::string& str) {
            self = T::FromString(str);
            };

        static constexpr auto write = [](const T& self) -> std::string {
            return self.ToString();
            };

        static constexpr auto value = glz::custom<read, write>;
    };

} // namespace glz