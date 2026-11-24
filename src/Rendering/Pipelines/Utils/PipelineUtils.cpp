#include "PipelineUtils.hpp"
#include "../../../UserSettings.hpp"

namespace Diligent::PipelineUtils {

    Uint8 GetMSAASampleCount()
    {
        return UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;
    }

    RefCntAutoPtr<IBuffer> CreateDynamicCB(IRenderDevice* pDevice, const char* name, Uint64 size)
    {
        BufferDesc CBDesc;
        CBDesc.Name = name;
        CBDesc.Size = size;
        CBDesc.Usage = USAGE_DYNAMIC;
        CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
        CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

        RefCntAutoPtr<IBuffer> pBuffer;
        pDevice->CreateBuffer(CBDesc, nullptr, &pBuffer);
        return pBuffer;
    }

    RefCntAutoPtr<IShaderResourceBinding> CreateSRB(IPipelineState* pPSO)
    {
        RefCntAutoPtr<IShaderResourceBinding> pSRB;
        if (pPSO) {
            pPSO->CreateShaderResourceBinding(&pSRB, true);
        }
        return pSRB;
    }

    RefCntAutoPtr<ITexture> CreateRenderTarget(IRenderDevice* pDevice, const char* name, TEXTURE_FORMAT format, Uint32 width, Uint32 height)
    {
        TextureDesc Desc;
        Desc.Name = name;
        Desc.Type = RESOURCE_DIM_TEX_2D;
        Desc.Width = width;
        Desc.Height = height;
        Desc.Format = format;
        Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture(Desc, nullptr, &pTexture);
        return pTexture;
    }

    ITextureView* SRV(ITexture* pTexture)
    {
        return pTexture ? pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE) : nullptr;
    }

    ITextureView* RTV(ITexture* pTexture)
    {
        return pTexture ? pTexture->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET) : nullptr;
    }

    void BindVariable(IShaderResourceBinding* pSRB, SHADER_TYPE shaderType, const char* name, IDeviceObject* pObject, SET_SHADER_RESOURCE_FLAGS flags)
    {
        if (!pSRB) return;

        if (auto* pVar = pSRB->GetVariableByName(shaderType, name)) {
            pVar->Set(pObject, flags);
        }
    }

    SamplerDesc LinearWrapSampler()
    {
        SamplerDesc Desc;
        Desc.MinFilter = FILTER_TYPE_LINEAR;
        Desc.MagFilter = FILTER_TYPE_LINEAR;
        Desc.MipFilter = FILTER_TYPE_LINEAR;
        Desc.AddressU = TEXTURE_ADDRESS_WRAP;
        Desc.AddressV = TEXTURE_ADDRESS_WRAP;
        Desc.AddressW = TEXTURE_ADDRESS_WRAP;
        return Desc;
    }

    SamplerDesc AnisotropicWrapSampler(IRenderDevice* pDevice, Uint32 maxAnisotropy)
    {
        SamplerDesc Desc = LinearWrapSampler();

        const Uint32 deviceMax = pDevice ? pDevice->GetAdapterInfo().Sampler.MaxAnisotropy : 1;
        const Uint32 anisotropy = maxAnisotropy < deviceMax ? maxAnisotropy : deviceMax;
        if (anisotropy > 1) {
            Desc.MinFilter = FILTER_TYPE_ANISOTROPIC;
            Desc.MagFilter = FILTER_TYPE_ANISOTROPIC;
            Desc.MipFilter = FILTER_TYPE_ANISOTROPIC;
            Desc.MaxAnisotropy = anisotropy;
        }
        return Desc;
    }

    SamplerDesc LinearClampSampler()
    {
        SamplerDesc Desc = LinearWrapSampler();
        Desc.AddressU = TEXTURE_ADDRESS_CLAMP;
        Desc.AddressV = TEXTURE_ADDRESS_CLAMP;
        Desc.AddressW = TEXTURE_ADDRESS_CLAMP;
        return Desc;
    }

    SamplerDesc PointClampSampler()
    {
        SamplerDesc Desc = LinearClampSampler();
        Desc.MinFilter = FILTER_TYPE_POINT;
        Desc.MagFilter = FILTER_TYPE_POINT;
        Desc.MipFilter = FILTER_TYPE_POINT;
        return Desc;
    }

    RefCntAutoPtr<ITexture> CreateSolidColorTexture(IRenderDevice* pDevice, const char* name, Uint32 rgba, bool cube)
    {
        TextureDesc Desc;
        Desc.Name = name;
        Desc.Type = cube ? RESOURCE_DIM_TEX_CUBE : RESOURCE_DIM_TEX_2D;
        Desc.Width = 1;
        Desc.Height = 1;
        Desc.ArraySize = cube ? 6 : 1;
        Desc.Format = TEX_FORMAT_RGBA8_UNORM;
        Desc.Usage = USAGE_IMMUTABLE;
        Desc.BindFlags = BIND_SHADER_RESOURCE;

        TextureSubResData Faces[6];
        for (auto& face : Faces) {
            face.pData = &rgba;
            face.Stride = sizeof(rgba);
        }

        TextureData InitData{ Faces, Desc.ArraySize };
        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture(Desc, &InitData, &pTexture);
        return pTexture;
    }

    void SetViewport(IDeviceContext* pContext, Uint32 width, Uint32 height, Uint32 targetWidth, Uint32 targetHeight)
    {
        Viewport VP;
        VP.Width = static_cast<float>(width);
        VP.Height = static_cast<float>(height);
        VP.MinDepth = 0.0f;
        VP.MaxDepth = 1.0f;
        pContext->SetViewports(1, &VP, targetWidth, targetHeight);
    }

    void DrawFullscreenTriangle(IDeviceContext* pContext)
    {
        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 3;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(DrawAttrs);
    }

    void DrawFullscreenPass(IDeviceContext* pContext, IPipelineState* pPSO, IShaderResourceBinding* pSRB)
    {
        pContext->SetPipelineState(pPSO);
        pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        DrawFullscreenTriangle(pContext);
    }

    RenderTargetBlendDesc ColorBlend(BLEND_FACTOR srcFactor, BLEND_FACTOR destFactor)
    {
        RenderTargetBlendDesc blend;
        blend.BlendEnable = True;
        blend.SrcBlend = srcFactor;
        blend.DestBlend = destFactor;
        blend.BlendOp = BLEND_OPERATION_ADD;
        blend.SrcBlendAlpha = BLEND_FACTOR_ZERO;
        blend.DestBlendAlpha = BLEND_FACTOR_ONE;
        blend.BlendOpAlpha = BLEND_OPERATION_ADD;
        blend.RenderTargetWriteMask = COLOR_MASK_ALL;
        return blend;
    }

}
