#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AnitoScriptSDK.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ScriptDescriptor;

class ScriptFieldBase {
public:
    virtual ~ScriptFieldBase() = default;
    virtual void* Data() = 0;
};

// Generic component whose fields come from a script descriptor instead of C++ members.
class ScriptComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
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

    // Destroys the native instance (fields stay); it is recreated on the next update.
    void ReleaseInstance();

    void Deserialize(gbe::SerializedData& data) override;
    std::vector<std::string> GetHiddenProperties() const override { return {"m_scriptName"}; }

private:
    void CreateFields(const ScriptDescriptor& descriptor);
    void EnsureInstance();

    std::string m_scriptName;
    GBE_SERIALIZE_FIELD_W_NAME(m_scriptName, "Script");

    std::vector<std::unique_ptr<ScriptFieldBase>> m_fields;
    uint64_t m_boundRevision = 0;

    anito::ScriptBehaviour* m_instance = nullptr;
    const anito::ScriptEntry* m_entry = nullptr;
    uint64_t m_instanceGeneration = 0;
    std::vector<void*> m_fieldPtrs;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ScriptComponent, ComponentBase);
};
