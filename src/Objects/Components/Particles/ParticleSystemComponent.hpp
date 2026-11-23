#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Types/EditorUpdateTrigger.hpp"
#include "HierarchyObject.hpp"
#include "ParticleRenderData.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <random>
#include <string>
#include <vector>

class ParticleSystemUI;

// CPU-simulated billboard particle emitter (Unity-style). Simulates in both edit and play mode
// so it can be previewed in the editor viewport.
class ParticleSystemComponent
    : public ComponentBase
    , public gbe::ITrigger<UpdateTrigger>
    , public gbe::ITrigger<EditorUpdateTrigger> {
    friend class ParticleSystemUI;
public:
    enum BlendMode : int { Alpha = 0, Additive = 1 };
    enum StopBehavior : int { DestroyAll = 0, Nothing = 1 };
    enum DestroyBehavior : int { DestroyWithObject = 0, LetDecay = 1 };

    ParticleSystemComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("ParticleSystemComponent", owner) {}
    ~ParticleSystemComponent() override;

    ParticleSystemComponent(const ParticleSystemComponent&) = delete;
    ParticleSystemComponent& operator=(const ParticleSystemComponent&) = delete;
    ParticleSystemComponent(ParticleSystemComponent&&) = default;
    ParticleSystemComponent& operator=(ParticleSystemComponent&&) = default;

    void OnUpdate(float deltaTime) override { Simulate(deltaTime); }
    void OnEditorUpdate(float deltaTime) override { Simulate(deltaTime); }

    // Restarts emission from t = 0 (fires the burst again).
    void Play();
    // Play() plus an immediate zero-time simulation step, so the burst is emitted right away
    // even if the owner is deleted before the next update.
    void PlayImmediate() { Play(); Simulate(0.0f); }
    // Stops emission; live particles are removed or left to expire depending on Stop Behavior.
    void Stop();
    void Clear() { m_particles.clear(); }
    bool IsPlaying() const { return m_playing; }
    size_t GetAliveCount() const { return m_particles.size(); }

    // Appends this system's live particles as a render batch (no-op when there are none).
    void BuildRenderBatch(std::vector<Diligent::ParticleBatch>& outBatches);

    // Orphaned particles (from components destroyed with LetDecay) live in a static pool.
    // Call once per frame; they are simulated and removed when fully expired.
    static void UpdateDecayingParticles(float deltaTime);
    static void BuildDecayingBatches(std::vector<Diligent::ParticleBatch>& outBatches);

    // Exposed for the inspector UI
    std::string& GetTexturePathRef() { return m_texturePath; }

private:
    struct Particle {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        float age = 0.0f;
        float lifetime = 1.0f;
        float size = 1.0f;
        float rotation = 0.0f;      // radians
        float angularVelocity = 0.0f;
    };

    void Simulate(float deltaTime);
    void CopySettingsFrom(const ParticleSystemComponent& other);
    void Emit(int count, const glm::vec3& worldPos, const glm::quat& worldRot);
    float Random(float minValue, float maxValue);

    bool m_playing = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_playing, "Playing");

    // DestroyAll: Stop() removes live particles. Nothing: emission stops, live particles finish their lifetime.
    int m_stopBehavior = DestroyAll;
    GBE_SERIALIZE_FIELD_W_NAME(m_stopBehavior, "Stop Behavior");

    // DestroyWithObject: particles vanish with the component. LetDecay: live particles are handed
    // to a static pool and finish their lifetime after the object is gone.
    int m_destroyBehavior = DestroyWithObject;
    GBE_SERIALIZE_FIELD_W_NAME(m_destroyBehavior, "Destroy Behavior");

    bool m_loop = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_loop, "Loop");

    float m_duration = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_duration, "Duration (s)");

    int m_maxParticles = 500;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxParticles, "Max Particles");

    float m_emissionRate = 20.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_emissionRate, "Emission Rate (/s)");

    int m_burstCount = 0;
    GBE_SERIALIZE_FIELD_W_NAME(m_burstCount, "Burst Count");

    float m_lifetimeMin = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_lifetimeMin, "Lifetime Min (s)");

    float m_lifetimeMax = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_lifetimeMax, "Lifetime Max (s)");

    // Initial velocity = StartVelocity + random(-1..1) * VelocityRandomness (per axis)
    glm::vec3 m_startVelocity{ 0.0f, 2.0f, 0.0f };
    GBE_SERIALIZE_FIELD_W_NAME(m_startVelocity, "Start Velocity");

    glm::vec3 m_velocityRandomness{ 0.5f, 0.5f, 0.5f };
    GBE_SERIALIZE_FIELD_W_NAME(m_velocityRandomness, "Velocity Randomness");

    // When true the velocity is rotated by the emitter's rotation (local axes)
    bool m_velocityLocal = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_velocityLocal, "Velocity In Local Space");

    glm::vec3 m_gravity{ 0.0f, 0.0f, 0.0f };
    GBE_SERIALIZE_FIELD_W_NAME(m_gravity, "Acceleration");

    glm::vec3 m_spawnBoxSize{ 0.0f, 0.0f, 0.0f };
    GBE_SERIALIZE_FIELD_W_NAME(m_spawnBoxSize, "Spawn Box Size");

    // true: particles keep their world position when the emitter moves; false: they follow it
    bool m_worldSpace = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_worldSpace, "World Space Simulation");

    float m_startSizeMin = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_startSizeMin, "Start Size Min");

    float m_startSizeMax = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_startSizeMax, "Start Size Max");

    float m_endSizeScale = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_endSizeScale, "End Size Scale");

    float m_startRotationMin = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_startRotationMin, "Start Rotation Min (deg)");

    float m_startRotationMax = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_startRotationMax, "Start Rotation Max (deg)");

    float m_rotationSpeed = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_rotationSpeed, "Rotation Speed (deg/s)");

    glm::vec4 m_startColor{ 1.0f, 1.0f, 1.0f, 1.0f };
    GBE_SERIALIZE_FIELD_W_NAME(m_startColor, "Start Color");

    glm::vec4 m_endColor{ 1.0f, 1.0f, 1.0f, 0.0f };
    GBE_SERIALIZE_FIELD_W_NAME(m_endColor, "End Color");

    std::string m_texturePath = "";
    GBE_SERIALIZE_FIELD_W_NAME(m_texturePath, "Texture Path");

    int m_blendMode = Alpha;
    GBE_SERIALIZE_FIELD_W_NAME(m_blendMode, "Blend Mode");

    int m_flipbookColumns = 1;
    GBE_SERIALIZE_FIELD_W_NAME(m_flipbookColumns, "Flipbook Columns");

    int m_flipbookRows = 1;
    GBE_SERIALIZE_FIELD_W_NAME(m_flipbookRows, "Flipbook Rows");

    // 0 = use Columns * Rows
    int m_flipbookFrameCount = 0;
    GBE_SERIALIZE_FIELD_W_NAME(m_flipbookFrameCount, "Flipbook Frame Count");

    // 0 = play the whole sheet once over each particle's lifetime
    float m_flipbookFps = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_flipbookFps, "Flipbook FPS");

    // Only used with Flipbook FPS > 0: wrap around instead of holding the last frame
    bool m_flipbookLoop = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_flipbookLoop, "Flipbook Loop");

    // Runtime (non-serialized) state
    std::vector<Particle> m_particles;
    glm::mat4 m_lastEmitterMatrix{ 1.0f };  // cached so particles can be converted to world space on destruction
    float m_time = 0.0f;
    float m_emitAccumulator = 0.0f;
    bool m_burstPending = true;
    std::mt19937 m_rng{ std::random_device{}() };

    std::string m_loadedTexturePath;
    Diligent::ITextureView* m_texture = nullptr;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ParticleSystemComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ParticleSystemComponent, ComponentBase);
