#include "Bindings/Time.hpp"

#include <chrono>

namespace {

float g_deltaTime = 0.0f;
float g_fixedDeltaTime = 0.0f;
const auto g_startTime = std::chrono::steady_clock::now();

}

namespace ScriptHost::Bindings::time {

float GetTime() {
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - g_startTime).count();
}

float GetDeltaTime() {
    return g_deltaTime;
}

float GetFixedDeltaTime() {
    return g_fixedDeltaTime;
}

void SetFrameDelta(float deltaTime) {
    g_deltaTime = deltaTime;
}

void SetFixedDelta(float deltaTime) {
    g_fixedDeltaTime = deltaTime;
}

anito::TimeAPI GetAPI() {
    return {GetTime, GetDeltaTime, GetFixedDeltaTime};
}
} // namespace ScriptHost::Bindings::time
