#include "ScriptHostAPI.hpp"

#include "ScriptComponent.hpp"

#include "../AppState.hpp"
#include "Components/Audio/AudioSourceComponent.hpp"
#include "Components/Camera.hpp"
#include "Components/Physics/Collider.hpp"
#include "Components/Physics/RigidBody.hpp"
#include "Components/Transform.hpp"
#include "GUIManager.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "HierarchyFeatures/PrefabFeature.hpp"
#include "HierarchyFeatures/PrefabRef.hpp"
#include "Input/InputMappedBool.hpp"
#include "Input/InputSystem.hpp"
#include "ObjectFactory.hpp"
#include "imgui.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <typeinfo>

static_assert(sizeof(anito::Vec3) == sizeof(glm::vec3), "Vec3 must match glm::vec3 layout");

namespace {

using ObjectId = anito::ObjectId;

// ---- Conversions & lookups -----------------------------------------------------------------------
glm::vec3 ToGlm(anito::Vec3 v) { return {v.x, v.y, v.z}; }
anito::Vec3 FromGlm(const glm::vec3& v) { return {v.x, v.y, v.z}; }
glm::quat ToGlm(anito::Quat q) { return glm::quat(q.w, q.x, q.y, q.z); }
anito::Quat FromGlm(const glm::quat& q) { return {q.x, q.y, q.z, q.w}; }

HierarchyObject* Obj(ObjectId id) { return id ? HierarchyObject::Ref(id).GetPtr() : nullptr; }
Transform* TransformOf(ObjectId id) {
    HierarchyObject* o = Obj(id);
    return o ? o->GetComponent<Transform>() : nullptr;
}
RigidBody* RigidOf(ObjectId id) {
    HierarchyObject* o = Obj(id);
    return o ? o->GetComponent<RigidBody>() : nullptr;
}
CameraComponent* CameraOf(ObjectId id) {
    HierarchyObject* o = Obj(id);
    return o ? o->GetComponent<CameraComponent>() : nullptr;
}
AudioSourceComponent* AudioOf(ObjectId id) {
    HierarchyObject* o = Obj(id);
    return o ? o->GetComponent<AudioSourceComponent>() : nullptr;
}
ObjectId IdOf(const HierarchyObject* o) { return o ? o->getId() : 0; }

// Returned pointers must outlive the call, so strings go through a per-thread buffer.
const char* TransientString(const std::string& s) {
    thread_local std::string buffer;
    buffer = s;
    return buffer.c_str();
}

// ---- Time ------------------------------------------------------------------------------------------
float g_deltaTime = 0.0f;
float g_fixedDeltaTime = 0.0f;
const auto g_startTime = std::chrono::steady_clock::now();

// ---- Transform -------------------------------------------------------------------------------------
namespace transform {

void GetPosition(ObjectId id, anito::Vec3* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetPosition()); }
void SetPosition(ObjectId id, anito::Vec3 v) { if (Transform* t = TransformOf(id)) t->SetWorldPosition(ToGlm(v)); }
void GetLocalPosition(ObjectId id, anito::Vec3* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetLocalPosition()); }
void SetLocalPosition(ObjectId id, anito::Vec3 v) { if (Transform* t = TransformOf(id)) t->SetPosition(ToGlm(v)); }
void GetRotation(ObjectId id, anito::Quat* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetRotation()); }
void SetRotation(ObjectId id, anito::Quat q) { if (Transform* t = TransformOf(id)) t->SetWorldRotation(glm::normalize(ToGlm(q))); }
void GetLocalRotation(ObjectId id, anito::Quat* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetLocalRotation()); }
void SetLocalRotation(ObjectId id, anito::Quat q) { if (Transform* t = TransformOf(id)) t->SetRotation(glm::normalize(ToGlm(q))); }
void GetLocalScale(ObjectId id, anito::Vec3* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetLocalScale()); }
void SetLocalScale(ObjectId id, anito::Vec3 v) { if (Transform* t = TransformOf(id)) t->SetScale(ToGlm(v)); }
void GetLossyScale(ObjectId id, anito::Vec3* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetScale()); }
void GetEulerAngles(ObjectId id, anito::Vec3* out) { if (Transform* t = TransformOf(id)) *out = FromGlm(t->GetEulerAnglesDegrees()); }
void SetEulerAngles(ObjectId id, anito::Vec3 v) { if (Transform* t = TransformOf(id)) t->SetEulerAnglesDegrees(ToGlm(v)); }

} // namespace transform

// ---- Physics ---------------------------------------------------------------------------------------
namespace physics {

bool HasRigidbody(ObjectId id) { return RigidOf(id) != nullptr; }
bool IsSimulated(ObjectId id) { RigidBody* r = RigidOf(id); return r && r->GetBody(); }
void GetVelocity(ObjectId id, anito::Vec3* out) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) *out = FromGlm(r->GetVelocity()); }
void SetVelocity(ObjectId id, anito::Vec3 v) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) r->SetVelocity(ToGlm(v)); }
void GetAngularVelocity(ObjectId id, anito::Vec3* out) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) *out = FromGlm(r->GetAngularVelocity()); }
void SetAngularVelocity(ObjectId id, anito::Vec3 v) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) r->SetAngularVelocity(ToGlm(v)); }
void AddForce(ObjectId id, anito::Vec3 v) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) r->ApplyForce(ToGlm(v)); }
void AddTorque(ObjectId id, anito::Vec3 v) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) r->ApplyTorque(ToGlm(v)); }
void AddImpulse(ObjectId id, anito::Vec3 v) { if (RigidBody* r = RigidOf(id); r && r->GetBody()) r->ApplyImpulse(ToGlm(v)); }
float GetMass(ObjectId id) { RigidBody* r = RigidOf(id); return r ? r->GetMass() : 0.0f; }
void SetMass(ObjectId id, float m) { if (RigidBody* r = RigidOf(id)) r->SetMass(m); }
void GetBodyRotation(ObjectId id, anito::Quat* out) {
    if (RigidBody* r = RigidOf(id); r && r->GetBody()) *out = FromGlm(r->GetBody()->GetRotation());
}
void Teleport(ObjectId id, anito::Vec3 p, anito::Quat q) { if (RigidBody* r = RigidOf(id)) r->Teleport(ToGlm(p), ToGlm(q)); }
void SetLockRotation(ObjectId id, bool x, bool y, bool z) {
    if (RigidBody* r = RigidOf(id)) {
        r->SetLockRotationX(x);
        r->SetLockRotationY(y);
        r->SetLockRotationZ(z);
    }
}
bool HasCollider(ObjectId id) { HierarchyObject* o = Obj(id); return o && o->GetComponent<Collider>(); }
void GetGravity(anito::Vec3* out) { *out = FromGlm(PhysicsEngine::GetInstance().Get().GetGravity()); }
void SetGravity(anito::Vec3 g) { PhysicsEngine::GetInstance().Get().SetGravity(ToGlm(g)); }

void Raycast(anito::Vec3 origin, anito::Vec3 direction, float maxDistance, ObjectId ignore, anito::RaycastResult* out) {
    *out = {};
    RigidBody* rigidBody = RigidOf(ignore);
    if (!rigidBody) return;
    const PhysicsRaycastHit hit = rigidBody->Raycast(ToGlm(origin), ToGlm(direction), maxDistance);
    out->hit = hit.hit ? 1 : 0;
    out->point = FromGlm(hit.point);
    out->object = hit.object;
}

} // namespace physics

// ---- Camera ----------------------------------------------------------------------------------------
namespace camera {

bool HasCamera(ObjectId id) { return CameraOf(id) != nullptr; }
float GetFOV(ObjectId id) { CameraComponent* c = CameraOf(id); return c ? c->GetFOV() : 0.0f; }
void SetFOV(ObjectId id, float value) { if (CameraComponent* c = CameraOf(id)) c->SetFOV(value); }
float GetAspect(ObjectId id) { CameraComponent* c = CameraOf(id); return c ? c->GetAspect() : 0.0f; }
void SetAspect(ObjectId id, float value) { if (CameraComponent* c = CameraOf(id)) c->SetAspect(value); }
float GetNearPlane(ObjectId id) { CameraComponent* c = CameraOf(id); return c ? c->GetNearPlane() : 0.0f; }
void SetNearPlane(ObjectId id, float value) { if (CameraComponent* c = CameraOf(id)) c->SetNearPlane(value); }
float GetFarPlane(ObjectId id) { CameraComponent* c = CameraOf(id); return c ? c->GetFarPlane() : 0.0f; }
void SetFarPlane(ObjectId id, float value) { if (CameraComponent* c = CameraOf(id)) c->SetFarPlane(value); }

} // namespace camera

// ---- Audio -----------------------------------------------------------------------------------------
namespace audio {

bool HasAudioSource(ObjectId id) { return AudioOf(id) != nullptr; }
bool HasClip(ObjectId id) { AudioSourceComponent* a = AudioOf(id); return a && a->HasAudioClip(); }
void Play(ObjectId id) { if (auto* a = AudioOf(id)) a->Play(); }
void Stop(ObjectId id) { if (auto* a = AudioOf(id)) a->Stop(); }
void PlayDetached(ObjectId id) { if (auto* a = AudioOf(id)) a->PlayDetached(); }
float GetVolume(ObjectId id) { auto* a = AudioOf(id); return a ? a->GetVolume() : 0.0f; }
void SetVolume(ObjectId id, float v) { if (auto* a = AudioOf(id)) a->SetVolume(v); }
bool GetLoop(ObjectId id) { auto* a = AudioOf(id); return a && a->GetLoop(); }
void SetLoop(ObjectId id, bool v) { if (auto* a = AudioOf(id)) a->SetLoop(v); }
float GetPitch(ObjectId id) { auto* a = AudioOf(id); return a ? a->GetPitch() : 1.0f; }
void SetPitch(ObjectId id, float v) { if (auto* a = AudioOf(id)) a->SetPitch(v); }
bool GetSpatialized(ObjectId id) { auto* a = AudioOf(id); return a && a->GetSpatialized(); }
void SetSpatialized(ObjectId id, bool v) { if (auto* a = AudioOf(id)) a->SetSpatialized(v); }
void SetDistance(ObjectId id, float lo, float hi) { if (auto* a = AudioOf(id)) a->SetDistance(lo, hi); }

} // namespace audio

// ---- Object ----------------------------------------------------------------------------------------
namespace object {

bool IsValid(ObjectId id) { return Obj(id) != nullptr; }
const char* GetName(ObjectId id) { HierarchyObject* o = Obj(id); return o ? TransientString(o->GetName()) : ""; }
void SetName(ObjectId id, const char* name) { if (HierarchyObject* o = Obj(id)) o->SetName(name ? name : ""); }
ObjectId GetParent(ObjectId id) { HierarchyObject* o = Obj(id); return o ? o->GetParent().GetID() : 0; }

ObjectId GetRoot(ObjectId id) {
    HierarchyObject* o = Obj(id);
    while (o) {
        HierarchyObject* parent = o->GetParent().GetPtr();
        if (!parent) break;
        o = parent;
    }
    return IdOf(o);
}

void SetParent(ObjectId id, ObjectId parent) {
    if (!Obj(id)) return;
    HierarchyManager::GetInstance().ReparentObject(HierarchyObject::Ref(id), HierarchyObject::Ref(parent));
}

int32_t GetChildCount(ObjectId id) { HierarchyObject* o = Obj(id); return o ? static_cast<int32_t>(o->GetChildren().size()) : 0; }

ObjectId GetChild(ObjectId id, int32_t index) {
    HierarchyObject* o = Obj(id);
    if (!o || index < 0 || static_cast<size_t>(index) >= o->GetChildren().size()) return 0;
    return IdOf(o->GetChildren()[static_cast<size_t>(index)].get());
}

ObjectId FindChild(ObjectId id, const char* name) {
    HierarchyObject* o = Obj(id);
    if (!o || !name) return 0;
    for (const auto& child : o->GetChildren()) {
        if (child && child->GetName() == name) return IdOf(child.get());
    }
    return 0;
}

std::string ComponentLabel(const ComponentBase* c) {
    if (const auto* script = dynamic_cast<const ScriptComponent*>(c)) return script->GetScriptName();
    std::string name = typeid(*c).name();
    for (const char* prefix : {"class ", "struct "}) {
        if (name.rfind(prefix, 0) == 0) name.erase(0, std::strlen(prefix));
    }
    return name;
}

int32_t GetComponentCount(ObjectId id) { HierarchyObject* o = Obj(id); return o ? static_cast<int32_t>(o->GetComponents().size()) : 0; }

const char* GetComponentName(ObjectId id, int32_t index) {
    HierarchyObject* o = Obj(id);
    if (!o || index < 0 || static_cast<size_t>(index) >= o->GetComponents().size()) return "";
    return TransientString(ComponentLabel(o->GetComponents()[static_cast<size_t>(index)].get()));
}

bool HasComponent(ObjectId id, const char* typeName) {
    HierarchyObject* o = Obj(id);
    if (!o || !typeName) return false;
    for (const auto& c : o->GetComponents()) {
        if (c && ComponentLabel(c.get()) == typeName) return true;
    }
    return false;
}

void AddRigidbody(ObjectId id, float mass) {
    if (!Obj(id) || RigidOf(id)) return;
    HierarchyManager::GetInstance().AddComponentToObject(HierarchyObject::Ref(id), std::make_unique<RigidBody>(HierarchyObject::Ref(id), mass));
}

void AddCollider(ObjectId id, int32_t shape, anito::Vec3 params) {
    if (!Obj(id)) return;
    IPhysicsEngine::ShapeParams shapeParams;
    shapeParams.v = ToGlm(params);
    const auto type = shape == 1 ? IPhysicsEngine::ShapeType::Sphere
                    : shape == 2 ? IPhysicsEngine::ShapeType::Capsule
                                 : IPhysicsEngine::ShapeType::Box;
    HierarchyManager::GetInstance().AddComponentToObject(HierarchyObject::Ref(id), std::make_unique<Collider>(HierarchyObject::Ref(id), type, shapeParams));
}

void Destroy(ObjectId id) { if (Obj(id)) HierarchyManager::GetInstance().QueueObjectDeletion(HierarchyObject::Ref(id)); }

} // namespace object

// ---- Scene -----------------------------------------------------------------------------------------
namespace scene {

ObjectId Attach(HierarchyObject::Ref created, ObjectId parent) {
    if (parent && created.IsValid() && Obj(parent)) HierarchyManager::GetInstance().ReparentObject(created, HierarchyObject::Ref(parent));
    return created.GetID();
}

ObjectId CreateEmpty(const char* name, ObjectId parent) {
    return Attach(ObjectFactory::GetInstance().CreateRootObjectWithTransform(name ? name : "GameObject"), parent);
}

ObjectId CreatePrimitive(const char* kind, ObjectId parent) {
    const std::string k = kind ? kind : "";
    if (k == "sphere") return Attach(ObjectFactory::GetInstance().CreateSpherePrimitive("Sphere"), parent);
    if (k == "cube") return Attach(ObjectFactory::GetInstance().CreateCubePrimitive("Cube"), parent);
    std::cerr << "[Script] Unknown primitive '" << k << "' (use \"sphere\" or \"cube\")." << std::endl;
    return 0;
}

ObjectId Instantiate(const char* prefabPath, ObjectId parent) {
    if (!prefabPath || !*prefabPath) return 0;
    return PrefabFeature::InstantiatePrefab(prefabPath, parent ? HierarchyObject::Ref(parent) : HierarchyObject::Ref{}).GetID();
}

ObjectId FindIn(HierarchyObject* o, const char* name) {
    if (o->GetName() == name) return o->getId();
    for (const auto& child : o->GetChildren()) {
        if (child) if (ObjectId found = FindIn(child.get(), name)) return found;
    }
    return 0;
}

ObjectId Find(const char* name) {
    if (!name) return 0;
    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        if (root) if (ObjectId found = FindIn(root.get(), name)) return found;
    }
    return 0;
}

ObjectId FindScriptIn(HierarchyObject* o, const char* script) {
    for (const auto& c : o->GetComponents()) {
        const auto* sc = dynamic_cast<const ScriptComponent*>(c.get());
        if (sc && sc->GetScriptName() == script) return o->getId();
    }
    for (const auto& child : o->GetChildren()) {
        if (child) if (ObjectId found = FindScriptIn(child.get(), script)) return found;
    }
    return 0;
}

ObjectId FindWithScript(const char* script) {
    if (!script) return 0;
    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        if (root) if (ObjectId found = FindScriptIn(root.get(), script)) return found;
    }
    return 0;
}

ObjectId GetMainCamera() {
    CameraComponent* camera = HierarchyManager::GetInstance().GetMainCamera();
    return camera ? camera->GetOwner().GetID() : 0;
}

} // namespace scene

// ---- Scripts ---------------------------------------------------------------------------------------
namespace scripts {

ScriptComponent* Find(ObjectId id, const char* script) {
    HierarchyObject* o = Obj(id);
    if (!o || !script) return nullptr;
    for (const auto& c : o->GetComponents()) {
        auto* sc = dynamic_cast<ScriptComponent*>(c.get());
        if (sc && sc->GetScriptName() == script) return sc;
    }
    return nullptr;
}

bool Has(ObjectId id, const char* script) { return Find(id, script) != nullptr; }

void Add(ObjectId id, const char* script) {
    if (!Obj(id) || !script || Find(id, script)) return;
    auto component = std::make_unique<ScriptComponent>(HierarchyObject::Ref(id));
    component->BindScript(script);
    HierarchyManager::GetInstance().AddComponentToObject(HierarchyObject::Ref(id), std::move(component));
}

bool Call(ObjectId id, const char* script, const char* method, const anito::Variant* args, uint32_t argc, anito::Variant* ret) {
    ScriptComponent* sc = Find(id, script);
    return sc && method && sc->CallMethod(method, args, argc, ret);
}

void SendMessage(ObjectId id, const char* method, const anito::Variant* args, uint32_t argc) {
    HierarchyObject* o = Obj(id);
    if (!o || !method) return;
    // Snapshot first: a handler may add or destroy components while we iterate.
    std::vector<ScriptComponent*> targets;
    for (const auto& c : o->GetComponents()) {
        if (auto* sc = dynamic_cast<ScriptComponent*>(c.get())) targets.push_back(sc);
    }
    anito::Variant ignored;
    for (ScriptComponent* sc : targets) sc->CallMethod(method, args, argc, &ignored);
}

} // namespace scripts

// ---- Input -----------------------------------------------------------------------------------------
namespace input {

bool GetAction(const char* action) {
    static std::map<std::string, std::unique_ptr<gbe::InputMappedBool>> actions;
    if (!action) return false;
    auto it = actions.find(action);
    if (it == actions.end()) it = actions.emplace(action, std::make_unique<gbe::InputMappedBool>(action)).first;
    return it->second->Get();
}

void GetMouseDelta(anito::Vec3* out) { const auto d = gbe::InputSystem::GetMouseDelta(); *out = {d.x, d.y, 0.0f}; }
void GetMousePosition(anito::Vec3* out) { const auto p = gbe::InputSystem::GetMousePosition(); *out = {p.x, p.y, 0.0f}; }

} // namespace input

// ---- Time ------------------------------------------------------------------------------------------
namespace clockapi {

float GetTime() { return std::chrono::duration<float>(std::chrono::steady_clock::now() - g_startTime).count(); }
float GetDeltaTime() { return g_deltaTime; }
float GetFixedDeltaTime() { return g_fixedDeltaTime; }

} // namespace clockapi

// ---- Gui -------------------------------------------------------------------------------------------
namespace gui {

void Viewport(ImVec2& pos, ImVec2& size) {
    pos = ImVec2(0.0f, 0.0f);
    size = ImGui::GetIO().DisplaySize;
    if (!AppState::isReleaseBuild) {
        const auto& manager = Diligent::GUIManager::GetInstance();
        pos = manager.GetGameViewportPos();
        size = manager.GetGameViewportSize();
    }
}

void WorldToScreen(anito::Vec3 world, anito::Vec3* out) {
    *out = {0.0f, 0.0f, -1.0f};
    glm::mat4 view(1.0f), proj(1.0f);
    HierarchyManager& hierarchy = HierarchyManager::GetInstance();
    if (!hierarchy.GetMainCamera() || !hierarchy.GetMainCameraMatrices(view, proj)) return;

    ImVec2 pos, size;
    Viewport(pos, size);
    if (size.x <= 1.0f || size.y <= 1.0f) return;

    const glm::vec4 clip = proj * view * glm::vec4(ToGlm(world), 1.0f);
    if (clip.w <= 0.0001f) return;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    // z carries visibility: negative when behind the camera or outside the depth range.
    const bool visible = ndc.z >= 0.0f && ndc.z <= 1.0f;
    *out = {pos.x + (ndc.x * 0.5f + 0.5f) * size.x, pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * size.y, visible ? ndc.z : -1.0f};
}

ImU32 Color(anito::Vec3 rgb, float alpha) {
    auto channel = [](float v) { return static_cast<int>(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
    return IM_COL32(channel(rgb.x), channel(rgb.y), channel(rgb.z), channel(alpha));
}

void FillRect(anito::Vec3 min, anito::Vec3 max, anito::Vec3 rgb, float alpha) {
    ImVec2 pos, size;
    Viewport(pos, size);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    list->AddRectFilled(ImVec2(min.x, min.y), ImVec2(max.x, max.y), Color(rgb, alpha));
    list->PopClipRect();
}

void DrawRect(anito::Vec3 min, anito::Vec3 max, anito::Vec3 rgb, float alpha, float thickness) {
    ImVec2 pos, size;
    Viewport(pos, size);
    ImDrawList* list = ImGui::GetForegroundDrawList();
    list->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    list->AddRect(ImVec2(min.x, min.y), ImVec2(max.x, max.y), Color(rgb, alpha), 0.0f, 0, thickness);
    list->PopClipRect();
}

} // namespace gui

// ---- Fields ----------------------------------------------------------------------------------------
namespace field {

ObjectId ResolveTransform(void* storage) {
    const auto& ref = *static_cast<gbe::ObjectRef<Transform>*>(storage);
    Transform* t = ref.Get();
    return t ? t->GetOwner().GetID() : 0;
}

const char* ResolvePrefab(void* storage) {
    thread_local std::string path;
    path = static_cast<PrefabRef*>(storage)->Resolve().string();
    return path.c_str();
}

} // namespace field

void HostLog(const char* message) { std::cerr << "[Script] " << message << std::endl; }

const anito::HostAPI kHostAPI = {
    HostLog,
    {transform::GetPosition, transform::SetPosition, transform::GetLocalPosition, transform::SetLocalPosition,
     transform::GetRotation, transform::SetRotation, transform::GetLocalRotation, transform::SetLocalRotation,
     transform::GetLocalScale, transform::SetLocalScale, transform::GetLossyScale, transform::GetEulerAngles,
     transform::SetEulerAngles},
    {physics::HasRigidbody, physics::IsSimulated, physics::GetVelocity, physics::SetVelocity, physics::GetAngularVelocity,
     physics::SetAngularVelocity, physics::AddForce, physics::AddTorque, physics::AddImpulse, physics::GetMass,
     physics::SetMass, physics::GetBodyRotation, physics::Teleport, physics::SetLockRotation, physics::HasCollider,
     physics::GetGravity, physics::SetGravity, physics::Raycast},
    {camera::HasCamera, camera::GetFOV, camera::SetFOV, camera::GetAspect, camera::SetAspect, camera::GetNearPlane,
     camera::SetNearPlane, camera::GetFarPlane, camera::SetFarPlane},
    {audio::HasAudioSource, audio::HasClip, audio::Play, audio::Stop, audio::PlayDetached, audio::GetVolume,
     audio::SetVolume, audio::GetLoop, audio::SetLoop, audio::GetPitch, audio::SetPitch, audio::GetSpatialized,
     audio::SetSpatialized, audio::SetDistance},
    {object::IsValid, object::GetName, object::SetName, object::GetParent, object::GetRoot, object::SetParent,
     object::GetChildCount, object::GetChild, object::FindChild, object::GetComponentCount, object::GetComponentName,
     object::HasComponent, object::AddRigidbody, object::AddCollider, object::Destroy},
    {scene::CreateEmpty, scene::CreatePrimitive, scene::Instantiate, scene::Find, scene::FindWithScript, scene::GetMainCamera},
    {scripts::Has, scripts::Add, scripts::Call, scripts::SendMessage},
    {input::GetAction, input::GetMouseDelta, input::GetMousePosition},
    {clockapi::GetTime, clockapi::GetDeltaTime, clockapi::GetFixedDeltaTime},
    {gui::WorldToScreen, gui::FillRect, gui::DrawRect},
    {field::ResolveTransform, field::ResolvePrefab},
};

} // namespace

namespace ScriptHost {

const anito::HostAPI* GetAPI() { return &kHostAPI; }
void SetFrameDelta(float deltaTime) { g_deltaTime = deltaTime; }
void SetFixedDelta(float deltaTime) { g_fixedDeltaTime = deltaTime; }

} // namespace ScriptHost
