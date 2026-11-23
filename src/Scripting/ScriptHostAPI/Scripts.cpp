#include "Bindings/Scripts.hpp"
#include "Common.hpp"

#include "ScriptComponent.hpp"
#include "HierarchyManager.hpp"

#include <memory>
#include <vector>

namespace ScriptHost::Bindings::scripts {
using detail::GetObject;

namespace {

ScriptComponent* Find(anito::ObjectId id, const char* script) {
    HierarchyObject* object = GetObject(id);
    if (!object || !script) return nullptr;
    for (const auto& component : object->GetComponents()) {
        auto* scriptComponent = dynamic_cast<ScriptComponent*>(component.get());
        if (scriptComponent && scriptComponent->GetScriptName() == script) return scriptComponent;
    }
    return nullptr;
}

}

bool Has(anito::ObjectId id, const char* script) {
    return Find(id, script) != nullptr;
}

void Add(anito::ObjectId id, const char* script) {
    if (!GetObject(id) || !script || Find(id, script)) return;
    auto component = std::make_unique<ScriptComponent>(HierarchyObject::Ref(id));
    component->BindScript(script);
    HierarchyManager::GetInstance().AddComponentToObject(HierarchyObject::Ref(id), std::move(component));
}

bool Call(anito::ObjectId id, const char* script, const char* method, const anito::Variant* args, uint32_t argc,
          anito::Variant* ret) {
    ScriptComponent* component = Find(id, script);
    return component && method && component->CallMethod(method, args, argc, ret);
}

void SendMessage(anito::ObjectId id, const char* method, const anito::Variant* args, uint32_t argc) {
    HierarchyObject* object = GetObject(id);
    if (!object || !method) return;

    std::vector<ScriptComponent*> targets;
    for (const auto& component : object->GetComponents()) {
        if (auto* scriptComponent = dynamic_cast<ScriptComponent*>(component.get()))
            targets.push_back(scriptComponent);
    }

    anito::Variant ignored;
    for (ScriptComponent* target : targets) target->CallMethod(method, args, argc, &ignored);
}

anito::ScriptAPI GetAPI() {
    return {Has, Add, Call, SendMessage};
}
} // namespace ScriptHost::Bindings::scripts
