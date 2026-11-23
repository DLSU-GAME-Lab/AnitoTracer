#include "PostProcessStack.hpp"
#include "../PostProcess/GodRaysPipeline.hpp"
#include "../PostProcess/ColorCorrectionPipeline.hpp"

namespace Diligent {

    PostProcessStack::PostProcessStack(bool displayOnly)
    {
        if (!displayOnly) {
            // Light shafts go first so the display pass exposes, tone maps and grades them together with the scene
            AddEffect<GodRaysPipeline>();
        }

        // Display pass: must stay last, it is the only effect that writes the display format
        AddEffect<ColorCorrectionPipeline>();
    }

    void PostProcessStack::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT sceneFormat, TEXTURE_FORMAT displayFormat)
    {
        for (size_t i = 0; i < m_Effects.size(); ++i) {
            const bool isLast = (i + 1 == m_Effects.size());
            m_Effects[i]->Initialize(pDevice, sceneFormat, isLast ? displayFormat : sceneFormat);
        }
        m_ActiveEffects.clear();
    }

    ITextureView* PostProcessStack::Begin(const ProjectSettings& settings, const RenderData& renderData, const PostProcessView& view,
        ITextureView* pOutputRTV, Uint32 width, Uint32 height)
    {
        m_pOutputRTV = pOutputRTV;
        m_ActiveEffects.clear();

        for (auto& pEffect : m_Effects) {
            if (!pEffect->Prepare(settings, renderData, view, width, height)) continue;

            if (ITextureView* pInputRTV = pEffect->GetIntermediateRTV(width, height)) {
                m_ActiveEffects.push_back({ pEffect.get(), pInputRTV });
            }
        }

        return m_ActiveEffects.empty() ? pOutputRTV : m_ActiveEffects.front().pInputRTV;
    }

    void PostProcessStack::Apply(IDeviceContext* pContext)
    {
        for (size_t i = 0; i < m_ActiveEffects.size(); ++i) {
            ITextureView* pDstRTV = (i + 1 < m_ActiveEffects.size()) ? m_ActiveEffects[i + 1].pInputRTV : m_pOutputRTV;
            m_ActiveEffects[i].pEffect->Apply(pContext, pDstRTV);
        }
        m_ActiveEffects.clear();
    }

}
