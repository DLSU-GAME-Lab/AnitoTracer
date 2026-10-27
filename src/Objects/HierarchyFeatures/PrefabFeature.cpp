#include "PrefabFeature.hpp"

#include "../HierarchyManager.hpp"
#include "../Components/Camera.hpp"
#include "File/Parser.hpp"
#include "SerializedData.hpp"
#include "SceneRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_map>

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

    void RemapGuids(gbe::SerializedData &data)
    {
        std::unordered_map<gbe::GUID, gbe::GUID> guidMap;

        // 1. Identify all GUID definitions inside the prefab data
        for (const auto &[key, value] : data.serialized_variables)
        {
            if (IsGuidSerializationKey(key))
            {
                gbe::GUID oldGuid = gbe::GUID::FromString(value);
                if (oldGuid && guidMap.find(oldGuid) == guidMap.end())
                {
                    gbe::GUID newGuid;
                    do
                    {
                        newGuid = gbe::GUID::Generate();
                    } while (gbe::SceneRegistry::GetInstance().Resolve<gbe::ISerializable>(newGuid) != nullptr);

                    guidMap[oldGuid] = newGuid;
                }
            }
        }

        if (guidMap.empty())
        {
            return;
        }

        // 2. Remap definitions and ObjectRef references to new GUIDs
        for (auto &[key, value] : data.serialized_variables)
        {
            if (IsGuidSerializationKey(key))
            {
                gbe::GUID oldGuid = gbe::GUID::FromString(value);
                auto it = guidMap.find(oldGuid);
                if (it != guidMap.end())
                {
                    value = it->second.ToString();
                }
            }
            else
            {
                gbe::GUID currentGuid = gbe::GUID::Empty();
                gbe::Parser::PopulateClassStr(currentGuid, value);
                if (currentGuid)
                {
                    auto it = guidMap.find(currentGuid);
                    if (it != guidMap.end())
                    {
                        value = gbe::Parser::ExportClassStr(it->second);
                    }
                }
            }
        }
    }

    void BuildInstanceGuidMap(
        const HierarchyObject *liveObject,
        const gbe::SerializedData &prefabData,
        const std::string &prefix,
        std::unordered_map<gbe::GUID, gbe::GUID> &guidMap)
    {
        if (!liveObject)
            return;

        // Map live root/child object GUID
        std::string objectGuidKey = prefix + "m_guid";
        auto itObj = prefabData.serialized_variables.find(objectGuidKey);
        if (itObj != prefabData.serialized_variables.end())
        {
            gbe::GUID assetGuid = gbe::GUID::FromString(itObj->second);
            if (assetGuid)
            {
                guidMap[assetGuid] = liveObject->GetGUID();
            }
        }

        // Map attached component GUIDs
        const auto &components = liveObject->GetComponents();
        for (size_t i = 0; i < components.size(); ++i)
        {
            if (!components[i])
                continue;
            std::string compGuidKey = prefix + "m_components[" + std::to_string(i) + "].m_guid";
            auto itComp = prefabData.serialized_variables.find(compGuidKey);
            if (itComp != prefabData.serialized_variables.end())
            {
                gbe::GUID assetGuid = gbe::GUID::FromString(itComp->second);
                if (assetGuid)
                {
                    guidMap[assetGuid] = components[i]->GetGUID();
                }
            }
        }

        // Recursively map child hierarchy node GUIDs
        const auto &children = liveObject->GetChildren();
        for (size_t j = 0; j < children.size(); ++j)
        {
            if (!children[j])
                continue;
            std::string childPrefix = prefix + "m_children[" + std::to_string(j) + "].";
            BuildInstanceGuidMap(children[j].get(), prefabData, childPrefix, guidMap);
        }
    }

    void RemapGuidsForInstance(
        HierarchyObject *liveObject,
        gbe::SerializedData &prefabData)
    {
        std::unordered_map<gbe::GUID, gbe::GUID> guidMap;

        // 1. Build mapping table from prefab asset GUIDs to the live instance's local GUIDs
        BuildInstanceGuidMap(liveObject, prefabData, "", guidMap);

        // 2. Fall back to generating new GUIDs for any asset elements missing in the live instance
        for (const auto &[key, value] : prefabData.serialized_variables)
        {
            if (IsGuidSerializationKey(key))
            {
                gbe::GUID assetGuid = gbe::GUID::FromString(value);
                if (assetGuid && guidMap.find(assetGuid) == guidMap.end())
                {
                    gbe::GUID newGuid;
                    do
                    {
                        newGuid = gbe::GUID::Generate();
                    } while (gbe::SceneRegistry::GetInstance().Resolve<gbe::ISerializable>(newGuid) != nullptr);

                    guidMap[assetGuid] = newGuid;
                }
            }
        }

        if (guidMap.empty())
            return;

        // 3. Remap definition keys and internal ObjectRef references
        for (auto &[key, value] : prefabData.serialized_variables)
        {
            if (IsGuidSerializationKey(key))
            {
                gbe::GUID assetGuid = gbe::GUID::FromString(value);
                auto it = guidMap.find(assetGuid);
                if (it != guidMap.end())
                {
                    value = it->second.ToString();
                }
            }
            else
            {
                gbe::GUID refGuid = gbe::GUID::Empty();
                gbe::Parser::PopulateClassStr(refGuid, value);
                if (refGuid)
                {
                    auto it = guidMap.find(refGuid);
                    if (it != guidMap.end())
                    {
                        value = gbe::Parser::ExportClassStr(it->second);
                    }
                }
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
        const std::string &storedPath)
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
        const std::filesystem::path &prefabPath)
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
        const std::filesystem::path &prefabPath,
        gbe::SerializedData &outData)
    {
        const std::filesystem::path resolvedPath = prefabPath.is_absolute()
                                                       ? NormalizePath(prefabPath)
                                                       : ResolvePrefabPath(prefabPath.generic_string());

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

        StripPrefabMetadataKeys(outData);
        return true;
    }

    std::filesystem::path BuildDefaultPrefabPath(
        const std::string &objectName)
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
            if (LoadPrefabData(objectPtr->GetPrefabAssetPath(), prefabData))
            {
                StripGuidKeys(prefabData);
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
                SyncPrefabOverridesRecursive(child->getRef());
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
            PrefabFeature::RefreshPrefabInstance(object);
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
            RefreshPrefabInstancesRecursive(childRef);
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
        prefabPath = BuildDefaultPrefabPath(objectPtr->GetName());
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
    StripPrefabMetadataKeys(prefabData);

    prefabData.label = prefabPath.string();
    gbe::Parser::ExportClass(prefabData, prefabPath);

    objectPtr->MutablePrefabAssetPath() = MakeStorablePrefabPath(prefabPath);
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
                                  : ResolvePrefabPath(prefabPath.generic_string());

    gbe::SerializedData prefabData;
    if (!LoadPrefabData(resolvedPath, prefabData))
    {
        return nullptr;
    }

    RemapGuids(prefabData);

    auto newObject = std::make_unique<HierarchyObject>("Prefab Instance");
    newObject->Deserialize(prefabData);
    newObject->MutablePrefabAssetPath() = MakeStorablePrefabPath(resolvedPath);
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

    // TODO: Expose standard spawn offset as a configurable engine setting
    constexpr float kStandardSpawnOffset = 5.0f;

    CameraComponent *activeCamera = HierarchyManager::GetInstance().GetMainCamera();
    if (!activeCamera)
    {
        activeCamera = HierarchyManager::GetInstance().GetEditorCamera();
    }

    if (activeCamera)
    {
        if (HierarchyObject::Ref camOwner = activeCamera->GetOwner())
        {
            if (Transform *camTransform = camOwner.GetPtr()->GetTransform())
            {
                const glm::vec3 camPos = camTransform->GetPosition();
                const glm::quat camRot = camTransform->GetRotation();
                const glm::vec3 camForward = camRot * glm::vec3(0.0f, 0.0f, 1.0f);

                const glm::vec3 spawnPosition = camPos + camForward * kStandardSpawnOffset;

                if (HierarchyObject *instantiatedObj = newRef.GetPtr())
                {
                    if (Transform *objTransform = instantiatedObj->GetTransform())
                    {
                        if (parent)
                        {
                            objTransform->SetWorldPosition(spawnPosition);
                        }
                        else
                        {
                            objTransform->SetPosition(spawnPosition);
                        }
                        objTransform->SyncPhysics();
                    }
                }
            }
        }
    }

    return newRef;
}

bool PrefabFeature::ApplyPrefabToAsset(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr || !objectPtr->IsPrefabInstance())
        return false;

    const std::filesystem::path prefabPath = ResolvePrefabPath(objectPtr->GetPrefabAssetPath());
    if (!IsPrefabFile(prefabPath))
        return false;

    gbe::SerializedData prefabData = objectPtr->Serialize();
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
    if (!LoadPrefabData(objectPtr->GetPrefabAssetPath(), prefabData))
    {
        return false;
    }

    RemapGuidsForInstance(objectPtr, prefabData);

    const std::string prefabAssetPath = objectPtr->GetPrefabAssetPath();
    objectPtr->Deserialize(prefabData);
    objectPtr->MutablePrefabAssetPath() = prefabAssetPath;
    objectPtr->MutablePrefabOverrides().clear();
    if (Transform *t = objectPtr->GetTransform())
        t->SyncPhysics();

    return true;
}

bool PrefabFeature::RefreshPrefabInstance(HierarchyObject::Ref object)
{
    HierarchyObject *objectPtr = object.GetPtr();
    if (!objectPtr || !objectPtr->IsPrefabInstance())
        return false;

    gbe::SerializedData prefabData;
    if (!LoadPrefabData(objectPtr->GetPrefabAssetPath(), prefabData))
    {
        return false;
    }

    RemapGuidsForInstance(objectPtr, prefabData);

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
    if (Transform *t = objectPtr->GetTransform())
        t->SyncPhysics();

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
            SyncPrefabOverridesRecursive(rootNode->getRef());
        }
    }
}

void PrefabFeature::RefreshPrefabInstancesAfterLoad()
{
    for (const auto &rootNode : HierarchyManager::GetInstance().m_rootNodes)
    {
        if (rootNode)
        {
            RefreshPrefabInstancesRecursive(rootNode->getRef());
        }
    }
}