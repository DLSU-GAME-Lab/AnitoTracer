#pragma once

#include "../HierarchyObject.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

#include "Organization/SingletonMacro.hpp"

class HierarchyManager;
namespace gbe
{
    struct SerializedData;
}

class PrefabFeature final
{
    SINGLETON_MACRO_DEFAULT(PrefabFeature);

public:
    static bool IsPrefabFile(const std::filesystem::path &filepath);

    static std::filesystem::path CreatePrefabAsset(

        HierarchyObject::Ref object,
        std::filesystem::path targetPath = {});

    static HierarchyObject::Ref InstantiatePrefab(

        std::filesystem::path prefabPath,
        HierarchyObject::Ref parent = nullptr);

    static bool ApplyPrefabToAsset(HierarchyObject::Ref object);
    static bool RevertPrefabInstance(HierarchyObject::Ref object);
    static bool RefreshPrefabInstance(HierarchyObject::Ref object);
    static void UnpackPrefabInstance(HierarchyObject::Ref object);

    static void SyncPrefabOverridesBeforeSave();
    static void RefreshPrefabInstancesAfterLoad();
};