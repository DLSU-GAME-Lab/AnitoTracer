#include "ParticleSpawnerComponent.hpp"
#include "ObjectFactory.hpp"
#include "HierarchyObject.hpp"
#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"

#include "PropertyDrawers/event_drawer.hpp"
#include <cstdlib>
#include <iostream>

ParticleSpawnerComponent::ParticleSpawnerComponent(Transform* /*transform*/, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("ParticleSpawnerComponent", owner) {
    GBE_Init();
}

ParticleSpawnerComponent::~ParticleSpawnerComponent() = default;

void ParticleSpawnerComponent::GBE_Init() {
    // Register component as a listener for the specified event key string
    if (!m_triggerEventKey.empty()) {
        SubscribeTo(m_triggerEventKey, [this](const std::unique_ptr<gbe::EventArgs>&) {
            Spawn();
        });
    }
}

void ParticleSpawnerComponent::Spawn() {
    Transform* spawnerTransform = nullptr;
    if (HierarchyObject* owner = GetOwner().GetPtr()) {
        spawnerTransform = owner->GetTransform();
    }

    glm::vec3 origin = spawnerTransform ? spawnerTransform->GetPosition() : glm::vec3(0.0f);

    for (int i = 0; i < m_spawnCount; ++i) {
        // Create sphere primitive instance via ObjectFactory
        HierarchyObject::Ref sphereRef = ObjectFactory::GetInstance().CreateSpherePrimitive("Particle_Sphere");
        HierarchyObject* sphereObj = sphereRef.GetPtr();

        if (sphereObj) {
            Transform* sphereTransform = sphereObj->GetTransform();
            if (sphereTransform) {
                sphereTransform->SetPosition(origin);
                sphereTransform->SetScale(glm::vec3(0.2f)); // Scale primitive down to particle size
            }

            // Calculate randomized upward and horizontal arc trajectory
            float randX = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * m_spread;
            float randZ = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * m_spread;
            float randY = m_upwardForce + ((static_cast<float>(rand()) / RAND_MAX) * 2.0f);

            Particle p;
            p.sphereRef = sphereRef;
            p.velocity = glm::vec3(randX, randY, randZ);
            p.currentAge = 0.0f;
            p.maxLifetime = m_particleLifetime;

            m_activeParticles.push_back(p);
        }
    }
}

void ParticleSpawnerComponent::OnUpdate(float deltaTime) {
    for (auto it = m_activeParticles.begin(); it != m_activeParticles.end();) {
        Particle& p = *it;
        p.currentAge += deltaTime;

        HierarchyObject* particleObj = p.sphereRef.GetPtr();

        if (p.currentAge >= p.maxLifetime || !particleObj) {
            // Particle lifecycle finished
            if (particleObj) {
                HierarchyManager::GetInstance().QueueObjectDeletion(p.sphereRef);
            }
            it = m_activeParticles.erase(it);
            continue;
        }

        Transform* particleTransform = particleObj->GetTransform();
        if (particleTransform) {
            // Apply downward gravity acceleration
            p.velocity.y -= m_gravity * deltaTime;

            // Move particle position
            glm::vec3 currentPos = particleTransform->GetPosition();
            particleTransform->SetPosition(currentPos + (p.velocity * deltaTime));
        }

        ++it;
    }
}