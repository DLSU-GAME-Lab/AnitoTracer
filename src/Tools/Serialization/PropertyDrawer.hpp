#pragma once
#include <string>

namespace gbe {

    // Primary template (Fallback if no ImGui drawer specialization exists)
    template <typename T>
    struct PropertyDrawer {
        //static bool Draw(const std::string& label, T& target)
		//Force the compiler to throw an error if no specialization exists for the type T
    };

    template <> struct PropertyDrawer<float>;
    template <> struct PropertyDrawer<int>;
    template <> struct PropertyDrawer<bool>;
    template <> struct PropertyDrawer<std::string>;

}