#pragma once

#include "Components/ComponentBase.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ScriptDescriptor;

class ScriptFieldBase {
public:
    virtual ~ScriptFieldBase() = default;
};

// Generic component whose fields come from a script descriptor instead of C++ members.
class ScriptComponent : public ComponentBase {
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

    void Deserialize(gbe::SerializedData& data) override;
    std::vector<std::string> GetHiddenProperties() const override { return {"m_scriptName"}; }

private:
    void CreateFields(const ScriptDescriptor& descriptor);

    std::string m_scriptName;
    GBE_SERIALIZE_FIELD_W_NAME(m_scriptName, "Script");

    std::vector<std::unique_ptr<ScriptFieldBase>> m_fields;
    uint64_t m_boundRevision = 0;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ScriptComponent, ComponentBase);
};
