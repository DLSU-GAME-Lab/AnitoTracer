#include "ScenePipelineRegistry.hpp"
#include "../BasicLitPipeline.hpp"
#include "../DeferredPipeline.hpp"
#include "../HybridPipeline.hpp"
#include "../DeferredHybridPipeline.hpp"

namespace Diligent {

    namespace {
        template <typename T>
        std::unique_ptr<ScenePipeline> Make() { return std::make_unique<T>(); }

        // Tried in order when the requested pipeline can't run on this device / with these settings
        constexpr PipelineType kFallbackOrder[] = { PipelineType::HYBRID, PipelineType::BASIC_LIT };
    }

    const std::vector<ScenePipelineInfo>& ScenePipelineRegistry::GetAll()
    {
        // Menu order
        static const std::vector<ScenePipelineInfo> pipelines = {
            //  Type                           Display name                              RT     MSAA   Factory
            { PipelineType::BASIC_LIT,       "Basic Lit Pipeline",                     false, true,  &Make<BasicLitPipeline> },
            // TODO: MSAA for deferred
            { PipelineType::DEFERRED,        "Deferred Pipeline",                      false, false, &Make<DeferredPipeline> },
            { PipelineType::HYBRID,          "Hybrid Pipeline (RayTraced)",            true,  true,  &Make<HybridPipeline> },
            { PipelineType::DEFERRED_HYBRID, "Deferred Hybrid Pipeline (RayTraced)",   true,  true,  &Make<DeferredHybridPipeline> },
        };
        return pipelines;
    }

    const ScenePipelineInfo* ScenePipelineRegistry::Find(PipelineType type)
    {
        for (const ScenePipelineInfo& info : GetAll()) {
            if (info.Type == type) return &info;
        }
        return nullptr;
    }

    bool ScenePipelineRegistry::CanRun(const ScenePipelineInfo& info, bool rayTracingSupported, bool msaaEnabled)
    {
        return (rayTracingSupported || !info.RequiresRayTracing) && (!msaaEnabled || info.SupportsMSAA);
    }

    const ScenePipelineInfo& ScenePipelineRegistry::Resolve(PipelineType requested, bool rayTracingSupported, bool msaaEnabled)
    {
        if (const ScenePipelineInfo* info = Find(requested); info && CanRun(*info, rayTracingSupported, msaaEnabled)) {
            return *info;
        }

        for (PipelineType fallback : kFallbackOrder) {
            if (const ScenePipelineInfo* info = Find(fallback); info && CanRun(*info, rayTracingSupported, msaaEnabled)) {
                return *info;
            }
        }
        return *Find(PipelineType::BASIC_LIT);
    }

}
