#pragma once

#include "Objects.hpp"

namespace anito {
// ================================================================================================
// Scene / Camera
// ================================================================================================
struct Scene {
    static GameObject none() { return GameObject{}; }
    static GameObject createEmpty(const std::string& name, GameObject parent) { return GameObject{detail::H().scene.CreateEmpty(name.c_str(), parent.id)}; }
    // kind: "sphere" or "cube".
    static GameObject createPrimitive(const std::string& kind, GameObject parent) { return GameObject{detail::H().scene.CreatePrimitive(kind.c_str(), parent.id)}; }
    static GameObject instantiate(const PrefabAsset& prefab, GameObject parent) {
        return GameObject{prefab.empty() ? 0 : detail::H().scene.Instantiate(prefab.path.c_str(), parent.id)};
    }
    static GameObject find(const std::string& name) { return GameObject{detail::H().scene.Find(name.c_str())}; }
    static GameObject findWithScript(const std::string& scriptName) { return GameObject{detail::H().scene.FindWithScript(scriptName.c_str())}; }
};

struct Camera {
    static GameObject main() { return GameObject{detail::H().scene.GetMainCamera()}; }
};

} // namespace anito
