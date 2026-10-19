#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "HierarchyObject.hpp"
#include "AudioManager.hpp"

class AudioListenerComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    // Initialize component
    AudioListenerComponent (gbe::IInstanceManager<HierarchyObject>::Ref owner = {}) 
        : ComponentBase("AudioListenerComponent", owner) {}

    ~AudioListenerComponent() override = default;

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    AudioListenerComponent(const AudioListenerComponent&) = delete;
    AudioListenerComponent& operator=(const AudioListenerComponent&) = delete;

    // Allow moving for container compatibility
    AudioListenerComponent(AudioListenerComponent&&) = default;
    AudioListenerComponent& operator=(AudioListenerComponent&&) = default;

    void OnUpdate(float deltaTime) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    bool m_enabled = true;

    GBE_SERIALIZE_FIELD_W_NAME(m_enabled, "Enabled");
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AudioListenerComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AudioListenerComponent, ComponentBase);