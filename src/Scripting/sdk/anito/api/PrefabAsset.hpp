#pragma once

#include "Common.hpp"

namespace anito {
// ================================================================================================
// Prefab asset reference (exposed field type)
// ================================================================================================
class PrefabAsset {
public:
    std::string path;
    bool empty() const { return path.empty(); }
};

inline PrefabAsset ResolvePrefabField(void* storage) {
    const char* p = detail::H().field.ResolvePrefab(storage);
    return PrefabAsset{p ? p : ""};
}
} // namespace anito
