#pragma once

#include "Graphics/GraphicsEngine/interface/PipelineState.h"
#include "Graphics/GraphicsEngine/interface/ShaderResourceBinding.h"
#include "Common/interface/RefCntAutoPtr.hpp"

namespace Diligent {

    // One draw pass of a pipeline: its PSO and the SRB made from it. BasePipeline::BuildPass makes one and binds the
    // constant buffers the pipeline owns; PipelineUtils::DrawFullscreenPass draws one.
    struct PipelinePass {
        RefCntAutoPtr<IPipelineState> pPSO;
        RefCntAutoPtr<IShaderResourceBinding> pSRB;

        // False when the PSO could not be built
        explicit operator bool() const { return pPSO && pSRB; }
    };

}
