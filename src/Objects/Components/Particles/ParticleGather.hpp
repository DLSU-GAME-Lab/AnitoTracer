#pragma once

#include <vector>
#include "ParticleRenderData.hpp"

// Walks the whole hierarchy and collects a render batch from every ParticleSystemComponent.
void GatherParticleBatches(std::vector<Diligent::ParticleBatch>& outBatches);
