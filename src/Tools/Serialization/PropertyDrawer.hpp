#pragma once
#include <string>

namespace gbe {

    // Primary template (Fallback if no ImGui drawer specialization exists)
    template <typename T, typename Enable = void>
    struct PropertyDrawer {
        static bool Draw(const std::string& label, T& target) {
            // Default no-op for headless/game runtime builds
            return false;
        }
    };

    template <> struct PropertyDrawer<float>;
    template <> struct PropertyDrawer<int>;
    template <> struct PropertyDrawer<bool>;
    template <> struct PropertyDrawer<std::string>;

}