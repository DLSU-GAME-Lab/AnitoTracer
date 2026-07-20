set(Rendering_SRC
	Shaders/ShaderManager.hpp
	Shaders/ShaderManager.cpp
	Shaders/VertexLayouts.hpp
	Shaders/VertexLayouts.cpp

	BasicPipeline.hpp
	BasicPipeline.cpp

	TexturedPipeline.hpp
	TexturedPipeline.cpp
)

add_library(Rendering STATIC ${Rendering_SRC})

target_link_libraries(Rendering
			PUBLIC
			glm::glm
			Diligent-Common
			Diligent-GraphicsEngineVk-shared
)

target_include_directories(Rendering
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_include_directories(Rendering PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})