#pragma once

#include <variant>
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/RefCntAutoPtr.hpp"

#include "Pipelines/BasicLitPipeline.hpp"
#include "Pipelines/HybridPipeline.hpp"
#include "RenderData.hpp"

#include "../UserSettings.hpp"

class RendererManager {
public:
    static RendererManager& GetInstance() {
        static RendererManager instance;
        return instance;
    }

    RendererManager(const RendererManager&) = delete;
    void operator=(const RendererManager&) = delete;

    void Initialize(Diligent::IRenderDevice* pDevice, Diligent::IDeviceContext* pContext, Diligent::ISwapChain* pSwapChain, bool supportsRayTracing);
    void InitializePipelines();

    void RenderFrame(const Diligent::RenderData& renderData);
    void OnResize(Diligent::Uint32 width, Diligent::Uint32 height);
    void Shutdown();

private:
    RendererManager() = default;
    ~RendererManager() = default;

    void CreateMSAABuffers();

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;

    Diligent::RefCntAutoPtr<Diligent::ITexture>     m_pMSAATarget;
    Diligent::RefCntAutoPtr<Diligent::ITexture>     m_pMSAADepth;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pMSAARTV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> m_pMSAADSV;

    std::variant<Diligent::HybridPipeline, Diligent::BasicLitPipeline> m_bLitPipeline;
    bool m_LastMSAAState = false;
};