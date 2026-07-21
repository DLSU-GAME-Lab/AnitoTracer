set(UI_SRC
	Panels/BasePanel.cpp
	Panels/BasePanel.hpp
	Panels/PropertiesPanel.cpp
	Panels/PropertiesPanel.hpp

	Panels/InspectorPanel/InspectorRegistry.cpp
	Panels/InspectorPanel/InspectorRegistry.hpp
	Panels/InspectorPanel/InspectorPanel.cpp
	Panels/InspectorPanel/InspectorPanel.hpp
	Panels/InspectorPanel/Components/IComponentUI.hpp
	Panels/InspectorPanel/Components/TransformUI.hpp
	Panels/InspectorPanel/Components/CameraUI.hpp

	Panels/HierarchyPanel.cpp
	Panels/HierarchyPanel.hpp

	MenuBar.hpp
	MenuBar.cpp

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