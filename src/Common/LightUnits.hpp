#pragma once

#include <algorithm>
#include <cmath>

// Photometric units for lights.
//
// The scene is rendered pre-exposed: a light's value is multiplied by the camera's exposure when it is gathered
// (HierarchyManager::GatherLightData), so the 16-bit float scene buffer holds values near 1 whatever the lamp. A
// 100,000 lux sun would not fit in it unscaled, and a glossy highlight of it even less.
namespace LightUnits {

    inline constexpr float kPi = 3.14159265358979f;

    // The scene value of one lux (or one cd/m2) when the camera is set to EV100 (ISO 100): 1 / (1.2 * 2^EV100)
    inline float ExposureScale(float ev100) {
        return 1.0f / (1.2f * std::exp2(ev100));
    }

    // Luminous intensity of a point or spot light that sends out `lumens` in every direction
    inline float LumensToCandela(float lumens) {
        return lumens / (4.0f * kPi);
    }

    // Radiance (cd/m2) of a one-sided rectangular emitter of the given area (m2) that sends out `lumens`
    inline float LumensToRadiance(float lumens, float area) {
        return lumens / (kPi * std::max(area, 0.0001f));
    }

    // Illuminance at the surface of a point light at a given distance, in lux
    inline float CandelaToLux(float candela, float distance) {
        return candela / std::max(distance * distance, 0.0001f);
    }
}
