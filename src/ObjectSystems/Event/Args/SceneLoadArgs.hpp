#pragma once

#include "EventSystem.hpp"

// 2. Define custom EventArgs with the "name" payload
struct SceneLoadArgs : public gbe::EventArgs {
    std::string name;

    explicit SceneLoadArgs(std::string sceneName)
        : name(std::move(sceneName)) {
    }
};