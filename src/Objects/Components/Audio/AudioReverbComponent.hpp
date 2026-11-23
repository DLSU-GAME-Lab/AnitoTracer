#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "HierarchyObject.hpp"
#include "AudioManager.hpp"
#include "DebugLine.hpp"

class AudioReverbComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    AudioReverbComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("AudioReverbComponent", owner) {}

    ~AudioReverbComponent() override {
        if (m_isActive) {
            AudioManager::GetInstance().SetSfxReverbWet(0.0f, m_rampMs);
            m_isActive = false;
        }
    }

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    AudioReverbComponent(const AudioReverbComponent&) = delete;
    AudioReverbComponent& operator=(const AudioReverbComponent&) = delete;

    // Allow moving for container compatibility
    AudioReverbComponent(AudioReverbComponent&&) = default;
    AudioReverbComponent& operator=(AudioReverbComponent&&) = default;

    void OnUpdate(float deltaTime) override;
    void AppendDebugLines(std::vector<DebugLineVertex>& out) const;

private:
    float m_reverbRadius = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_reverbRadius, "Radius", [this](float) { m_reverbRadius = std::clamp(m_reverbRadius, 0.0f, 1000.0f); });

    float m_fadeDistance = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_fadeDistance, "Fade Distance", [this](float) { m_fadeDistance = std::clamp(m_fadeDistance, 0.0f, 500.0f); });

    float m_reverbWet = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_reverbWet, "Wet", [this](float) { m_reverbWet = std::clamp(m_reverbWet, 0.0f, 1.0f); });

    float m_reverbRoomSize = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_reverbRoomSize, "Room Size", [this](float) { m_reverbRoomSize = std::clamp(m_reverbRoomSize, 0.0f, 1.0f); });

    float m_reverbDamping = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_reverbDamping, "Damping", [this](float) { m_reverbDamping = std::clamp(m_reverbDamping, 0.0f, 1.0f); });

    bool m_isActive = false;
    static constexpr uint64_t m_rampMs = 120;

    // Wireframe sphere for zone visibility
    static void AddCircle(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, int axisA, int axisB, const glm::vec4& color);
    static void AddSphere(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, const glm::vec4& color);

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AudioReverbComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AudioReverbComponent, ComponentBase);