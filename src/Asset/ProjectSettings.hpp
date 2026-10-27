#pragma once

#include <array>

struct ProjectLightingSettings
{
    float shadow_bias = 0.015f;
    float ambient_multiplier = 1.0f;
    std::array<float, 3> ambient_color{0.03f, 0.03f, 0.03f};
};

struct ProjectDebugSettings
{
    bool show_physics_collider_wireframes = false;
};

struct ProjectPhysicsSettings
{
    bool gravity_enabled = true;
    std::array<float, 3> gravity{0.0f, -9.81f, 0.0f};
};

struct ProjectSettings
{
    ProjectLightingSettings lighting;
    ProjectDebugSettings debug;
    ProjectPhysicsSettings physics;
};