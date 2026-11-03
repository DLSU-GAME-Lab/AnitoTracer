#pragma once

#include <memory>
#include <vector>

#include "../PostProcess/PostProcessPipeline.hpp"

namespace Diligent {

    // Ordered chain of post effects for one view: scene -> effect 1 -> ... -> effect N -> output.
    // Each active effect owns the texture the previous stage renders into, so disabled effects cost nothing.
    //
    // To add an effect, derive from PostProcessEffect and add it in the constructor at the point in the chain where it should run.
    class PostProcessStack {
    public:
        PostProcessStack();

        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat);

        // Picks this view's active effects and returns where the scene should be rendered:
        // the first active effect's intermediate, or pOutputRTV when none run (or enabled is false)
        ITextureView* Begin(const ProjectSettings& settings, const RenderData& renderData, ITextureView* pOutputRTV,
            Uint32 width, Uint32 height, bool enabled = true);

        // Runs the effects chosen by Begin, the last one writing into the output
        void Apply(IDeviceContext* pContext);

    private:
        struct ActiveEffect {
            PostProcessPipeline* pEffect;
            ITextureView* pInputRTV; // The effect's intermediate, written by the previous stage
        };

        template <typename T>
        void AddEffect() { m_Effects.push_back(std::make_unique<T>()); }

        std::vector<std::unique_ptr<PostProcessPipeline>> m_Effects; // Execution order
        std::vector<ActiveEffect> m_ActiveEffects;
        ITextureView* m_pOutputRTV = nullptr;
    };

}
