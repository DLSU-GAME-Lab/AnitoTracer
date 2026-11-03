#pragma once

#include <vector>

#include "../ScenePipeline.hpp"
#include "../Data/ScenePipelineInfo.hpp"

namespace Diligent {

    // Every selectable scene pipeline. Adding one = a PipelineType value plus an entry in ScenePipelineRegistry.cpp;
    // RendererManager and the Rendering menu pick it up from here.
    class ScenePipelineRegistry {
    public:
        static const std::vector<ScenePipelineInfo>& GetAll();
        static const ScenePipelineInfo* Find(PipelineType type);

        static bool CanRun(const ScenePipelineInfo& info, bool rayTracingSupported, bool msaaEnabled);

        // The requested pipeline if it can run, otherwise the first fallback that can (ending with BASIC_LIT, which always can)
        static const ScenePipelineInfo& Resolve(PipelineType requested, bool rayTracingSupported, bool msaaEnabled);
    };

}
