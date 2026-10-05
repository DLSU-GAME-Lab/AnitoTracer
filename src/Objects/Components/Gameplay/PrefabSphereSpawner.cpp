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
    if (m_spawnOnStart) {
        Spawn();
    }
}

void PrefabSphereSpawner::Spawn() {
    if (m_prefab.IsEmpty() || m_count <= 0) {
        return;
    }

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

    for (int i = 0; i < m_count; ++i) {
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
