#pragma once

#include "Components.hpp"

namespace anito {
// ================================================================================================
// Input / Time / Gui / Debug
// ================================================================================================
struct Input {
    // True while the named input action (e.g. "INPUT_KEY_THRUST_FORWARD") is held.
    static bool getAction(const std::string& action) { return detail::H().input.GetAction(action.c_str()); }
    static Vec3 mouseDelta() { Vec3 v{}; detail::H().input.GetMouseDelta(&v); return v; }
    static Vec3 mousePosition() { Vec3 v{}; detail::H().input.GetMousePosition(&v); return v; }
};

struct Time {
    static float time() { return detail::H().time.GetTime(); }
    static float deltaTime() { return detail::H().time.GetDeltaTime(); }
    static float fixedDeltaTime() { return detail::H().time.GetFixedDeltaTime(); }
};

// Immediate-mode drawing; only valid inside `on GUI`. Coordinates are screen pixels in the game view.
struct Gui {
    // z < 0 means the point is behind the camera.
    static Vec3 worldToScreen(Vec3 world) { Vec3 v{}; detail::H().gui.WorldToScreen(world, &v); return v; }
    static void fillRect(Vec3 min, Vec3 max, Vec3 rgb, float alpha) { detail::H().gui.FillRect(min, max, rgb, alpha); }
    static void drawRect(Vec3 min, Vec3 max, Vec3 rgb, float alpha, float thickness) { detail::H().gui.DrawRect(min, max, rgb, alpha, thickness); }
};

struct Debug {
    static void log(const std::string& message) { detail::H().Log(message.c_str()); }
};

} // namespace anito
