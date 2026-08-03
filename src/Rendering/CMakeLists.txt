#SETUP
set(CUR_TARGET_SUBLIBNAME rendering)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Models/ModelStructs.hpp
	Models/ModelManager.hpp
	Models/ModelManager.cpp

	RenderData.hpp
	RenderData.cpp

	Shaders/ShaderManager.hpp
	Shaders/ShaderManager.cpp
	Shaders/VertexLayouts.hpp
	Shaders/VertexLayouts.cpp

	Pipelines/PipelineDefs.hpp

	Pipelines/BasePipeline.hpp
	Pipelines/BasePipeline.cpp

	Pipelines/BasicPipeline.hpp
	Pipelines/BasicPipeline.cpp

	Pipelines/TexturedPipeline.hpp
	Pipelines/TexturedPipeline.cpp

	Pipelines/BasicLitPipeline.cpp
	Pipelines/BasicLitPipeline.hpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PRIVATE
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
	Diligent-Common
	Diligent-GraphicsEngineVk-shared
	Diligent-TextureLoader
)

target_include_directories(${CUR_TARGET_NAME}
	PUBLIC
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

#ADDITIONAL STUFF