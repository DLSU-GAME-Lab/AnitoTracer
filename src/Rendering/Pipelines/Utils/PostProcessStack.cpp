#include "PostProcessStack.hpp"
#include "../PostProcess/GodRaysPipeline.hpp"
#include "../PostProcess/ColorCorrectionPipeline.hpp"

namespace Diligent {

    PostProcessStack::PostProcessStack()
    {
        // Light shafts go first so color correction grades them together with the scene
        AddEffect<GodRaysPipeline>();
        AddEffect<ColorCorrectionPipeline>();
    }

    void PostProcessStack::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat)
    {
        for (auto& pEffect : m_Effects) {
            pEffect->Initialize(pDevice, RTVFormat);
        }
        m_ActiveEffects.clear();
    }

    ITextureView* PostProcessStack::Begin(const ProjectSettings& settings, const RenderData& renderData, ITextureView* pOutputRTV,
        Uint32 width, Uint32 height, bool enabled)
    {
        m_pOutputRTV = pOutputRTV;
        m_ActiveEffects.clear();
        if (!enabled) return pOutputRTV;

        for (auto& pEffect : m_Effects) {
            if (!pEffect->Prepare(settings, renderData, width, height)) continue;

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
