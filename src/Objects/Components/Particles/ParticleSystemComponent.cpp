#include "ParticleSystemComponent.hpp"

#include "Models/ModelManager.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace {
    constexpr float kMaxDeltaTime = 0.1f;
    constexpr float kDegToRad = 0.017453292519943295f;

    // Intentionally leaked so components destroyed during static teardown never touch a dead container.
    std::vector<std::unique_ptr<ParticleSystemComponent>>& DecayPool() {
        static auto* pool = new std::vector<std::unique_ptr<ParticleSystemComponent>>();
        return *pool;
    }
}

void ParticleSystemComponent::CopySettingsFrom(const ParticleSystemComponent& o) {
    m_maxParticles = o.m_maxParticles;
    m_gravity = o.m_gravity;
    m_startSizeMin = o.m_startSizeMin;
    m_startSizeMax = o.m_startSizeMax;
    m_endSizeScale = o.m_endSizeScale;
    m_startColor = o.m_startColor;
    m_endColor = o.m_endColor;
    m_texturePath = o.m_texturePath;
    m_loadedTexturePath = o.m_loadedTexturePath;
    m_texture = o.m_texture;
    m_blendMode = o.m_blendMode;
    m_flipbookColumns = o.m_flipbookColumns;
    m_flipbookRows = o.m_flipbookRows;
    m_flipbookFrameCount = o.m_flipbookFrameCount;
    m_flipbookFps = o.m_flipbookFps;
    m_flipbookLoop = o.m_flipbookLoop;
}

ParticleSystemComponent::~ParticleSystemComponent() {
    if (m_destroyBehavior != LetDecay || m_particles.empty()) return;

    auto decaying = std::make_unique<ParticleSystemComponent>();
    decaying->CopySettingsFrom(*this);
    decaying->m_playing = false;
    decaying->m_worldSpace = true;
    decaying->m_destroyBehavior = DestroyWithObject;

    // Bake emitter-space particles into world space since the emitter is going away
    decaying->m_particles = std::move(m_particles);
    if (!m_worldSpace) {
        for (Particle& p : decaying->m_particles) {
            p.position = glm::vec3(m_lastEmitterMatrix * glm::vec4(p.position, 1.0f));
            p.velocity = glm::vec3(m_lastEmitterMatrix * glm::vec4(p.velocity, 0.0f));
        }
    }

    DecayPool().push_back(std::move(decaying));
}

void ParticleSystemComponent::UpdateDecayingParticles(float deltaTime) {
    auto& pool = DecayPool();
    for (auto& system : pool) {
        system->Simulate(deltaTime);
    }
    pool.erase(std::remove_if(pool.begin(), pool.end(),
        [](const std::unique_ptr<ParticleSystemComponent>& s) { return s->m_particles.empty(); }), pool.end());
}

void ParticleSystemComponent::BuildDecayingBatches(std::vector<Diligent::ParticleBatch>& outBatches) {
    for (auto& system : DecayPool()) {
        system->BuildRenderBatch(outBatches);
    }
}

float ParticleSystemComponent::Random(float minValue, float maxValue) {
    if (maxValue <= minValue) return minValue;
    return std::uniform_real_distribution<float>(minValue, maxValue)(m_rng);
}

void ParticleSystemComponent::Play() {
    m_playing = true;
    m_time = 0.0f;
    m_emitAccumulator = 0.0f;
    m_burstPending = true;
}

void ParticleSystemComponent::Stop() {
    m_playing = false;
    if (m_stopBehavior == DestroyAll) {
        m_particles.clear();
    }
}

void ParticleSystemComponent::Emit(int count, const glm::vec3& worldPos, const glm::quat& worldRot) {
    const int capacity = std::max(m_maxParticles, 0);

    for (int i = 0; i < count && static_cast<int>(m_particles.size()) < capacity; ++i) {
        Particle p;

        const glm::vec3 spawnOffset(
            Random(-0.5f, 0.5f) * m_spawnBoxSize.x,
            Random(-0.5f, 0.5f) * m_spawnBoxSize.y,
            Random(-0.5f, 0.5f) * m_spawnBoxSize.z);

        glm::vec3 velocity = m_startVelocity + glm::vec3(
            Random(-1.0f, 1.0f) * m_velocityRandomness.x,
            Random(-1.0f, 1.0f) * m_velocityRandomness.y,
            Random(-1.0f, 1.0f) * m_velocityRandomness.z);

        if (m_worldSpace) {
            p.position = worldPos + worldRot * spawnOffset;
            p.velocity = m_velocityLocal ? worldRot * velocity : velocity;
        }
        else {
            // Stored in emitter space and transformed by the emitter's matrix when rendered
            p.position = spawnOffset;
            p.velocity = velocity;
        }

        p.lifetime = std::max(Random(m_lifetimeMin, m_lifetimeMax), 0.001f);
        p.size = Random(m_startSizeMin, m_startSizeMax);
        p.rotation = Random(m_startRotationMin, m_startRotationMax) * kDegToRad;
        p.angularVelocity = m_rotationSpeed * kDegToRad;

        m_particles.push_back(p);
    }
}

void ParticleSystemComponent::Simulate(float deltaTime) {
    const float dt = std::clamp(deltaTime, 0.0f, kMaxDeltaTime);

    if (!m_worldSpace) {
        if (HierarchyObject* owner = GetOwner().GetPtr()) {
            if (Transform* transform = owner->GetTransform()) {
                m_lastEmitterMatrix = transform->GetWorldMatrix();
            }
        }
    }

    for (size_t i = 0; i < m_particles.size();) {
        Particle& p = m_particles[i];
        p.age += dt;
        if (p.age >= p.lifetime) {
            p = m_particles.back();
            m_particles.pop_back();
            continue;
        }
        p.velocity += m_gravity * dt;
        p.position += p.velocity * dt;
        p.rotation += p.angularVelocity * dt;
        ++i;
    }

    if (!m_playing) return;

    glm::vec3 worldPos(0.0f);
    glm::quat worldRot(1.0f, 0.0f, 0.0f, 0.0f);
    if (HierarchyObject* owner = GetOwner().GetPtr()) {
        if (Transform* transform = owner->GetTransform()) {
            worldPos = transform->GetPosition();
            worldRot = transform->GetRotation();
        }
    }

    if (m_burstPending) {
        Emit(m_burstCount, worldPos, worldRot);
        m_burstPending = false;
    }

    const bool emitting = m_loop || m_time < m_duration;
    m_time += dt;

    if (emitting && m_emissionRate > 0.0f) {
        m_emitAccumulator += m_emissionRate * dt;
        const int count = static_cast<int>(m_emitAccumulator);
        m_emitAccumulator -= static_cast<float>(count);
        Emit(count, worldPos, worldRot);
    }

    if (m_loop && m_duration > 0.0f && m_time >= m_duration) {
        m_time = std::fmod(m_time, m_duration);
        m_burstPending = true;
    }
}

void ParticleSystemComponent::BuildRenderBatch(std::vector<Diligent::ParticleBatch>& outBatches) {
    if (m_particles.empty()) return;

    if (m_texturePath != m_loadedTexturePath) {
        m_loadedTexturePath = m_texturePath;
        m_texture = m_texturePath.empty() ? nullptr : ModelManager::GetInstance().LoadTexture(m_texturePath, true);
    }

    const int columns = std::max(m_flipbookColumns, 1);
    const int rows = std::max(m_flipbookRows, 1);
    const int gridFrames = columns * rows;
    const int totalFrames = m_flipbookFrameCount > 0 ? std::min(m_flipbookFrameCount, gridFrames) : gridFrames;

    glm::mat4 emitterMatrix(1.0f);
    if (!m_worldSpace) {
        if (HierarchyObject* owner = GetOwner().GetPtr()) {
            if (Transform* transform = owner->GetTransform()) {
                emitterMatrix = transform->GetWorldMatrix();
            }
        }
    }

    Diligent::ParticleBatch batch;
    batch.Texture = m_texture;
    batch.Blend = (m_blendMode == Additive) ? Diligent::ParticleBlendMode::Additive : Diligent::ParticleBlendMode::Alpha;
    batch.Columns = static_cast<uint32_t>(columns);
    batch.Rows = static_cast<uint32_t>(rows);
    batch.Instances.reserve(m_particles.size());

    for (const Particle& p : m_particles) {
        const float t = std::clamp(p.age / p.lifetime, 0.0f, 1.0f);

        float frame;
        if (m_flipbookFps > 0.0f) {
            frame = p.age * m_flipbookFps;
            frame = m_flipbookLoop ? std::fmod(frame, static_cast<float>(totalFrames))
                                   : std::min(frame, static_cast<float>(totalFrames - 1));
        }
        else {
            frame = t * static_cast<float>(totalFrames);
        }
        frame = std::min(std::floor(frame), static_cast<float>(totalFrames - 1));

        Diligent::ParticleInstance instance;
        instance.Position = m_worldSpace ? p.position : glm::vec3(emitterMatrix * glm::vec4(p.position, 1.0f));
        instance.Rotation = p.rotation;
        const float size = p.size * glm::mix(1.0f, m_endSizeScale, t);
        instance.Size = glm::vec2(size);
        instance.Frame = frame;
        instance.Color = glm::mix(m_startColor, m_endColor, t);
        batch.Instances.push_back(instance);
    }

    outBatches.push_back(std::move(batch));
}
