#include "GBuffer.hpp"
#include "PipelineUtils.hpp"
#include <algorithm>

namespace Diligent {

    void GBuffer::Configure(std::vector<Attachment> colors, Attachment depth, bool growOnly)
    {
        Release();
        m_Colors = std::move(colors);
        m_Depth = depth;
        m_GrowOnly = growOnly;
    }

    bool GBuffer::Resize(IRenderDevice* pDevice, Uint32 width, Uint32 height)
    {
        if (!IsConfigured() || width == 0 || height == 0) return false;

        if (IsCreated()) {
            const bool fits = m_GrowOnly ? (width <= m_Width && height <= m_Height) : (width == m_Width && height == m_Height);
            if (fits) return false;
        }

        if (m_GrowOnly) {
            width = (std::max)(width, m_Width);
            height = (std::max)(height, m_Height);
        }
        Release();
        m_Width = width;
        m_Height = height;

        TextureDesc TexDesc;
        TexDesc.Type = RESOURCE_DIM_TEX_2D;
        TexDesc.Width = width;
        TexDesc.Height = height;
        TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

        for (const Attachment& attachment : m_Colors) {
            TexDesc.Name = attachment.Name;
            TexDesc.Format = attachment.Format;
            TexDesc.ClearValue.Format = attachment.Format;

            RefCntAutoPtr<ITexture> pTexture;
            pDevice->CreateTexture(TexDesc, nullptr, &pTexture);
            m_RTVs.push_back(pTexture ? pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) : nullptr);
            m_ColorTextures.push_back(pTexture);
        }

        TexDesc.Name = m_Depth.Name;
        TexDesc.Format = m_Depth.Format;
        TexDesc.BindFlags = m_Depth.ShaderVariable ? BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE : BIND_DEPTH_STENCIL;
        TexDesc.ClearValue.Format = m_Depth.Format;
        TexDesc.ClearValue.DepthStencil.Depth = 1.0f;
        pDevice->CreateTexture(TexDesc, nullptr, &m_pDepthTexture);

        return true;
    }

    void GBuffer::Release()
    {
        m_ColorTextures.clear();
        m_RTVs.clear();
        m_pDepthTexture.Release();
        m_Width = 0;
        m_Height = 0;
    }

    void GBuffer::SetRenderTargets(IDeviceContext* pContext, bool depthOnly) const
    {
        if (depthOnly) {
            pContext->SetRenderTargets(0, nullptr, GetDSV(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
        else {
            pContext->SetRenderTargets(static_cast<Uint32>(m_RTVs.size()), const_cast<ITextureView**>(m_RTVs.data()), GetDSV(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
    }

    void GBuffer::Clear(IDeviceContext* pContext, bool clearColor) const
    {
        if (clearColor) {
            const float ClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            for (ITextureView* pRTV : m_RTVs) {
                pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
        }
        pContext->ClearDepthStencil(GetDSV(), CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void GBuffer::BindToSRB(IShaderResourceBinding* pSRB) const
    {
        if (!pSRB || !IsCreated()) return;

        for (size_t i = 0; i < m_Colors.size(); ++i) {
            if (m_Colors[i].ShaderVariable && m_ColorTextures[i]) {
                PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, m_Colors[i].ShaderVariable,
                    m_ColorTextures[i]->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
            }
        }

        if (m_Depth.ShaderVariable) {
            PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, m_Depth.ShaderVariable,
                m_pDepthTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        }
    }

    std::vector<TEXTURE_FORMAT> GBuffer::GetColorFormats() const
    {
        std::vector<TEXTURE_FORMAT> formats;
        for (const Attachment& attachment : m_Colors) {
            formats.push_back(attachment.Format);
        }
        return formats;
    }

    ITextureView* GBuffer::GetDSV() const
    {
        return m_pDepthTexture ? m_pDepthTexture->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL) : nullptr;
    }

}
