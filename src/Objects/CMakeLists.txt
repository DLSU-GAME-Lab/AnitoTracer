set(OBJ_SRC
	Models/ModelManager.hpp
	Models/ModelManager.cpp

	CameraObj.cpp
	CameraObj.hpp
)

add_library(Objects STATIC ${OBJ_SRC})

target_link_libraries(Objects
			PUBLIC
			glm::glm
			assimp::assimp
			Diligent-Common
			Diligent-GraphicsEngineVk-shared
			Diligent-Imgui
			Diligent-TextureLoader
)

target_include_directories(Objects
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_include_directories(Objects PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})