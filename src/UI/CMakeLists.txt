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
	Panels/InspectorPanel/Components/AnimationComponentUI.hpp

	Panels/HierarchyPanel.cpp
	Panels/HierarchyPanel.hpp
	Panels/UserSettingsPanel.cpp
	Panels/UserSettingsPanel.hpp
	Panels/ProfilerPanel.cpp
	Panels/ProfilerPanel.hpp
	Panels/FileExplorerPanel.cpp
	Panels/FileExplorerPanel.hpp

	Panels/ViewportPanel/ViewportPanel.cpp
	Panels/ViewportPanel/ViewportPanel.hpp
	Panels/ViewportPanel/EditorPanel.cpp
	Panels/ViewportPanel/EditorPanel.hpp
	Panels/ViewportPanel/GamePanel.cpp
	Panels/ViewportPanel/GamePanel.hpp

	Panels/FileExplorerPanel.cpp           
    Panels/FileOpeners/SceneFileOpener.cpp 
	Panels/FileOpeners/PrefabFileOpener.cpp
	Panels/FileOpeners/ModelFileOpener.cpp 
	Panels/FileOpeners/ScriptFileOpener.cpp
	Panels/FileOpeners/AnimationFileOpener.cpp
	Panels/FileOpeners/AnimationFileOpener.hpp

	Panels/ScriptEditorPanel.cpp
	Panels/ScriptEditorPanel.hpp
	Panels/AnimationEditorPanel.cpp
	Panels/AnimationEditorPanel.hpp
	Panels/AnimationEditor/AnimationEditorDocument.cpp
	Panels/AnimationEditor/AnimationEditorDocument.hpp
	Panels/ScriptEditor/CaretNavigator.cpp
	Panels/ScriptEditor/CaretNavigator.hpp
	Panels/ScriptEditor/EditorCommands.cpp
	Panels/ScriptEditor/EditorCommands.hpp
	Panels/ScriptEditor/EditorDocument.cpp
	Panels/ScriptEditor/EditorDocument.hpp
	Panels/ScriptEditor/EditorKeyBindings.cpp
	Panels/ScriptEditor/EditorKeyBindings.hpp
	Panels/ScriptEditor/LineLayout.cpp
	Panels/ScriptEditor/LineLayout.hpp
	Panels/ScriptEditor/ScriptEditorView.cpp
	Panels/ScriptEditor/ScriptEditorView.hpp
	Panels/ScriptEditor/TextBuffer.cpp
	Panels/ScriptEditor/TextBuffer.hpp
	Panels/ScriptEditor/TextTypes.hpp
	Panels/ScriptEditor/UndoHistory.cpp
	Panels/ScriptEditor/UndoHistory.hpp
	Panels/ScriptEditor/Utf8.hpp

	FileDialog.hpp
	FileDialog.cpp

	MenuBar.cpp

	GUIManager.cpp
	GUIManager.hpp
	DockLayoutScaler.cpp
	DockLayoutScaler.hpp

	ObjectPicker.hpp
	ObjectPicker.cpp

	Gizmos/ImGuizmo.h
	Gizmos/ImGuizmo.cpp
	Gizmos/GizmoDrawer.hpp
	Gizmos/GizmoDrawer.cpp
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
	anito::event
	anito::rendering
	anito::objects
	anito::scripting
	

	Diligent-GraphicsEngineVk-shared
)

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	Diligent-Imgui

	anito::common
	anito::scriptlang

	gbe::tools
)

#ADDITIONAL STUFF

if(BUILD_TESTING)
	add_executable(dock_layout_scaler_tests
		tests/DockLayoutScalerTests.cpp
		DockLayoutScaler.cpp
	)
	target_include_directories(dock_layout_scaler_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_compile_definitions(dock_layout_scaler_tests PRIVATE
		ANITO_DEFAULT_LAYOUT="${PROJECT_SOURCE_DIR}/Assets/Default Layouts/Default.ini")
	target_link_libraries(dock_layout_scaler_tests PRIVATE Diligent-Imgui)
	add_test(NAME dock_layout_scaler_tests COMMAND dock_layout_scaler_tests)

	add_executable(script_editor_tests
		Panels/ScriptEditor/tests/ScriptEditorTests.cpp
		Panels/ScriptEditor/CaretNavigator.cpp
		Panels/ScriptEditor/EditorCommands.cpp
		Panels/ScriptEditor/EditorDocument.cpp
		Panels/ScriptEditor/EditorKeyBindings.cpp
		Panels/ScriptEditor/LineLayout.cpp
		Panels/ScriptEditor/TextBuffer.cpp
		Panels/ScriptEditor/UndoHistory.cpp
		Panels/FileOpeners/ScriptFileOpener.cpp
	)
	target_link_libraries(script_editor_tests PRIVATE Diligent-Imgui anito::scriptlang)
	add_test(NAME script_editor_tests COMMAND script_editor_tests)

	add_executable(animation_editor_tests
		Panels/AnimationEditor/tests/AnimationEditorTests.cpp
		Panels/AnimationEditor/AnimationEditorDocument.cpp
		Panels/FileOpeners/AnimationFileOpener.cpp
	)
	target_link_libraries(animation_editor_tests PRIVATE anito::asset)
	add_test(NAME animation_editor_tests COMMAND animation_editor_tests)
endif()