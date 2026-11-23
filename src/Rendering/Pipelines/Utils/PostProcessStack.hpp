#pragma once

#include <memory>
#include <vector>

#include "../PostProcess/PostProcessPipeline.hpp"

namespace Diligent {

    // Ordered chain of post effects for one view: scene -> effect 1 -> ... -> display pass -> output.
    // The scene and every effect work in the HDR scene format; the display pass (ColorCorrectionPipeline) is always last
    // and always runs, since it is what converts the HDR image into the 8-bit output. Each active effect owns the texture
    // the previous stage renders into, so disabled effects cost nothing.
    //
    // To add an effect, derive from PostProcessEffect and add it in the constructor, before the display pass,
    // at the point in the chain where it should run.
    class PostProcessStack {
    public:
        // displayOnly leaves out the optional effects, for views that never use them (the editor view)
        explicit PostProcessStack(bool displayOnly = false);

        // sceneFormat is what the scene is rendered in and the effects exchange; displayFormat is the output target's format
        void Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT sceneFormat, TEXTURE_FORMAT displayFormat);

        // Picks this view's active effects and returns where the scene should be rendered: the first active effect's intermediate.
        // Never pOutputRTV itself, unless the intermediate textures could not be created.
        ITextureView* Begin(const ProjectSettings& settings, const RenderData& renderData, const PostProcessView& view,
            ITextureView* pOutputRTV, Uint32 width, Uint32 height);

        // Runs the effects chosen by Begin, the last one writing into the output
        void Apply(IDeviceContext* pContext);

    private:
        struct ActiveEffect {
            PostProcessPipeline* pEffect;
            ITextureView* pInputRTV; // The effect's intermediate, written by the previous stage
        };

        template <typename T>
        void AddEffect() { m_Effects.push_back(std::make_unique<T>()); }

        std::vector<std::unique_ptr<PostProcessPipeline>> m_Effects; // Execution order, the display pass last
        std::vector<ActiveEffect> m_ActiveEffects;
        ITextureView* m_pOutputRTV = nullptr;
    };

}
