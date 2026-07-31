#pragma once

#include <glaze/glaze.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// =========================================================================
// GLAZE METADATA FOR GLM TYPES
// Teaches Glaze how to serialize/deserialize GLM vectors, quaternions, & matrices
// =========================================================================
namespace glz {

    // --- GLM Vectors (vec2, vec3, vec4) ---
    template <>
    struct meta<glm::vec2> {
        using V = glm::vec2;
        static constexpr auto value = object(
            "x", &V::x,
            "y", &V::y
        );
    };

    template <>
    struct meta<glm::vec3> {
        using V = glm::vec3;
        static constexpr auto value = object(
            "x", &V::x,
            "y", &V::y,
            "z", &V::z
        );
    };

    template <typename T, glm::qualifier Q>
    struct meta<glm::vec<4, T, Q>> {
        using V = glm::vec<4, T, Q>;
        static constexpr auto value = object(
            "x", &V::x,
            "y", &V::y,
            "z", &V::z,
            "w", &V::w
        );
    };

    // --- GLM Quaternion (qua / quat) ---
    template <typename T, glm::qualifier Q>
    struct meta<glm::qua<T, Q>> {
        using QType = glm::qua<T, Q>;
        static constexpr auto value = object(
            "w", &QType::w,
            "x", &QType::x,
            "y", &QType::y,
            "z", &QType::z
        );
    };

    // --- GLM Matrices (mat2, mat3, mat4) ---
    template <typename T, glm::qualifier Q>
    struct meta<glm::mat<2, 2, T, Q>> {
        using M = glm::mat<2, 2, T, Q>;
        static constexpr auto value = object(
            "col0", [](M& m) -> auto& { return m[0]; },
            "col1", [](M& m) -> auto& { return m[1]; }
        );
    };

    template <typename T, glm::qualifier Q>
    struct meta<glm::mat<3, 3, T, Q>> {
        using M = glm::mat<3, 3, T, Q>;
        static constexpr auto value = object(
            "col0", [](M& m) -> auto& { return m[0]; },
            "col1", [](M& m) -> auto& { return m[1]; },
            "col2", [](M& m) -> auto& { return m[2]; }
        );
    };

    template <typename T, glm::qualifier Q>
    struct meta<glm::mat<4, 4, T, Q>> {
        using M = glm::mat<4, 4, T, Q>;
        static constexpr auto value = object(
            "col0", [](M& m) -> auto& { return m[0]; },
            "col1", [](M& m) -> auto& { return m[1]; },
            "col2", [](M& m) -> auto& { return m[2]; },
            "col3", [](M& m) -> auto& { return m[3]; }
        );
    };

} // namespace glz