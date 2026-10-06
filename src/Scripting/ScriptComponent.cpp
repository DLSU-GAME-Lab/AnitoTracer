#include "ScriptComponent.hpp"

#include "ScriptDescriptor.hpp"
#include "ScriptModule.hpp"
#include "ScriptRegistry.hpp"

namespace {

template <typename T>
class ScriptFieldHolder final : public ScriptFieldBase {
public:
    ScriptFieldHolder(gbe::ISerializable* owner, const std::string& id, const std::string& display, T initial)
        : m_value(std::move(initial)), m_serializer(owner, id, display, m_value) {}

    void* Data() override { return &m_value; }

private:
    T m_value;
    gbe::AutoSerializer<T> m_serializer;
};

// Prefix keeps script field ids from colliding with built-in ids such as m_guid.
std::string FieldId(const ScriptField& field) { return "script." + field.name; }

} // namespace

ScriptComponent::ScriptComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("ScriptComponent", owner) {}

ScriptComponent::~ScriptComponent() { ReleaseInstance(); }

void ScriptComponent::ReleaseInstance() {
    if (!m_instance) return;
    ScriptModule::GetInstance().Unregister(this);
    m_entry->destroy(m_instance);
    m_instance = nullptr;
    m_entry = nullptr;
}

void ScriptComponent::EnsureInstance() {
    ScriptModule& module = ScriptModule::GetInstance();
    if (m_instance && m_instanceGeneration == module.Generation()) return;
    ReleaseInstance();
    m_instanceGeneration = module.Generation();

    const ScriptDescriptor* descriptor = ScriptRegistry::GetInstance().Find(m_scriptName);
    if (!descriptor || descriptor->revision != m_boundRevision) return;
    const anito::ScriptEntry* entry = module.FindEntry(*descriptor);
    if (!entry || m_fields.size() != entry->fieldCount) return;

    m_fieldPtrs.clear();
    for (const auto& field : m_fields) m_fieldPtrs.push_back(field->Data());

    const anito::ScriptContext context{module.GetHostAPI(), static_cast<ComponentBase*>(this), m_fieldPtrs.data()};
    m_instance = entry->create(context);
    m_entry = entry;
    // Only components holding native code need to be released before a reload.
    module.Register(this);
    m_instance->OnStart();
}

void ScriptComponent::OnUpdate(float deltaTime) {
    Refresh();
    EnsureInstance();
    if (m_instance) m_instance->OnUpdate(deltaTime);
}

void ScriptComponent::BindScript(const std::string& scriptName) {
    ReleaseInstance();
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

    ReleaseInstance();
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
