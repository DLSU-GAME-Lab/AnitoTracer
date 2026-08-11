#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// A strong type wrapper to tell the Inspector to use ImGui::ColorEdit4
struct Color4 {
    glm::vec4 value;

    // Constructors for easy initialization
    Color4() : value(1.0f) {}
    Color4(float r, float g, float b, float a) : value(r, g, b, a) {}
    Color4(const glm::vec4& v) : value(v) {}

    // Implicit conversions so it behaves identically to glm::vec4 in math!
    operator glm::vec4& () { return value; }
    operator const glm::vec4& () const { return value; }
};

struct Color3 {
    glm::vec3 value;

    // Constructors for easy initialization
    Color3() : value(1.0f) {}
    Color3(float r, float g, float b) : value(r, g, b) {}
    Color3(const glm::vec3& v) : value(v) {}

    // Implicit conversions so it behaves identically to glm::vec3 in math!
    operator glm::vec3& () { return value; }
    operator const glm::vec3& () const { return value; }
};