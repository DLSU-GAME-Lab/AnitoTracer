#pragma once
#include "StylizedPipeline.hpp"

namespace Diligent {
    // Anime (cel) shading, ported from Weeb_Big2 (Shaders/Anime). Forward, two passes:
    //   1. Toon pass    - two-tone lighting switched by a step on N.L, optional specular, ink along inner edges and
    //                     creases, in the style set by ProjectAnimeSettings (anime_vs/ps.hlsl)
    //   2. Outline pass - inverted hull: the mesh pushed outward and drawn in ink with front faces culled (outline_vs/ps.hlsl)
    // Both passes read a second vertex stream, Model::pAnimeVertexBuffer, which holds each vertex's smooth normal and
    // curvature (built at load by BuildAnimeVertexData).
    class AnimePipeline : public StylizedPipeline {
    public:
        AnimePipeline() : StylizedPipeline("AnimeConstants") {}

        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;

    protected:
        void CreatePasses() override;
        void BindModelVertexBuffers(IDeviceContext* pContext, const Model& model) override;
    };
}
