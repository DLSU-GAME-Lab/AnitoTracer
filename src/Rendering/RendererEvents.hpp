#pragma once
#include "Pipelines/PipelineDefs.hpp"

#include ANITO_EVENT_INCLUDES

struct RendererChangeArgs : public gbe::EventArgs {
    Diligent::PipelineType targetPipeline;
    RendererChangeArgs(Diligent::PipelineType type) : targetPipeline(type) {}
};