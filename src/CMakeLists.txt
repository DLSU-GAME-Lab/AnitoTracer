	set(exe_name ${MAIN_PROJECT})

set(src_files_assets
	Assets/CornellBox.cpp
	Assets/CornellBox.hpp
	Assets/Material.hpp
	Assets/Model.cpp
	Assets/Model.hpp
	Assets/Procedural.hpp
	Assets/RayScene.cpp
	Assets/RayScene.hpp
	Assets/Scene.cpp
	Assets/Scene.hpp
	Assets/SphereProc.hpp
	Assets/Texture.cpp
	Assets/Texture.hpp
	Assets/CubeMapTexture.hpp
	Assets/TextureImage.cpp
	Assets/TextureImage.hpp
	Assets/UniformBuffer.cpp
	Assets/UniformBuffer.hpp
	Assets/Vertex.hpp
	Assets/Box.cpp
	Assets/Box.hpp
	Assets/Capsule.cpp
	Assets/Capsule.hpp
	Assets/Cylinder.cpp
	Assets/Cylinder.hpp
	Assets/Plane.cpp
	Assets/Plane.hpp
	Assets/Sphere.cpp
	Assets/Sphere.hpp
	Assets/Ray.cpp
	Assets/Ray.hpp
	Assets/RayVertex.hpp
	Assets/RayInfo.hpp
	Assets/ModelLibrary.hpp
	Assets/ModelLibrary.cpp
	Assets/GameObjectFactory.hpp
	Assets/GameObjectFactory.cpp
)

set(src_files_obb
	OBB/BoundingBox.cpp
	OBB/BoundingBox.hpp
	OBB/Ray.cpp
	OBB/Ray.hpp
)

set(src_files_camerasystem
	Engine/CameraSystem/Camera.cpp
	Engine/CameraSystem/Camera.h
	Engine/CameraSystem/CameraManager.cpp
	Engine/CameraSystem/CameraManager.h
	Engine/CameraSystem/SceneCamera.cpp
	Engine/CameraSystem/SceneCamera.h
)

set(src_files_animationsystem
	Engine/AnimationSystem/Animation.cpp
	Engine/AnimationSystem/Animation.h
	Engine/AnimationSystem/AnimationFrame.cpp
	Engine/AnimationSystem/AnimationFrame.h
)

set(src_files_lightsystem
	Engine/LightSystem/Light.cpp
	Engine/LightSystem/Light.h
)

set(src_files_profiler
	Engine/Profiler/Profiler.cpp
	Engine/Profiler/Profiler.h
)

set(src_files_sceneio
	Engine/Scene/SceneIO.cpp
	Engine/Scene/SceneIO.hpp
)

set(src_files_ray_visualization
	RayVisualization/RayVisualizationPipeline.cpp
	RayVisualization/RayVisualizationPipeline.h
)

set(src_files_imgui
	ImGui/imgui_freetype.cpp
	ImGui/imgui_freetype.h
	ImGui/imgui_impl_glfw.cpp
	ImGui/imgui_impl_glfw.h
	ImGui/imgui_impl_vulkan.cpp
	ImGui/imgui_impl_vulkan.h
	ImGui/imgui_stdlib.cpp
	ImGui/imgui_stdlib.h
	ImGui/ImGuizmo.cpp
	ImGui/ImGuizmo.h
)

set(src_files_utilities
	Utilities/Console.cpp
	Utilities/Console.hpp
	Utilities/Exception.hpp
	Utilities/Glm.hpp
	Utilities/StbImage.cpp
	Utilities/StbImage.hpp
	Utilities/FileUtils.cpp
	Utilities/FileUtils.h
	Utilities/Screenshot.hpp
	Utilities/Video/VideoExporter.cpp
	Utilities/Video/VideoExporter.h
)

set(src_files_vulkan
	Vulkan/Application.cpp
	Vulkan/Application.hpp
	Vulkan/Buffer.cpp
	Vulkan/Buffer.hpp
	Vulkan/BufferUtil.hpp
	Vulkan/CommandBuffers.cpp
	Vulkan/CommandBuffers.hpp
	Vulkan/CommandPool.cpp
	Vulkan/CommandPool.hpp
	Vulkan/DebugUtils.cpp
	Vulkan/DebugUtils.hpp
	Vulkan/DebugUtilsMessenger.cpp
	Vulkan/DebugUtilsMessenger.hpp
	Vulkan/DepthBuffer.cpp
	Vulkan/DepthBuffer.hpp
	Vulkan/DescriptorBinding.hpp
	Vulkan/DescriptorPool.cpp
	Vulkan/DescriptorPool.hpp
	Vulkan/DescriptorSetLayout.cpp
	Vulkan/DescriptorSetLayout.hpp
	Vulkan/DescriptorSetManager.cpp
	Vulkan/DescriptorSetManager.hpp
	Vulkan/DescriptorSets.cpp
	Vulkan/DescriptorSets.hpp
	Vulkan/Device.cpp
	Vulkan/Device.hpp
	Vulkan/DeviceMemory.cpp
	Vulkan/DeviceMemory.hpp
	Vulkan/Enumerate.hpp
	Vulkan/Fence.cpp
	Vulkan/Fence.hpp
	Vulkan/FrameBuffer.cpp
	Vulkan/FrameBuffer.hpp
	Vulkan/GraphicsPipeline.cpp
	Vulkan/GraphicsPipeline.hpp
	Vulkan/Image.cpp
	Vulkan/Image.hpp	
	Vulkan/ImageMemoryBarrier.hpp	
	Vulkan/ImageView.cpp
	Vulkan/ImageView.hpp	
	Vulkan/Instance.cpp
	Vulkan/Instance.hpp
	Vulkan/PipelineLayout.cpp
	Vulkan/PipelineLayout.hpp
	Vulkan/RenderPass.cpp
	Vulkan/RenderPass.hpp
	Vulkan/Sampler.cpp
	Vulkan/Sampler.hpp
	Vulkan/Semaphore.cpp
	Vulkan/Semaphore.hpp
	Vulkan/ShaderModule.cpp
	Vulkan/ShaderModule.hpp	
	Vulkan/SingleTimeCommands.hpp
	Vulkan/Strings.cpp
	Vulkan/Strings.hpp	
	Vulkan/Surface.cpp
	Vulkan/Surface.hpp	
	Vulkan/SwapChain.cpp
	Vulkan/SwapChain.hpp
	Vulkan/Version.hpp
	Vulkan/Vulkan.cpp
	Vulkan/Vulkan.hpp
	Vulkan/Window.cpp
	Vulkan/Window.hpp
	Vulkan/WindowConfig.hpp
)

set(src_files_vulkan_raytracing
	Vulkan/RayTracing/AccelerationStructure.cpp
	Vulkan/RayTracing/AccelerationStructure.hpp
	Vulkan/RayTracing/Application.cpp
	Vulkan/RayTracing/Application.hpp
	Vulkan/RayTracing/BottomLevelAccelerationStructure.cpp
	Vulkan/RayTracing/BottomLevelAccelerationStructure.hpp
	Vulkan/RayTracing/BottomLevelGeometry.cpp
	Vulkan/RayTracing/BottomLevelGeometry.hpp
	Vulkan/RayTracing/DeviceProcedures.cpp
	Vulkan/RayTracing/DeviceProcedures.hpp
	Vulkan/RayTracing/RayTracingPipeline.cpp
	Vulkan/RayTracing/RayTracingPipeline.hpp
	Vulkan/RayTracing/RayTracingProperties.cpp
	Vulkan/RayTracing/RayTracingProperties.hpp
	Vulkan/RayTracing/ShaderBindingTable.cpp
	Vulkan/RayTracing/ShaderBindingTable.hpp
	Vulkan/RayTracing/TopLevelAccelerationStructure.cpp
	Vulkan/RayTracing/TopLevelAccelerationStructure.hpp
)

set(src_files_ui
	UI/AUIScreen.cpp
	UI/AUIScreen.h
	UI/ConsoleScreen.cpp
	UI/ConsoleScreen.h
	UI/ExportAnimationScreen.cpp
	UI/ExportAnimationScreen.h
	UI/HierarchyScreen.cpp
	UI/HierarchyScreen.h
	UI/InspectorScreen.cpp
	UI/InspectorScreen.h
	UI/MenuScreen.cpp
	UI/MenuScreen.h
	UI/ProfilerScreen.cpp
	UI/ProfilerScreen.h
	UI/PlaybackScreen.cpp
	UI/PlaybackScreen.h
	UI/MaterialEditorScreen.cpp
	UI/MaterialEditorScreen.h
	UI/AssetExplorerScreen.cpp
	UI/AssetExplorerScreen.h
	UI/SettingsScreen.cpp
	UI/SettingsScreen.h
	UI/UIManager.cpp
	UI/UIManager.h
	UI/EditorTheme.hpp
	UI/EditorTheme.cpp
	UI/IconsMaterialDesign.h
	UI/UIConfig.hpp
)

set(src_files_from_gdgrap2
	From-GDGRAP2/Debug.cpp
	From-GDGRAP2/Debug.h
	From-GDGRAP2/IETSemaphore.cpp
	From-GDGRAP2/IETSemaphore.h
	From-GDGRAP2/IETThread.cpp
	From-GDGRAP2/IETThread.h
	From-GDGRAP2/EventBroadcaster.cpp
	From-GDGRAP2/EventBroadcaster.h
	From-GDGRAP2/MathUtils.cpp
	From-GDGRAP2/MathUtils.h
	From-GDGRAP2/VectorUtils.cpp
	From-GDGRAP2/VectorUtils.h
	From-GDGRAP2/GlobalConfig.cpp
	From-GDGRAP2/GlobalConfig.h
	From-GDGRAP2/ObjectGroup.cpp
	From-GDGRAP2/ObjectGroup.h
	From-GDGRAP2/GameObject.cpp
	From-GDGRAP2/GameObject.h
	From-GDGRAP2/ModelManager.cpp
	From-GDGRAP2/ModelManager.h
	From-GDGRAP2/MaterialLibrary.cpp
	From-GDGRAP2/MaterialLibrary.h
	From-GDGRAP2/EventNames.h
	From-GDGRAP2/RTConfig.h
	From-GDGRAP2/TextureLibrary.cpp
	From-GDGRAP2/TextureLibrary.h
	From-GDGRAP2/TransformHistory.cpp
	From-GDGRAP2/TransformHistory.h
)


set(src_files_statemanagement
	StateManagement/CommandManager.hpp
	StateManagement/CommandManager.cpp
	StateManagement/ICommand.hpp
	StateManagement/ConcreteCommands/InspectorCommands.hpp
	StateManagement/ConcreteCommands/InspectorCommands.cpp
	StateManagement/ConcreteCommands/MaterialCommands.hpp
	StateManagement/ConcreteCommands/MaterialCommands.cpp
	StateManagement/ConcreteCommands/HierarchyCommands.hpp
	StateManagement/ConcreteCommands/HierarchyCommands.cpp
	StateManagement/ConcreteCommands/MenuBarCommands.hpp
	StateManagement/ConcreteCommands/MenuBarCommands.cpp
	StateManagement/ConcreteCommands/GUICommands.hpp
	StateManagement/ConcreteCommands/GUICommands.cpp
)

set(src_files_hotkeysytem
	HotkeySystem/HotkeySystem.hpp
	HotkeySystem/HotkeySystem.cpp
	HotkeySystem/HotkeyListener.hpp
	HotkeySystem/GLFWInputAdapter.hpp
	HotkeySystem/GLFWInputAdapter.cpp
	HotkeySystem/KeyCodes.hpp
	HotkeySystem/ActionMap.hpp
	HotkeySystem/InputEvent.hpp

)

set(src_files_raypicker
	RayPicker/RayPicker.hpp
	RayPicker/RayPicker.cpp
	RayPicker/RayPickerPipeline.hpp
	RayPicker/RayPickerPipeline.cpp
	RayPicker/RayPickerPipelineLayout.hpp
	RayPicker/RayPickerPipelineLayout.cpp
	RayPicker/RayPickerUBO.hpp
	RayPicker/RayPickerUBO.cpp
)

set(src_files
	main.cpp
	Options.cpp
	Options.hpp
	RayTracer.cpp
	RayTracer.hpp
	SceneList.cpp
	SceneList.hpp
	UserSettings.hpp
)

set(src_files_imgui
	Dependencies/include/imconfig.h
	Dependencies/include/imgui.cpp
	Dependencies/include/imgui.h
	Dependencies/include/imgui_draw.cpp
	Dependencies/include/imgui_freetype.cpp
	Dependencies/include/imgui_freetype.h
	Dependencies/include/imgui_impl_glfw.cpp
	Dependencies/include/imgui_impl_glfw.h
	Dependencies/include/imgui_impl_vulkan.cpp
	Dependencies/include/imgui_impl_vulkan.h
	Dependencies/include/imgui_internal.h
	Dependencies/include/imgui_widgets.cpp
	Dependencies/include/imgui_stdlib.h
	Dependencies/include/imgui_stdlib.cpp
	Dependencies/include/imgui_tables.cpp
	Dependencies/include/imstb_rectpack.h
	Dependencies/include/imstb_textedit.h
	Dependencies/include/imstb_truetype.h
	Dependencies/include/ImGuizmo.cpp
	Dependencies/include/ImGuizmo.h
)


source_group("Assets" FILES ${src_files_assets})
source_group("OBB" FILES ${src_files_obb})
source_group("CameraSystem" FILES ${src_files_camerasystem})
source_group("AnimationSystem" FILES ${src_files_animationsystem})
source_group("LightSystem" FILES ${src_files_lightsystem})
source_group("HotkeySystem" FILES ${src_files_hotkeysytem})
source_group("Profiler" FILES ${src_files_profiler})
source_group("Scene IO" FILES ${src_files_sceneio})
source_group("RayVisualization" FILES ${src_files_ray_visualization})
source_group("StateManagement" FILES ${src_files_statemanagement})
source_group("Utilities" FILES ${src_files_utilities})
source_group("UI" FILES ${src_files_ui})
source_group("From-GDGRAP2" FILES ${src_files_from_gdgrap2})
source_group("Vulkan" FILES ${src_files_vulkan})
source_group("Vulkan.RayTracing" FILES ${src_files_vulkan_raytracing})
source_group("RayPicker" FILES ${src_files_raypicker})
source_group("Main" FILES ${src_files})
source_group("ImGui" FILES ${src_files_imgui})

add_executable(${exe_name} 
	${src_files_assets}
	${src_files_obb}

	${src_files_camerasystem}
	${src_files_animationsystem}
	${src_files_lightsystem}
	${stc_files_hotkeysystem}
	${src_files_profiler}
	${src_files_sceneio}
	${src_files_hotkeysytem}
	${src_files_ray_visualization} 
	${src_files_statemanagement}
	${src_files_utilities}
	${src_files_ui}
	${src_files_from_gdgrap2}
	${src_files_vulkan} 
	${src_files_vulkan_raytracing} 
	${src_files_raypicker}
	${src_files}
	${src_files_imgui}
 "Engine/Scene/SceneIO.cpp" "UI/ButtonTexture.cpp" "Utilities/FileExplorer/FileTreeNode.h" "Utilities/FileExplorer/FileTree.h" "Utilities/FileExplorer/FileIcon.h" "Utilities/FileExplorer/FileTree.cpp" "Utilities/FileExplorer/FileTreeNode.cpp" "UI/ProjectScreen.h" "UI/ProjectScreen.cpp" "Utilities/FileExplorer/FileIcon.cpp" "Utilities/FileExplorer/FileExplorerConstants.h" "UI/FileExplorer/FileListView.h" "UI/FileExplorer/FileIconView.h" "UI/FileExplorer/FileListView.cpp" "Utilities/FileExplorer/FileExplorerUtils.h" "Utilities/FileExplorer/FileExplorerUtils.cpp" "UI/FileExplorer/FileIconView.cpp"  "Utilities/DragAndDrop/DragAndDropUtils.h" "Utilities/DragAndDrop/DragAndDropUtils.cpp" "Utilities/DragAndDrop/DragAndDropConstants.h")

if (UNIX)
	# GCC8 needs an extra lib for <filesystem>.
	# This is not needed with GCC9 or higher.
	set(extra_libs -lstdc++fs ${Backtrace_LIBRARIES})
endif()

add_dependencies(${exe_name} Assets)
set_target_properties(${exe_name} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_include_directories(
	${exe_name} 
	PRIVATE . 
	${assimp_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} 
	${glfw3_INCLUDE_DIRS} 
	${glm_INCLUDE_DIRS} 
	${STB_INCLUDE_DIRS} 
	${Vulkan_INCLUDE_DIRS}
	${FFMPEG_INCLUDE_DIRS}
	Dependencies/include
)


target_link_directories(${exe_name} PRIVATE Assimp/lib ${Vulkan_LIBRARY})
target_link_libraries(${exe_name} PRIVATE assimp::assimp ${Boost_LIBRARIES} freetype glfw glm::glm ${Vulkan_LIBRARIES} 
	##FFMPEG_LIBRARIES has a lot of dependencies that CMake doesn't automatically link, so we have to add them manually -..-
	${FFMPEG_LIBRARIES}
    ncrypt.lib 
    crypt32.lib 
    setupapi.lib 
    winmm.lib 
    imm32.lib 
    version.lib 
    mfplat.lib 
    strmiids.lib 
    mfuuid.lib 
    Bcrypt.lib 
${extra_libs})

#if(${WIN32})
#add_custom_command(TARGET ${exe_name} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        "${PROJECT_SOURCE_DIR}/src/Assimp/dll/assimp-vc143-mt.dll"
#        $<TARGET_FILE_DIR:${exe_name}>)
#endif()

add_compile_definitions(TRACY_ENABLE)