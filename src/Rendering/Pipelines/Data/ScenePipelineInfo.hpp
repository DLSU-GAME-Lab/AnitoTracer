#pragma once

#include <memory>

#include "PipelineDefs.hpp"

namespace Diligent {

    class ScenePipeline;

    // One selectable scene pipeline, as listed in ScenePipelineRegistry
    struct ScenePipelineInfo {
        PipelineType Type;
        const char* DisplayName;    // Shown in the Rendering menu and logs
        bool RequiresRayTracing;
        bool SupportsMSAA;
        std::unique_ptr<ScenePipeline>(*Create)();
    };

}
