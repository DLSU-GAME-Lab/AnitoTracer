#SETUP
set(CUR_TARGET_SUBLIBNAME ui)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Panels/BasePanel.cpp
	Panels/BasePanel.hpp

	Panels/InspectorPanel/InspectorRegistry.cpp
	Panels/InspectorPanel/InspectorRegistry.hpp
	Panels/InspectorPanel/InspectorPanel.cpp
	Panels/InspectorPanel/InspectorPanel.hpp
	Panels/InspectorPanel/Components/IComponentUI.hpp
	Panels/InspectorPanel/Components/TransformUI.hpp
	Panels/InspectorPanel/Components/CameraUI.hpp
	Panels/InspectorPanel/Components/DirectionalLightUI.hpp
	Panels/InspectorPanel/Components/PointLightUI.hpp

	Panels/HierarchyPanel.cpp
	Panels/HierarchyPanel.hpp
	Panels/UserSettingsPanel.cpp
	Panels/UserSettingsPanel.hpp
	Panels/ProfilerPanel.cpp
	Panels/ProfilerPanel.hpp

	MenuBar.hpp
	MenuBar.cpp

	FileDialog.hpp
	FileDialog.cpp

	GUIManager.cpp
	GUIManager.hpp

	ObjectPicker.hpp
	ObjectPicker.cpp
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
target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	glm::glm

	anito::serialization
	gbe::filedialogue
	anito::rendering
	anito::objects
	

	Diligent-GraphicsEngineVk-shared
)

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	Diligent-Imgui

	anito::common

	gbe::tools
)

#ADDITIONAL STUFF