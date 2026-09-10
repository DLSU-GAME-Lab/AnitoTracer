#include "PrefabFeature.hpp"

#include "../HierarchyManager.hpp"
#include "File/Parser.hpp"
#include "SerializedData.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace
{
    constexpr std::string_view kPrefabExtension = ".aprefab";

    std::filesystem::path NormalizePath(const std::filesystem::path &path)
    {
        std::error_code error;
        const auto absolute = std::filesystem::absolute(path, error);
        return error ? path.lexically_normal() : absolute.lexically_normal();
    }

    bool HasCaseInsensitiveExtension(const std::filesystem::path &path, std::string_view extension)
    {
        std::string currentExtension = path.extension().string();
        std::transform(currentExtension.begin(), currentExtension.end(), currentExtension.begin(),
                       [](unsigned char character)
                       { return static_cast<char>(std::tolower(character)); });
        return currentExtension == extension;
    }

    bool IsGuidSerializationKey(const std::string &key)
    {
        return key == "m_guid" ||
               (key.size() > 7 && key.compare(key.size() - 7, 7, ".m_guid") == 0);
    }

    bool IsPrefabMetadataKey(const std::string &key)
    {
        return key == "m_prefabAssetPath" ||
               key.rfind("m_prefabOverrides", 0) == 0 ||
               key.find(".m_prefabAssetPath") != std::string::npos ||
               key.find(".m_prefabOverrides") != std::string::npos;
    }

    void StripGuidKeys(gbe::SerializedData &data)
    {
        for (auto it = data.serialized_variables.begin(); it != data.serialized_variables.end();)
        {
            if (IsGuidSerializationKey(it->first))
            {
                it = data.serialized_variables.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void StripPrefabMetadataKeys(gbe::SerializedData &data)
    {
        for (auto it = data.serialized_variables.begin(); it != data.serialized_variables.end();)
        {
            if (IsPrefabMetadataKey(it->first))
            {
                it = data.serialized_variables.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::string SanitizePrefabFileName(std::string name)
    {
        if (name.empty())
        {
            return "NewPrefab";
        }

        for (char &character : name)
        {
            const bool invalidCharacter =
                character == '<' || character == '>' || character == ':' ||
                character == '"' || character == '/' || character == '\\' ||
                character == '|' || character == '?' || character == '*' ||
                static_cast<unsigned char>(character) < 32;
            if (invalidCharacter)
            {
                character = '_';
            }
        }

        return name;
    }

    std::unordered_map<std::string, std::string> BuildPrefabOverrides(
        const std::unordered_map<std::string, std::string> &prefabData,
        const std::unordered_map<std::string, std::string> &instanceData)
    {
        std::unordered_map<std::string, std::string> overrides;
        for (const auto &[key, value] : instanceData)
        {
            auto it = prefabData.find(key);
            if (it == prefabData.end() || it->second != value)
            {
                overrides.insert_or_assign(key, value);
            }
        }
        return overrides;
    }

    std::filesystem::path ResolvePrefabPath(
        const const std::string &storedPath)
    {
        if (storedPath.empty())
        {
            return {};
        }

        std::filesystem::path prefabPath(storedPath);
        if (prefabPath.is_absolute())
        {
            return NormalizePath(prefabPath);
        }

        const std::filesystem::path sceneFile = HierarchyManager::GetInstance().GetSceneFile();
        const std::filesystem::path sceneDirectory = sceneFile.empty()
                                                         ? std::filesystem::current_path()
                                                         : NormalizePath(sceneFile).parent_path();
        return NormalizePath(sceneDirectory / prefabPath);
    }

    std::string MakeStorablePrefabPath(
        const const std::filesystem::path &prefabPath)
    {
        const std::filesystem::path normalizedPath = NormalizePath(prefabPath);
        const std::filesystem::path sceneFile = HierarchyManager::GetInstance().GetSceneFile();
        if (sceneFile.empty())
        {
            return normalizedPath.generic_string();
        }

        const std::filesystem::path sceneDirectory = NormalizePath(sceneFile).parent_path();
        std::error_code error;
        const std::filesystem::path relativePath = std::filesystem::relative(normalizedPath, sceneDirectory, error);
        if (error || relativePath.empty())
        {
            return normalizedPath.generic_string();
        }

        return relativePath.lexically_normal().generic_string();
    }

    bool LoadPrefabData(
        const const std::filesystem::path &prefabPath,
        gbe::SerializedData &outData)
    {
        const std::filesystem::path resolvedPath = prefabPath.is_absolute()
                                                       ? NormalizePath(prefabPath)
                                                       : ResolvePrefabPath( prefabPath.generic_string());

        if (!PrefabFeature::IsPrefabFile(resolvedPath))
        {
            return false;
        }

        std::error_code error;
        if (!std::filesystem::exists(resolvedPath, error) || error)
        {
            return false;
        }

        outData = {};
        outData.label = resolvedPath.string();
        if (!gbe::Parser::PopulateClass(outData, resolvedPath))
        {
            return false;
        }

        StripGuidKeys(outData);
        StripPrefabMetadataKeys(outData);
        return true;
    }

    std::filesystem::path BuildDefaultPrefabPath(
        const const std::string &objectName)
    {
        const std::filesystem::path sceneFile = HierarchyManager::GetInstance().GetSceneFile();
        const std::filesystem::path sceneDirectory = sceneFile.empty()
                                                         ? std::filesystem::current_path()
                                                         : NormalizePath(sceneFile).parent_path();
        const std::filesystem::path prefabDirectory = sceneDirectory / "Prefabs";

        std::error_code error;
        std::filesystem::create_directories(prefabDirectory, error);

        const std::string baseName = SanitizePrefabFileName(objectName);
        std::filesystem::path candidate = prefabDirectory / (baseName + std::string(kPrefabExtension));
        for (size_t suffix = 1; std::filesystem::exists(candidate); ++suffix)
        {
            candidate = prefabDirectory / (baseName + "_" + std::to_string(suffix) + std::string(kPrefabExtension));
        }
        return NormalizePath(candidate);
    }

    void SyncPrefabOverridesRecursive(HierarchyObject::Ref object)
    {
        HierarchyObject *objectPtr = object.GetPtr();
        if (!objectPtr)
            return;

        if (objectPtr->IsPrefabInstance())
        {
            gbe::SerializedData prefabData;
            if (LoadPrefabData( objectPtr->GetPrefabAssetPath(), prefabData))
            {
                gbe::SerializedData instanceData = objectPtr->Serialize();
                StripGuidKeys(instanceData);
                StripPrefabMetadataKeys(instanceData);
                objectPtr->MutablePrefabOverrides() = BuildPrefabOverrides(
                    prefabData.serialized_variables,
                    instanceData.serialized_variables);
            }
        }

        for (const auto &child : objectPtr->GetChildren())
        {
            if (child)
            {
                SyncPrefabOverridesRecursive( child->getRef());
            }
        }
    }

    void RefreshPrefabInstancesRecursive(HierarchyObject::Ref object)
    {
        HierarchyObject *objectPtr = object.GetPtr();
        if (!objectPtr)
            return;

        if (objectPtr->IsPrefabInstance())
        {
            PrefabFeature::RefreshPrefabInstance( object);
            objectPtr = object.GetPtr();
            if (!objectPtr)
            {
                return;
            }
        }

        std::vector<HierarchyObject::Ref> children;
        children.reserve(objectPtr->GetChildren().size());
        for (const auto &child : objectPtr->GetChildren())
        {
            if (child)
            {
                children.push_back(child->getRef());
            }
        }

        for (const auto &childRef : children)
        {
            RefreshPrefabInstancesRecursive( childRef);
        }
    }
}

bool PrefabFeature::IsPrefabFile(const std::filesystem::path &filepath)
{
    return HasCaseInsensitiveExtension(filepath, kPrefabExtension);
}

std::filesystem::path PrefabFeature::CreatePrefabAsset(

    HierarchyObject::Ref object,
    std::filesystem::path targetPath)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr)
        return {};

    std::filesystem::path prefabPath = targetPath;
    if (prefabPath.empty())
    {
        prefabPath = BuildDefaultPrefabPath( objectPtr->GetName());
    }
    else
    {
        if (!IsPrefabFile(prefabPath))
        {
            prefabPath.replace_extension(kPrefabExtension);
        }

        if (!prefabPath.is_absolute())
        {
            const auto sceneFile = HierarchyManager::GetInstance().GetSceneFile();
            const auto sceneDirectory = sceneFile.empty()
                                            ? std::filesystem::current_path()
                                            : NormalizePath(sceneFile).parent_path();
            prefabPath = sceneDirectory / prefabPath;
        }
        prefabPath = NormalizePath(prefabPath);
    }

    gbe::SerializedData prefabData = objectPtr->Serialize();
    StripGuidKeys(prefabData);
    StripPrefabMetadataKeys(prefabData);

    prefabData.label = prefabPath.string();
    gbe::Parser::ExportClass(prefabData, prefabPath);

    objectPtr->MutablePrefabAssetPath() = MakeStorablePrefabPath( prefabPath);
    objectPtr->MutablePrefabOverrides().clear();

    return prefabPath;
}

HierarchyObject::Ref PrefabFeature::InstantiatePrefab(

    std::filesystem::path prefabPath,
    HierarchyObject::Ref parent)
{
    if (prefabPath.empty())
        return nullptr;

    const auto resolvedPath = prefabPath.is_absolute()
                                  ? NormalizePath(prefabPath)
                                  : ResolvePrefabPath( prefabPath.generic_string());

    gbe::SerializedData prefabData;
    if (!LoadPrefabData( resolvedPath, prefabData))
    {
        return nullptr;
    }

    auto newObject = std::make_unique<HierarchyObject>("Prefab Instance");
    newObject->Deserialize(prefabData);
    newObject->MutablePrefabAssetPath() = MakeStorablePrefabPath( resolvedPath);
    newObject->MutablePrefabOverrides().clear();

    HierarchyObject::Ref newRef = HierarchyManager::GetInstance().AddRootObject(std::move(newObject));
    if (!newRef)
    {
        return nullptr;
    }

    if (parent && !HierarchyManager::GetInstance().ReparentObject(newRef, parent))
    {
        HierarchyManager::GetInstance().RemoveRootObject(newRef);
        return nullptr;
    }

    return newRef;
}

bool PrefabFeature::ApplyPrefabToAsset(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr || !objectPtr->IsPrefabInstance())
        return false;

    const std::filesystem::path prefabPath = ResolvePrefabPath( objectPtr->GetPrefabAssetPath());
    if (!IsPrefabFile(prefabPath))
        return false;

    gbe::SerializedData prefabData = objectPtr->Serialize();
    StripGuidKeys(prefabData);
    StripPrefabMetadataKeys(prefabData);

    prefabData.label = prefabPath.string();
    gbe::Parser::ExportClass(prefabData, prefabPath);
    objectPtr->MutablePrefabOverrides().clear();

    return true;
}

bool PrefabFeature::RevertPrefabInstance(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr || !objectPtr->IsPrefabInstance())
        return false;

    gbe::SerializedData prefabData;
    if (!LoadPrefabData( objectPtr->GetPrefabAssetPath(), prefabData))
    {
        return false;
    }

    const std::string prefabAssetPath = objectPtr->GetPrefabAssetPath();
    objectPtr->Deserialize(prefabData);
    objectPtr->MutablePrefabAssetPath() = prefabAssetPath;
    objectPtr->MutablePrefabOverrides().clear();

    return true;
}

bool PrefabFeature::RefreshPrefabInstance(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr || !objectPtr->IsPrefabInstance())
        return false;

    gbe::SerializedData prefabData;
    if (!LoadPrefabData( objectPtr->GetPrefabAssetPath(), prefabData))
    {
        return false;
    }

    gbe::SerializedData mergedData = prefabData;
    for (const auto &[key, value] : objectPtr->GetPrefabOverrides())
    {
        mergedData.serialized_variables.insert_or_assign(key, value);
    }

    const std::string prefabAssetPath = objectPtr->GetPrefabAssetPath();
    const auto prefabOverrides = objectPtr->GetPrefabOverrides();
    objectPtr->Deserialize(mergedData);
    objectPtr->MutablePrefabAssetPath() = prefabAssetPath;
    objectPtr->MutablePrefabOverrides() = prefabOverrides;

    return true;
}

void PrefabFeature::UnpackPrefabInstance(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr)
        return;

    objectPtr->ClearPrefabLink();
}

void PrefabFeature::SyncPrefabOverridesBeforeSave()
{
    for (const auto &rootNode : HierarchyManager::GetInstance().m_rootNodes)
    {
        if (rootNode)
        {
            SyncPrefabOverridesRecursive( rootNode->getRef());
        }
    }
}

void PrefabFeature::RefreshPrefabInstancesAfterLoad()
{
    for (const auto &rootNode : HierarchyManager::GetInstance().m_rootNodes)
    {
        if (rootNode)
        {
            RefreshPrefabInstancesRecursive( rootNode->getRef());
        }
    }
}
