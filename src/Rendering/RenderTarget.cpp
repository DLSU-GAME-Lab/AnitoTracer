#include "RenderTarget.hpp"

namespace Diligent {

    namespace {
        // For an sRGB color format: the typeless format to store it in and the plain one that reads the stored values
        bool SplitSrgbFormat(TEXTURE_FORMAT format, TEXTURE_FORMAT& typeless, TEXTURE_FORMAT& plain) {
            switch (format) {
            case TEX_FORMAT_RGBA8_UNORM_SRGB: typeless = TEX_FORMAT_RGBA8_TYPELESS; plain = TEX_FORMAT_RGBA8_UNORM; return true;
            case TEX_FORMAT_BGRA8_UNORM_SRGB: typeless = TEX_FORMAT_BGRA8_TYPELESS; plain = TEX_FORMAT_BGRA8_UNORM; return true;
            default: return false;
            }
        }
    }

    void RenderTarget::Create(IRenderDevice* pDevice, uint32_t width, uint32_t height, TEXTURE_FORMAT colorFmt, TEXTURE_FORMAT depthFmt, uint8_t sampleCount, bool shownInUI) {
        if (width == 0 || height == 0) return;
        if (IsValid() && m_Width == width && m_Height == height &&
            m_ColorFormat == colorFmt && m_DepthFormat == depthFmt && m_SampleCount == sampleCount &&
            m_ShownInUI == shownInUI) return; // Nothing changed

        Release();

        m_Width = width;
        m_Height = height;
        m_ColorFormat = colorFmt;
        m_DepthFormat = depthFmt;
        m_SampleCount = sampleCount;
        m_ShownInUI = shownInUI;

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

        // Shown in the UI: rendered through an sRGB view as before, read through a plain one (see Create in the header)
        TEXTURE_FORMAT typelessFmt = TEX_FORMAT_UNKNOWN;
        TEXTURE_FORMAT plainFmt = TEX_FORMAT_UNKNOWN;
        const bool twoViews = shownInUI && sampleCount == 1 && SplitSrgbFormat(colorFmt, typelessFmt, plainFmt);
        if (twoViews) colorDesc.Format = typelessFmt;

        pDevice->CreateTexture(colorDesc, nullptr, &m_pColorTexture);

        if (twoViews) {
            TextureViewDesc viewDesc;
            viewDesc.ViewType = TEXTURE_VIEW_RENDER_TARGET;
            viewDesc.Format = colorFmt;
            m_pColorTexture->CreateView(viewDesc, &m_pRTV);
            viewDesc.ViewType = TEXTURE_VIEW_SHADER_RESOURCE;
            viewDesc.Format = plainFmt;
            m_pColorTexture->CreateView(viewDesc, &m_pSRV);
        }
        else {
            m_pRTV = m_pColorTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
            if (sampleCount == 1) {
                m_pSRV = m_pColorTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
            }
        }

        TextureDesc depthDesc = colorDesc;
        depthDesc.Name = depthName.c_str();
        depthDesc.Format = depthFmt;
        depthDesc.BindFlags = sampleCount > 1 ? BIND_DEPTH_STENCIL : BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
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
