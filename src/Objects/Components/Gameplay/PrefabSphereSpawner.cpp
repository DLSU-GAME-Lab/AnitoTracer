#include "PrefabSphereSpawner.hpp"

#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"
#include "HierarchyFeatures/PrefabFeature.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

PrefabSphereSpawner::PrefabSphereSpawner(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("PrefabSphereSpawner", owner) {}

void PrefabSphereSpawner::OnStart() {
    m_waveTimer = 0.0f;
    m_wavesSpawned = 0;
    m_spawned.clear();
    if (m_spawnOnStart) {
        Spawn();
    }
}

void PrefabSphereSpawner::OnUpdate(float deltaTime) {
    if (!m_spawnByInterval) {
        return;
    }

    m_waveTimer += deltaTime;
    if (m_waveTimer < std::max(m_waveInterval, 0.01f)) {
        return;
    }
    m_waveTimer = 0.0f;
    Spawn();
}

void PrefabSphereSpawner::Spawn() {
    if (m_prefab.IsEmpty() || m_count <= 0) {
        return;
    }
    if (m_maxWaves > 0 && m_wavesSpawned >= m_maxWaves) {
        return;
    }

    m_spawned.erase(
        std::remove_if(m_spawned.begin(), m_spawned.end(),
            [](const HierarchyObject::Ref& ref) { return ref.GetPtr() == nullptr; }),
        m_spawned.end());

    int toSpawn = m_count;
    if (m_maxAlive > 0) {
        toSpawn = std::min(toSpawn, m_maxAlive - static_cast<int>(m_spawned.size()));
    }
    if (toSpawn <= 0) {
        return;
    }
    ++m_wavesSpawned;

    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* ownerTransform = owner ? owner->GetTransform() : nullptr;
    if (!ownerTransform) {
        return;
    }

    const std::filesystem::path prefabPath = m_prefab.Resolve();
    const glm::vec3 center = ownerTransform->GetPosition();

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    const float minScale = std::min(m_minScale, m_maxScale);
    const float maxScale = std::max(m_minScale, m_maxScale);

    for (int i = 0; i < toSpawn; ++i) {
        // Uniform direction on the sphere, then cube-root radius for uniform volume density.
        const float z = unit(rng) * 2.0f - 1.0f;
        const float angle = unit(rng) * glm::two_pi<float>();
        const float ring = std::sqrt(std::max(0.0f, 1.0f - z * z));
        const glm::vec3 direction(ring * std::cos(angle), ring * std::sin(angle), z);
        const float distance = m_surfaceOnly ? m_radius : m_radius * std::cbrt(unit(rng));

        HierarchyObject::Ref spawnedRef = PrefabFeature::InstantiatePrefab(prefabPath);
        HierarchyObject* spawned = spawnedRef.GetPtr();
        if (!spawned) {
            return;
        }
        m_spawned.push_back(spawnedRef);

        Transform* transform = spawned->GetTransform();
        if (!transform) {
            continue;
        }

        transform->SetPosition(center + direction * distance);

        if (m_randomRotation) {
            const glm::vec3 axis = glm::normalize(glm::vec3(unit(rng) - 0.5f, unit(rng) - 0.5f, unit(rng) - 0.5f) + glm::vec3(1e-4f));
            transform->SetRotation(glm::angleAxis(unit(rng) * glm::two_pi<float>(), axis));
        }

        if (maxScale != 1.0f || minScale != 1.0f) {
            const float scale = minScale + (maxScale - minScale) * unit(rng);
            transform->SetScale(transform->GetLocalScale() * scale);
        }
    }
}
