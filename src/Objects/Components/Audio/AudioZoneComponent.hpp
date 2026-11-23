#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "DebugLine.hpp"
#include "HierarchyObject.hpp"
#include "AudioClip.hpp"
#include "AudioManager.hpp"

class AudioZoneComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    // Initialize component
    AudioZoneComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("AudioZoneComponent", owner) {}

    ~AudioZoneComponent() override {
        if (m_isInsideZone) {
            AudioManager::GetInstance().PopMusic(m_activeKey, GetFadeLength());
            m_isInsideZone = false;
        }
    }

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    AudioZoneComponent(const AudioZoneComponent&) = delete;
    AudioZoneComponent& operator=(const AudioZoneComponent&) = delete;

    // Allow moving for container compatibility
    AudioZoneComponent(AudioZoneComponent&&) = default;
    AudioZoneComponent& operator=(AudioZoneComponent&&) = default;

    void OnUpdate(float deltaTime) override;
    void AppendDebugLines(std::vector<DebugLineVertex>& out) const;

private:
    // Audio file to play inside zone
    gbe::AssetRef<AudioClip> m_audioClip = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_audioClip, "Audio Clip");
    
    uint64_t m_activeKey = 0;

    // Distance from zone center to activate the audio
    float m_enterRadius = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_enterRadius, "Enter Radius", [this](float) { m_enterRadius = std::clamp(m_enterRadius, 0.0f, 1000.0f); });

    // Distance beyond the enter radius that deactivates the audio
    float m_exitMargin = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_exitMargin, "Exit Margin", [this](float) { m_exitMargin = std::clamp(m_exitMargin, 0.0f, 500.0f); });

    // Determines how long the audio increases/decreases its volume for transition
    int m_fadeLength = 2000;
    GBE_SERIALIZE_FIELD_W_NAME_CB(m_fadeLength, "Fade Length (ms)", [this](int) { m_fadeLength = std::clamp(m_fadeLength, 0, 10000); });

    bool m_isInsideZone = false;

    uint64_t MakeOwnerKey() const { return static_cast<uint64_t>(std::hash<gbe::GUID>{}(GetGUID())); }
    uint64_t GetFadeLength() const { return static_cast<uint64_t>(std::clamp(m_fadeLength, 0, 10000)); }

    // Wireframe sphere for zone visibility
    static void AddCircle(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, int axisA, int axisB, const glm::vec4& color);
    static void AddSphere(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, const glm::vec4& color);

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AudioZoneComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AudioZoneComponent, ComponentBase);