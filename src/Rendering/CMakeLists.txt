#SETUP
set(CUR_TARGET_SUBLIBNAME rendering)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Models/ModelStructs.hpp
	Models/ModelManager.hpp
	Models/ModelManager.cpp

	RenderData.hpp
	RenderData.cpp

	RendererEvents.hpp

	FrameTarget.hpp
	RenderTarget.hpp
	RenderTarget.cpp

	SceneConstants.hpp
	SceneConstants.cpp

	RendererManager.hpp
	RendererManager.cpp

	Shaders/ShaderManager.hpp
	Shaders/ShaderManager.cpp
	Shaders/VertexLayouts.hpp
	Shaders/VertexLayouts.cpp

	# Pipeline data (structs mirroring shader constants, registry entries)
	Pipelines/Data/PipelineDefs.hpp
	Pipelines/Data/ScenePipelineInfo.hpp

	# Pipeline utilities
	Pipelines/Utils/PipelineUtils.hpp
	Pipelines/Utils/PipelineUtils.cpp
	Pipelines/Utils/PSOBuilder.hpp
	Pipelines/Utils/PSOBuilder.cpp
	Pipelines/Utils/GBuffer.hpp
	Pipelines/Utils/GBuffer.cpp
	Pipelines/Utils/SceneTLAS.hpp
	Pipelines/Utils/SceneTLAS.cpp
	Pipelines/Utils/ScenePipelineRegistry.hpp
	Pipelines/Utils/ScenePipelineRegistry.cpp
	Pipelines/Utils/PostProcessStack.hpp
	Pipelines/Utils/PostProcessStack.cpp

	# Render pipelines
	Pipelines/BasePipeline.hpp
	Pipelines/BasePipeline.cpp

	# Scene pipelines (selectable renderers, see ScenePipelineRegistry)
	Pipelines/ScenePipeline.hpp
	Pipelines/ScenePipeline.cpp
	Pipelines/BasicLitPipeline.hpp
	Pipelines/BasicLitPipeline.cpp
	Pipelines/HybridPipeline.hpp
	Pipelines/HybridPipeline.cpp
	Pipelines/DeferredPipeline.hpp
	Pipelines/DeferredPipeline.cpp
	Pipelines/DeferredHybridPipeline.hpp
	Pipelines/DeferredHybridPipeline.cpp

	# Other pipelines
	Pipelines/BasicPipeline.hpp
	Pipelines/BasicPipeline.cpp
	Pipelines/TexturedPipeline.hpp
	Pipelines/TexturedPipeline.cpp

	Pipelines/DebugLinePipeline.hpp
	Pipelines/DebugLinePipeline.cpp

	ParticleRenderData.hpp
	Pipelines/ParticlePipeline.hpp
	Pipelines/ParticlePipeline.cpp

	Pipelines/SkyboxPipeline.hpp
	Pipelines/SkyboxPipeline.cpp

	# Post processing pipelines (chained by Utils/PostProcessStack)
	Pipelines/PostProcess/PostProcessPipeline.hpp
	Pipelines/PostProcess/PostProcessPipeline.cpp
	Pipelines/PostProcess/ColorCorrectionPipeline.hpp
	Pipelines/PostProcess/ColorCorrectionPipeline.cpp
	Pipelines/PostProcess/GodRaysPipeline.hpp
	Pipelines/PostProcess/GodRaysPipeline.cpp

	# Temporal anti-aliasing (runs before the post process stack, with a history per view)
	Pipelines/PostProcess/TemporalAAPass.hpp
	Pipelines/PostProcess/TemporalAAPass.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} 
	PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
		${diligentcore_SOURCE_DIR}
		${diligenttools_SOURCE_DIR}
		${diligentfx_SOURCE_DIR}
)

#PRIVATE

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
	Diligent-Common
	Diligent-GraphicsEngineVk-shared
	Diligent-TextureLoader

	anito::asset
	anito::event
)

target_include_directories(${CUR_TARGET_NAME}
	PUBLIC
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

#ADDITIONAL STUFF