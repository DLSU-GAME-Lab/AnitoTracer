#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "Graphics/GraphicsEngine/interface/TextureView.h"

namespace Diligent {

    enum class ParticleBlendMode : int {
        Alpha = 0,
        Additive = 1
    };

    // Per-particle GPU data. Layout must match the instance input layout in ParticlePipeline.
    struct ParticleInstance {
        glm::vec3 Position{ 0.0f };
        float Rotation = 0.0f;          // radians, roll around the view axis
        glm::vec2 Size{ 1.0f };
        float Frame = 0.0f;             // flipbook frame index
        float Padding = 0.0f;
        glm::vec4 Color{ 1.0f };
    };

    // All particles that share a texture / flipbook grid / blend mode are drawn with one instanced draw.
    struct ParticleBatch {
        ITextureView* Texture = nullptr;    // null -> pipeline falls back to a white texture
        ParticleBlendMode Blend = ParticleBlendMode::Alpha;
        uint32_t Columns = 1;
        uint32_t Rows = 1;
        std::vector<ParticleInstance> Instances;
    };

}
