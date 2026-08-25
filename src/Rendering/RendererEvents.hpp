#pragma once

#include ANITO_EVENT_INCLUDES

struct RendererChangeArgs : public gbe::EventArgs {
    enum PipelineType { BASIC_LIT, HYBRID };

    PipelineType targetPipeline;
    RendererChangeArgs(PipelineType type) : targetPipeline(type) {}
};