set(RENDERING_SRC
	Shaders/ShaderLoader.hpp
	Shaders/ShaderLoader.cpp
	Shaders/ShaderBase.cpp
	Shaders/ShaderBase.hpp
	Shaders/SimpleShader.hpp
)

add_library(Rendering STATIC ${RENDERING_SRC})

target_include_directories(Rendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Rendering
						PRIVATE
						bgfx::bgfx
						glm::glm
)