#include "Bindings/Scene.hpp"
#include "Common.hpp"

#include "ScriptComponent.hpp"
#include "Components/Camera.hpp"
#include "HierarchyFeatures/PrefabFeature.hpp"
#include "HierarchyManager.hpp"
#include "ObjectFactory.hpp"

#include <iostream>
#include <string>

namespace ScriptHost::Bindings::scene {
using detail::GetObject;

namespace {

anito::ObjectId Attach(HierarchyObject::Ref created, anito::ObjectId parent) {
    if (parent && created.IsValid() && GetObject(parent))
        HierarchyManager::GetInstance().ReparentObject(created, HierarchyObject::Ref(parent));
    return created.GetID();
}

anito::ObjectId FindIn(HierarchyObject* object, const char* name) {
    if (object->GetName() == name) return object->getId();
    for (const auto& child : object->GetChildren()) {
        if (child) {
            if (const anito::ObjectId found = FindIn(child.get(), name)) return found;
        }
    }
    return 0;
}

anito::ObjectId FindScriptIn(HierarchyObject* object, const char* script) {
    for (const auto& component : object->GetComponents()) {
        const auto* scriptComponent = dynamic_cast<const ScriptComponent*>(component.get());
        if (scriptComponent && scriptComponent->GetScriptName() == script) return object->getId();
    }
    for (const auto& child : object->GetChildren()) {
        if (child) {
            if (const anito::ObjectId found = FindScriptIn(child.get(), script)) return found;
        }
    }
    return 0;
}

}

anito::ObjectId CreateEmpty(const char* name, anito::ObjectId parent) {
    return Attach(ObjectFactory::GetInstance().CreateRootObjectWithTransform(name ? name : "GameObject"), parent);
}

anito::ObjectId CreatePrimitive(const char* kind, anito::ObjectId parent) {
    const std::string primitive = kind ? kind : "";
    if (primitive == "sphere")
        return Attach(ObjectFactory::GetInstance().CreateSpherePrimitive("Sphere"), parent);
    if (primitive == "cube")
        return Attach(ObjectFactory::GetInstance().CreateCubePrimitive("Cube"), parent);
    std::cerr << "[Script] Unknown primitive '" << primitive << "' (use \"sphere\" or \"cube\")." << std::endl;
    return 0;
}

anito::ObjectId Instantiate(const char* prefabPath, anito::ObjectId parent) {
    if (!prefabPath || !*prefabPath) return 0;
    return PrefabFeature::InstantiatePrefab(
        prefabPath, parent ? HierarchyObject::Ref(parent) : HierarchyObject::Ref{}).GetID();
}

anito::ObjectId Find(const char* name) {
    if (!name) return 0;
    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        if (root) {
            if (const anito::ObjectId found = FindIn(root.get(), name)) return found;
        }
    }
    return 0;
}

anito::ObjectId FindWithScript(const char* script) {
    if (!script) return 0;
    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        if (root) {
            if (const anito::ObjectId found = FindScriptIn(root.get(), script)) return found;
        }
    }
    return 0;
}

anito::ObjectId GetMainCamera() {
    CameraComponent* camera = HierarchyManager::GetInstance().GetMainCamera();
    return camera ? camera->GetOwner().GetID() : 0;
}

anito::SceneAPI GetAPI() {
    return {CreateEmpty, CreatePrimitive, Instantiate, Find, FindWithScript, GetMainCamera};
}
} // namespace ScriptHost::Bindings::scene
