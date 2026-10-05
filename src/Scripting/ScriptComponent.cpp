#include "ScriptComponent.hpp"

#include "ScriptDescriptor.hpp"
#include "ScriptRegistry.hpp"

namespace {

template <typename T>
class ScriptFieldHolder final : public ScriptFieldBase {
public:
    ScriptFieldHolder(gbe::ISerializable* owner, const std::string& id, const std::string& display, T initial)
        : m_value(std::move(initial)), m_serializer(owner, id, display, m_value) {}

private:
    T m_value;
    gbe::AutoSerializer<T> m_serializer;
};

// Prefix keeps script field ids from colliding with built-in ids such as m_guid.
std::string FieldId(const ScriptField& field) { return "script." + field.name; }

} // namespace

ScriptComponent::ScriptComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("ScriptComponent", owner) {}

ScriptComponent::~ScriptComponent() = default;

void ScriptComponent::BindScript(const std::string& scriptName) {
    m_scriptName = scriptName;
    m_fields.clear();
    m_boundRevision = 0;

    if (const ScriptDescriptor* descriptor = ScriptRegistry::GetInstance().Find(scriptName)) {
        CreateFields(*descriptor);
    }
}

void ScriptComponent::CreateFields(const ScriptDescriptor& descriptor) {
    for (const ScriptField& field : descriptor.fields) {
        const std::string id = FieldId(field);
        switch (field.type) {
        case ScriptFieldType::Float:
            m_fields.push_back(std::make_unique<ScriptFieldHolder<float>>(this, id, field.displayName, std::get<float>(field.defaultValue)));
            break;
        case ScriptFieldType::Int:
            m_fields.push_back(std::make_unique<ScriptFieldHolder<int>>(this, id, field.displayName, std::get<int>(field.defaultValue)));
            break;
        case ScriptFieldType::Bool:
            m_fields.push_back(std::make_unique<ScriptFieldHolder<bool>>(this, id, field.displayName, std::get<bool>(field.defaultValue)));
            break;
        case ScriptFieldType::String:
            m_fields.push_back(std::make_unique<ScriptFieldHolder<std::string>>(this, id, field.displayName, std::get<std::string>(field.defaultValue)));
            break;
        case ScriptFieldType::Vec3:
            m_fields.push_back(std::make_unique<ScriptFieldHolder<glm::vec3>>(this, id, field.displayName, std::get<glm::vec3>(field.defaultValue)));
            break;
        }
    }
    m_boundRevision = descriptor.revision;
}

void ScriptComponent::Refresh() {
    const ScriptDescriptor* descriptor = ScriptRegistry::GetInstance().Find(m_scriptName);
    if (!descriptor || descriptor->revision == m_boundRevision) return;

    gbe::SerializedData saved = Serialize();
    m_fields.clear();
    CreateFields(*descriptor);
    ComponentBase::Deserialize(saved);
}

bool ScriptComponent::IsScriptMissing() const {
    return ScriptRegistry::GetInstance().Find(m_scriptName) == nullptr;
}

void ScriptComponent::Deserialize(gbe::SerializedData& data) {
    // Fields must exist before the base class walks the property list.
    const auto it = data.serialized_variables.find("m_scriptName");
    if (it != data.serialized_variables.end() && it->second != m_scriptName) {
        BindScript(it->second);
    }
    ComponentBase::Deserialize(data);
}
