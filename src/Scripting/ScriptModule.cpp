#include "ScriptModule.hpp"

#include "ScriptComponent.hpp"

#include "Components/Transform.hpp"
#include "HierarchyObject.hpp"

#include <glm/gtc/quaternion.hpp>

#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static_assert(sizeof(anito::Vec3) == sizeof(glm::vec3), "Vec3 must match glm::vec3 layout");

namespace {

Transform* OwnerTransform(void* owner) {
    auto* component = static_cast<ComponentBase*>(owner);
    HierarchyObject* object = component->GetOwner().GetPtr();
    return object ? object->GetComponent<Transform>() : nullptr;
}

void HostLog(const char* message) { std::cerr << "[Script] " << message << std::endl; }

void HostGetPosition(void* owner, anito::Vec3* out) {
    if (Transform* t = OwnerTransform(owner)) {
        const glm::vec3& p = t->GetLocalPosition();
        *out = {p.x, p.y, p.z};
    }
}

void HostSetPosition(void* owner, anito::Vec3 value) {
    if (Transform* t = OwnerTransform(owner)) t->SetPosition({value.x, value.y, value.z});
}

void HostRotate(void* owner, anito::Vec3 axis, float degrees) {
    Transform* t = OwnerTransform(owner);
    if (!t) return;
    const glm::vec3 a{axis.x, axis.y, axis.z};
    const float lengthSq = glm::dot(a, a);
    if (lengthSq < 1e-8f) return;
    const glm::quat delta = glm::angleAxis(glm::radians(degrees), a / std::sqrt(lengthSq));
    t->SetRotation(glm::normalize(delta * t->GetLocalRotation()));
}

const anito::HostAPI kHostAPI = {HostLog, HostGetPosition, HostSetPosition, HostRotate};

} // namespace

ScriptModule& ScriptModule::GetInstance() {
    static ScriptModule instance;
#ifdef ANITO_SCRIPT_MODULE_PATH
    if (instance.m_modulePath.empty()) instance.m_modulePath = ANITO_SCRIPT_MODULE_PATH;
#endif
    return instance;
}

ScriptModule::~ScriptModule() { Unload(); }

const anito::HostAPI* ScriptModule::GetHostAPI() const { return &kHostAPI; }

void ScriptModule::SetModulePath(const std::filesystem::path& path) {
    if (path == m_modulePath) return;
    ReleaseAllInstances();
    Unload();
    m_modulePath = path;
    m_lastWrite = {};
    m_lastPoll = {};
}

void ScriptModule::ReleaseAllInstances() {
    // ReleaseInstance unregisters the component, so iterate a copy.
    const std::set<ScriptComponent*> components = m_components;
    for (ScriptComponent* c : components) c->ReleaseInstance();
}

bool ScriptModule::Poll() {
    using namespace std::chrono_literals;
    const auto now = std::chrono::steady_clock::now();
    if (m_lastPoll != std::chrono::steady_clock::time_point{} && now - m_lastPoll < 500ms) return false;
    m_lastPoll = now;

    if (m_modulePath.empty()) return false;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(m_modulePath, ec)) {
        if (!m_handle) return false;
        ReleaseAllInstances();
        Unload();
        ++m_generation;
        m_lastWrite = {};
        return true;
    }

    const auto writeTime = std::filesystem::last_write_time(m_modulePath, ec);
    if (ec || writeTime == m_lastWrite) return false;

    // Instances must die before their code is unmapped.
    ReleaseAllInstances();
    Unload();

    m_lastWrite = writeTime;
    Load();
    ++m_generation;
    return true;
}

bool ScriptModule::Load() {
    std::error_code ec;
    const std::filesystem::path liveDir = m_modulePath.parent_path() / "live";
    if (m_loadCounter == 0) std::filesystem::remove_all(liveDir, ec); // Leftovers from earlier sessions

    // The original and its PDB stay unlocked so the compiler can overwrite them while the editor runs.
    const std::filesystem::path loadDir = liveDir / std::to_string(++m_loadCounter);
    std::filesystem::create_directories(loadDir, ec);
    m_loadedCopy = loadDir / m_modulePath.filename();
    std::filesystem::copy_file(m_modulePath, m_loadedCopy, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "[Script] Could not copy module: " << ec.message() << std::endl;
        m_lastWrite = {};
        return false;
    }

    std::filesystem::path pdb = m_modulePath;
    pdb.replace_extension(".pdb");
    if (std::filesystem::is_regular_file(pdb, ec)) {
        std::filesystem::copy_file(pdb, loadDir / pdb.filename(), std::filesystem::copy_options::overwrite_existing, ec);
    }

    HMODULE handle = LoadLibraryW(m_loadedCopy.c_str());
    if (!handle) {
        std::cerr << "[Script] LoadLibrary failed (" << GetLastError() << ") for " << m_modulePath.string() << std::endl;
        std::filesystem::remove(m_loadedCopy, ec);
        return false;
    }

    using GetModuleFn = const anito::ModuleInfo* (*)();
    auto getModule = reinterpret_cast<GetModuleFn>(GetProcAddress(handle, "AnitoScript_GetModule"));
    const anito::ModuleInfo* info = getModule ? getModule() : nullptr;
    if (!info || info->abiVersion != ANITO_SCRIPT_ABI_VERSION) {
        std::cerr << "[Script] Module is missing AnitoScript_GetModule or has a different ABI version." << std::endl;
        FreeLibrary(handle);
        std::filesystem::remove(m_loadedCopy, ec);
        return false;
    }

    m_handle = handle;
    m_info = info;
    std::cerr << "[Script] Loaded module with " << info->entryCount << " script(s)." << std::endl;
    return true;
}

void ScriptModule::Unload() {
    if (m_handle) FreeLibrary(static_cast<HMODULE>(m_handle));
    m_handle = nullptr;
    m_info = nullptr;

    if (!m_loadedCopy.empty()) {
        std::error_code ec;
        std::filesystem::remove_all(m_loadedCopy.parent_path(), ec);
        m_loadedCopy.clear();
    }
}

const anito::ScriptEntry* ScriptModule::FindEntry(const ScriptDescriptor& descriptor) const {
    if (!m_info) return nullptr;
    for (uint32_t i = 0; i < m_info->entryCount; ++i) {
        const anito::ScriptEntry& entry = m_info->entries[i];
        if (descriptor.name != entry.name) continue;
        if (entry.fieldCount != descriptor.fields.size()) return nullptr;
        for (uint32_t f = 0; f < entry.fieldCount; ++f) {
            if (static_cast<int32_t>(entry.fieldTypes[f]) != static_cast<int32_t>(descriptor.fields[f].type)) return nullptr;
        }
        return &entry;
    }
    return nullptr;
}
