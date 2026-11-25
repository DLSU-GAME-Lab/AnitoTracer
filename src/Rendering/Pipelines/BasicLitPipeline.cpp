#include "BasicLitPipeline.hpp"

void Diligent::BasicLitPipeline::CreatePasses()
{
    BuildMainPass(CreateModelPSOBuilder("PBR Fallback (Non-RT) Rendering PSO", "fallbackLit_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount()));
}
