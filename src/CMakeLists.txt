set(exe_name ${MAIN_PROJECT})

set(src_files_assets
	Assets/CornellBox.cpp
	Assets/CornellBox.hpp
	Assets/Material.hpp
	Assets/Model.cpp
	Assets/Model.hpp
	Assets/Procedural.hpp
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
)

set(src_files_obb
	OBB/BoundingBox.cpp
	OBB/BoundingBox.hpp
	OBB/Ray.cpp
	OBB/Ray.hpp
)

set(src_files_assimp
	Assimp/include/assimp/aabb.h
	Assimp/include/assimp/ai_assert.h
	Assimp/include/assimp/anim.h
	Assimp/include/assimp/AssertHandler.h
	Assimp/include/assimp/Base64.hpp
	Assimp/include/assimp/BaseImporter.h
	Assimp/include/assimp/Bitmap.h
	Assimp/include/assimp/BlobIOSystem.h
	Assimp/include/assimp/ByteSwapper.h
	Assimp/include/assimp/camera.h
	Assimp/include/assimp/cexport.h
	Assimp/include/assimp/cfileio.h
	Assimp/include/assimp/cimport.h
	Assimp/include/assimp/ColladaMetaData.h
	Assimp/include/assimp/color4.h
	Assimp/include/assimp/color4.inl
	Assimp/include/assimp/commonMetaData.h
	Assimp/include/assimp/config.h
	Assimp/include/assimp/config.h.in
	Assimp/include/assimp/CreateAnimMesh.h
	Assimp/include/assimp/DefaultIOStream.h
	Assimp/include/assimp/DefaultIOSystem.h
	Assimp/include/assimp/DefaultLogger.hpp
	Assimp/include/assimp/defs.h
	Assimp/include/assimp/Exceptional.h
	Assimp/include/assimp/Exporter.hpp
	Assimp/include/assimp/fast_atof.h
	Assimp/include/assimp/GenericProperty.h
	Assimp/include/assimp/GltfMaterial.h
	Assimp/include/assimp/Hash.h
	Assimp/include/assimp/Importer.hpp
	Assimp/include/assimp/importerdesc.h
	Assimp/include/assimp/IOStream.hpp
	Assimp/include/assimp/IOStreamBuffer.h
	Assimp/include/assimp/IOSystem.hpp
	Assimp/include/assimp/light.h
	Assimp/include/assimp/LineSplitter.h
	Assimp/include/assimp/LogAux.h
	Assimp/include/assimp/Logger.hpp
	Assimp/include/assimp/LogStream.hpp
	Assimp/include/assimp/material.h
	Assimp/include/assimp/material.inl
	Assimp/include/assimp/MathFunctions.h
	Assimp/include/assimp/matrix3x3.h
	Assimp/include/assimp/matrix3x3.inl
	Assimp/include/assimp/matrix4x4.h
	Assimp/include/assimp/matrix4x4.inl
	Assimp/include/assimp/MemoryIOWrapper.h
	Assimp/include/assimp/mesh.h
	Assimp/include/assimp/metadata.h
	Assimp/include/assimp/module.modulemap
	Assimp/include/assimp/NullLogger.hpp
	Assimp/include/assimp/ObjMaterial.h
	Assimp/include/assimp/ParsingUtils.h
	Assimp/include/assimp/pbrmaterial.h
	Assimp/include/assimp/postprocess.h
	Assimp/include/assimp/Profiler.h
	Assimp/include/assimp/ProgressHandler.hpp
	Assimp/include/assimp/qnan.h
	Assimp/include/assimp/quaternion.h
	Assimp/include/assimp/quaternion.inl
	Assimp/include/assimp/RemoveComments.h
	Assimp/include/assimp/revision.h
	Assimp/include/assimp/revision.h.in
	Assimp/include/assimp/scene.h
	Assimp/include/assimp/SceneCombiner.h
	Assimp/include/assimp/SGSpatialSort.h
	Assimp/include/assimp/SkeletonMeshBuilder.h
	Assimp/include/assimp/SmallVector.h
	Assimp/include/assimp/SmoothingGroups.h
	Assimp/include/assimp/SmoothingGroups.inl
	Assimp/include/assimp/SpatialSort.h
	Assimp/include/assimp/StandardShapes.h
	Assimp/include/assimp/StreamReader.h
	Assimp/include/assimp/StreamWriter.h
	Assimp/include/assimp/StringComparison.h
	Assimp/include/assimp/StringUtils.h
	Assimp/include/assimp/Subdivision.h
	Assimp/include/assimp/texture.h
	Assimp/include/assimp/TinyFormatter.h
	Assimp/include/assimp/types.h
	Assimp/include/assimp/vector2.h
	Assimp/include/assimp/vector2.inl
	Assimp/include/assimp/vector3.h
	Assimp/include/assimp/vector3.inl
	Assimp/include/assimp/version.h
	Assimp/include/assimp/Vertex.h
	Assimp/include/assimp/XmlParser.h
	Assimp/include/assimp/XMLTools.h
)

set(src_files_camerasystem
	Engine/CameraSystem/Camera.cpp
	Engine/CameraSystem/Camera.h
	Engine/CameraSystem/CameraManager.cpp
	Engine/CameraSystem/CameraManager.h
	Engine/CameraSystem/SceneCamera.cpp
	Engine/CameraSystem/SceneCamera.h
)

set(src_files_lightsystem
	Engine/LightSystem/Light.cpp
	Engine/LightSystem/Light.h
)

set(src_files_profiler
	Engine/Profiler/Profiler.cpp
	Engine/Profiler/Profiler.h
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
)

set(src_files_from_gdgrap2
	From-GDGRAP2/Debug.cpp
	From-GDGRAP2/Debug.h
	From-GDGRAP2/IETSemaphore.cpp
	From-GDGRAP2/IETSemaphore.h
	From-GDGRAP2/IETThread.cpp
	From-GDGRAP2/IETThread.h
	From-GDGRAP2/RTRules.cpp
	From-GDGRAP2/RTRules.h
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
)

set(src_files
	main.cpp
	ModelViewController.cpp
	ModelViewController.hpp
	Options.cpp
	Options.hpp
	RayTracer.cpp
	RayTracer.hpp
	SceneList.cpp
	SceneList.hpp
	UserSettings.hpp
)

source_group("Assets" FILES ${src_files_assets})
source_group("OBB" FILES ${src_files_obb})
source_group("Assimp" FILES ${src_files_assimp})
source_group("CameraSystem" FILES ${src_files_camerasystem})
source_group("LightSystem" FILES ${src_files_lightsystem})
source_group("Profiler" FILES ${src_files_profiler})
source_group("ImGui" FILES ${src_files_imgui})
source_group("Utilities" FILES ${src_files_utilities})
source_group("UI" FILES ${src_files_ui})
source_group("From-GDGRAP2" FILES ${src_files_from_gdgrap2})
source_group("Vulkan" FILES ${src_files_vulkan})
source_group("Vulkan.RayTracing" FILES ${src_files_vulkan_raytracing})
source_group("Main" FILES ${src_files})

add_executable(${exe_name} 
	${src_files_assets}
	${src_files_obb}
	${src_files_assimp}
	${src_files_camerasystem} 
	${src_files_lightsystem} 
	${src_files_profiler}
	${src_files_imgui} 
	${src_files_utilities}
	${src_files_ui}
	${src_files_from_gdgrap2}
	${src_files_vulkan} 
	${src_files_vulkan_raytracing} 
	${src_files}
)

if (UNIX)
	# GCC8 needs an extra lib for <filesystem>.
	# This is not needed with GCC9 or higher.
	set(extra_libs -lstdc++fs ${Backtrace_LIBRARIES})
endif()

add_dependencies(${exe_name} Assets)
set_target_properties(${exe_name} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_include_directories(
	${exe_name} 
	PRIVATE . Assimp/include 
	${Boost_INCLUDE_DIRS} 
	${glfw3_INCLUDE_DIRS} 
	${glm_INCLUDE_DIRS} 
	${STB_INCLUDE_DIRS} 
	${Vulkan_INCLUDE_DIRS})
target_link_directories(${exe_name} PRIVATE Assimp/lib ${Vulkan_LIBRARY})
target_link_libraries(${exe_name} PRIVATE assimp-vc143-mt ${Boost_LIBRARIES} freetype glfw glm::glm imgui::imgui tinyobjloader::tinyobjloader ${Vulkan_LIBRARIES} ${extra_libs})

if(${WIN32})
add_custom_command(TARGET ${exe_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/src/Assimp/dll/assimp-vc143-mt.dll"
        $<TARGET_FILE_DIR:${exe_name}>)
endif()

add_compile_definitions(TRACY_ENABLE)