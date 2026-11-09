#include "AnitoScriptSDK.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cmath>
#include <iostream>
#include <string>

namespace {

anito::Vec3 position{1.0f, 0.0f, 2.0f};
anito::Quat rotation{};
bool correctObject = true;

anito::Vec3 GetPosition(anito::ObjectId id) {
    correctObject = correctObject && id == 42;
    return position;
}
void SetPosition(anito::ObjectId id, anito::Vec3 value) {
    correctObject = correctObject && id == 42;
    position = value;
}
anito::Quat GetRotation(anito::ObjectId id) {
    correctObject = correctObject && id == 42;
    return rotation;
}
void SetRotation(anito::ObjectId id, anito::Quat value) {
    correctObject = correctObject && id == 42;
    rotation = value;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) {
        std::cerr << "Expected sample module path\n";
        return 1;
    }
    HMODULE module = LoadLibraryW(argv[1]);
    if (!module) {
        std::cerr << "Could not load sample module: " << GetLastError() << '\n';
        return 1;
    }
    using GetModule = const anito::ModuleInfo* (*)();
    const auto getModule = reinterpret_cast<GetModule>(GetProcAddress(module, "AnitoScript_GetModule"));
    const auto* info = getModule ? getModule() : nullptr;
    if (!info || info->abiVersion != ANITO_SCRIPT_ABI_VERSION || info->entryCount != 1 ||
        info->entries[0].fieldCount != 7) {
        std::cerr << "Invalid sample module metadata\n";
        FreeLibrary(module);
        return 1;
    }

    anito::HostAPI host{};
    host.transform.TransformGetPosition = GetPosition;
    host.transform.TransformSetPosition = SetPosition;
    host.transform.TransformGetLocalRotation = GetRotation;
    host.transform.TransformSetLocalRotation = SetRotation;
    float speed = 45.0f, height = 0.5f, frequency = 1.0f;
    anito::Vec3 axis{0.0f, 1.0f, 0.0f};
    bool enabled = true;
    std::string tag = "sample";
    int count = 0;
    void* fields[] = {&speed, &axis, &height, &frequency, &enabled, &tag, &count};
    anito::ScriptContext context{&host, 42, fields};
    const auto& entry = info->entries[0];
    auto* instance = entry.create(context);
    instance->OnUpdate(1.0f);
    const bool invoked = instance->Invoke("resetTime");
    const bool valid = correctObject && invoked && count == 1 && position.x == 1.0f &&
        position.z == 2.0f && std::abs(position.y - std::cos(1.0f) * height) < 0.0001f &&
        std::abs(rotation.y) > 0.1f;
    entry.destroy(instance);
    FreeLibrary(module);
    if (!valid) std::cerr << "Sample module did not invoke the expected host callbacks\n";
    return valid ? 0 : 1;
}
