#include "RenderTarget.hpp"

namespace Diligent {

    void RenderTarget::Create(IRenderDevice* pDevice, uint32_t width, uint32_t height, TEXTURE_FORMAT colorFmt, TEXTURE_FORMAT depthFmt, uint8_t sampleCount) {
        if (width == 0 || height == 0) return;
        if (IsValid() && m_Width == width && m_Height == height &&
            m_ColorFormat == colorFmt && m_DepthFormat == depthFmt && m_SampleCount == sampleCount) return; // Nothing changed

        Release();

        m_Width = width;
        m_Height = height;
        m_ColorFormat = colorFmt;
        m_DepthFormat = depthFmt;
        m_SampleCount = sampleCount;

        // Names must outlive CreateTexture, which reads TextureDesc::Name
        const std::string colorName = m_Name + " Color";
        const std::string depthName = m_Name + " Depth";

        TextureDesc colorDesc;
        colorDesc.Name = colorName.c_str();
        colorDesc.Type = RESOURCE_DIM_TEX_2D;
        colorDesc.Width = width;
        colorDesc.Height = height;
        colorDesc.Format = colorFmt;
        colorDesc.BindFlags = sampleCount > 1 ? BIND_RENDER_TARGET : BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
        colorDesc.SampleCount = sampleCount;
        pDevice->CreateTexture(colorDesc, nullptr, &m_pColorTexture);

        m_pRTV = m_pColorTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        if (sampleCount == 1) {
            m_pSRV = m_pColorTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        }

        TextureDesc depthDesc = colorDesc;
        depthDesc.Name = depthName.c_str();
        depthDesc.Format = depthFmt;
        depthDesc.BindFlags = BIND_DEPTH_STENCIL;
        pDevice->CreateTexture(depthDesc, nullptr, &m_pDepthTexture);

        m_pDSV = m_pDepthTexture->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
    }

    void RenderTarget::Release() {
        m_pColorTexture.Release();
        m_pDepthTexture.Release();
        m_pRTV.Release();
        m_pSRV.Release();
        m_pDSV.Release();
        m_Width = 0;
        m_Height = 0;
    }
}
