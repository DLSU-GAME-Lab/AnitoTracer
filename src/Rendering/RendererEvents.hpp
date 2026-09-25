#pragma once
#include "Pipelines/PipelineDefs.hpp"

#include ANITO_EVENT_INCLUDES

struct RendererChangeArgs : public gbe::EventArgs {
    Diligent::PipelineType targetPipeline;
    RendererChangeArgs(Diligent::PipelineType type) : targetPipeline(type) {}
};

struct UpdateEditorOptionsArgs : public gbe::EventArgs {
    Diligent::PipelineType targetPipeline;

    bool showWireFrame = false;
    bool showSurfaces = true;
    glm::vec4 wireFrameColor = glm::vec4(0, 1, 0, 1);

    UpdateEditorOptionsArgs(bool _showWireFrame, bool _showSurfaces, glm::vec4 _wireFrameColor) :
        showWireFrame(_showWireFrame), showSurfaces(_showSurfaces), wireFrameColor(_wireFrameColor){}
};