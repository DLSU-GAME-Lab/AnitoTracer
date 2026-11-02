#include "ParticleGather.hpp"

#include "ParticleSystemComponent.hpp"
#include "HierarchyManager.hpp"

#include <chrono>

namespace {
    void GatherRecursive(HierarchyObject* object, std::vector<Diligent::ParticleBatch>& outBatches) {
        if (!object) return;

        if (ParticleSystemComponent* particles = object->GetComponent<ParticleSystemComponent>()) {
            particles->BuildRenderBatch(outBatches);
        }

        for (const auto& child : object->GetChildren()) {
            GatherRecursive(child.get(), outBatches);
        }
    }
}

void GatherParticleBatches(std::vector<Diligent::ParticleBatch>& outBatches) {
    outBatches.clear();
    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        GatherRecursive(root.get(), outBatches);
    }

    // Particles left behind by destroyed emitters (Destroy Behavior = Let Decay)
    using Clock = std::chrono::steady_clock;
    static Clock::time_point lastTime = Clock::now();
    const Clock::time_point now = Clock::now();
    const float dt = std::chrono::duration<float>(now - lastTime).count();
    lastTime = now;
    ParticleSystemComponent::UpdateDecayingParticles(dt);
    ParticleSystemComponent::BuildDecayingBatches(outBatches);
}
