set(UI_SRC
	GUIManager.cpp
	GUIManager.hpp
)

add_library(UI STATIC ${UI_SRC})

target_link_libraries(UI
			PUBLIC
			glm::glm
			Diligent-GraphicsEngineVk-shared
			Diligent-Imgui
)

target_include_directories(UI
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_include_directories(UI PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})