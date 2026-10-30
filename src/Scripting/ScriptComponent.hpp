#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Types/FixedUpdateTrigger.hpp"
#include "Types/OnGUI_Release.hpp"
#include "../Physics/IPhysicsEngine.hpp"
#include "AnitoScriptSDK.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "../Tools/Serialization/SerializedData.hpp"

struct ScriptDescriptor;

class ScriptFieldBase {
public:
    virtual ~ScriptFieldBase() = default;
    virtual void* Data() = 0;
};

// Host-side component wrapper: serialized/editor fields remain native, while behavior is supplied
// by a separately compiled script instance.
class ScriptComponent
    : public ComponentBase
    , public gbe::ITrigger<UpdateTrigger>
    , public gbe::ITrigger<FixedUpdateTrigger>
    , public gbe::ITrigger<OnGUI_Release>
    , public gbe::ITrigger<CollisionEnterTrigger> {
public:
    ScriptComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~ScriptComponent() override;

    ScriptComponent(const ScriptComponent&) = delete;
    ScriptComponent& operator=(const ScriptComponent&) = delete;

    // Creates the inspector/serialized fields declared by the named script.
    void BindScript(const std::string& scriptName);

    // Rebuilds fields (keeping values) if the script file was re-parsed.
    void Refresh();

    const std::string& GetScriptName() const { return m_scriptName; }
    bool IsScriptMissing() const;

    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate(float deltaTime) override;
    void OnGUI_ReleaseEvent(float deltaTime) override;
    void OnTrigger(const CollisionEnterTrigger& trigger) override;

    // Calls a script method by name (creating the native instance first if needed).
    // Returns false when the script has no such callable method.
    bool CallMethod(const char* method, const anito::Variant* args, uint32_t argc, anito::Variant* ret);
    // Destroys the native instance (fields stay); it is recreated on the next update.
    void ReleaseInstance();

    gbe::SerializedData Serialize() override;
    void Deserialize(gbe::SerializedData& data) override;
    std::vector<std::string> GetHiddenProperties() const override { return {"m_scriptName"}; }

private:
    void CreateFields(const ScriptDescriptor& descriptor);
    void EnsureInstance();

    std::string m_scriptName;
    GBE_SERIALIZE_FIELD_W_NAME(m_scriptName, "Script");

    std::vector<std::unique_ptr<ScriptFieldBase>> m_fields;
    std::optional<gbe::SerializedData> m_deferredSerializedData;
    uint64_t m_boundRevision = 0;

    anito::ScriptBehaviour* m_instance = nullptr;
    const anito::ScriptEntry* m_entry = nullptr;
    uint64_t m_instanceGeneration = 0;
    std::vector<void*> m_fieldPtrs;

    // Collision callbacks can arrive on physics worker threads; they are queued and dispatched in OnUpdate.
    std::unique_ptr<std::mutex> m_hitMutex = std::make_unique<std::mutex>();
    std::vector<anito::ObjectId> m_pendingHits;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ScriptComponent, ComponentBase);
};
