#pragma once

#include "AnitoScriptSDK.hpp"
#include "ScriptDescriptor.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

class ScriptComponent;

// Loads the compiled script DLL and hot-reloads it when the file changes.
class ScriptModule {
public:
    static ScriptModule& GetInstance();

    void SetModulePath(const std::filesystem::path& path) { m_modulePath = path; }

    // Throttled; reloads when the DLL's write time changes. Returns true if a (re)load happened.
    bool Poll();

    // Bumped on every load so components know their instances are stale.
    uint64_t Generation() const { return m_generation; }

    // Returns null if the module lacks the script or its field layout differs from the descriptor.
    const anito::ScriptEntry* FindEntry(const ScriptDescriptor& descriptor) const;

    const anito::HostAPI* GetHostAPI() const;

    void Register(ScriptComponent* component) { m_components.insert(component); }
    void Unregister(ScriptComponent* component) { m_components.erase(component); }

    ~ScriptModule();

private:
    ScriptModule() = default;

    bool Load();
    void Unload();
    void ReleaseAllInstances();

    std::filesystem::path m_modulePath;
    std::filesystem::path m_loadedCopy;
    std::filesystem::file_time_type m_lastWrite{};
    std::chrono::steady_clock::time_point m_lastPoll{};
    void* m_handle = nullptr;
    const anito::ModuleInfo* m_info = nullptr;
    uint64_t m_generation = 0;
    uint64_t m_loadCounter = 0;
    std::set<ScriptComponent*> m_components;
};
