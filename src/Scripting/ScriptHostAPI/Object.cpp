#include "Bindings/Object.hpp"
#include "Common.hpp"

#include "ScriptComponent.hpp"
#include "Components/Physics/Collider.hpp"
#include "Components/Physics/RigidBody.hpp"
#include "HierarchyManager.hpp"

#include <cstring>
#include <memory>
#include <string>
#include <typeinfo>

namespace ScriptHost::Bindings::object {
using detail::GetComponent;
using detail::GetId;
using detail::GetObject;
using detail::TransientString;

namespace {

std::string ComponentLabel(const ComponentBase* component) {
    if (const auto* script = dynamic_cast<const ScriptComponent*>(component)) return script->GetScriptName();
    std::string name = typeid(*component).name();
    for (const char* prefix : {"class ", "struct "}) {
        if (name.rfind(prefix, 0) == 0) name.erase(0, std::strlen(prefix));
    }
    return name;
}

}

bool IsValid(anito::ObjectId id) {
    return GetObject(id) != nullptr;
}

const char* GetName(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    return object ? TransientString(object->GetName()) : "";
}

void SetName(anito::ObjectId id, const char* name) {
    if (HierarchyObject* object = GetObject(id)) object->SetName(name ? name : "");
}

anito::ObjectId GetParent(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    return object ? object->GetParent().GetID() : 0;
}

anito::ObjectId GetRoot(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    while (object) {
        HierarchyObject* parent = object->GetParent().GetPtr();
        if (!parent) break;
        object = parent;
    }
    return GetId(object);
}

void SetParent(anito::ObjectId id, anito::ObjectId parent) {
    if (!GetObject(id)) return;
    HierarchyManager::GetInstance().ReparentObject(HierarchyObject::Ref(id), HierarchyObject::Ref(parent));
}

int32_t GetChildCount(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    return object ? static_cast<int32_t>(object->GetChildren().size()) : 0;
}

anito::ObjectId GetChild(anito::ObjectId id, int32_t index) {
    HierarchyObject* object = GetObject(id);
    if (!object || index < 0 || static_cast<size_t>(index) >= object->GetChildren().size()) return 0;
    return GetId(object->GetChildren()[static_cast<size_t>(index)].get());
}

anito::ObjectId FindChild(anito::ObjectId id, const char* name) {
    HierarchyObject* object = GetObject(id);
    if (!object || !name) return 0;
    for (const auto& child : object->GetChildren()) {
        if (child && child->GetName() == name) return GetId(child.get());
    }
    return 0;
}

int32_t GetComponentCount(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    return object ? static_cast<int32_t>(object->GetComponents().size()) : 0;
}

const char* GetComponentName(anito::ObjectId id, int32_t index) {
    HierarchyObject* object = GetObject(id);
    if (!object || index < 0 || static_cast<size_t>(index) >= object->GetComponents().size()) return "";
    return TransientString(ComponentLabel(object->GetComponents()[static_cast<size_t>(index)].get()));
}

bool HasComponent(anito::ObjectId id, const char* typeName) {
    HierarchyObject* object = GetObject(id);
    if (!object || !typeName) return false;
    for (const auto& component : object->GetComponents()) {
        if (component && ComponentLabel(component.get()) == typeName) return true;
    }
    return false;
}

void AddRigidbody(anito::ObjectId id, float mass) {
    if (!GetObject(id) || GetComponent<RigidBody>(id)) return;
    HierarchyManager::GetInstance().AddComponentToObject(
        HierarchyObject::Ref(id), std::make_unique<RigidBody>(HierarchyObject::Ref(id), mass));
}

void AddCollider(anito::ObjectId id, int32_t shape, anito::Vec3 params) {
    if (!GetObject(id)) return;
    IPhysicsEngine::ShapeParams shapeParams;
    shapeParams.v = detail::ToGlm(params);
    const auto type = shape == 1 ? IPhysicsEngine::ShapeType::Sphere
                    : shape == 2 ? IPhysicsEngine::ShapeType::Capsule
                                 : IPhysicsEngine::ShapeType::Box;
    HierarchyManager::GetInstance().AddComponentToObject(
        HierarchyObject::Ref(id), std::make_unique<Collider>(HierarchyObject::Ref(id), type, shapeParams));
}

void Destroy(anito::ObjectId id) {
    if (GetObject(id)) HierarchyManager::GetInstance().QueueObjectDeletion(HierarchyObject::Ref(id));
}

anito::ObjectAPI GetAPI() {
    return {IsValid, GetName, SetName, GetParent, GetRoot, SetParent, GetChildCount, GetChild,
            FindChild, GetComponentCount, GetComponentName, HasComponent, AddRigidbody, AddCollider, Destroy};
}
} // namespace ScriptHost::Bindings::object
