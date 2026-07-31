# CMakeList.txt : CMake project for AnitoTracer_Rebuild, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AnitoTracer_Rebuild" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

#=================#DEPENDENCIES#=================#
include(deps.cmake)

if(MSVC)
    # Compiler flags for modern C++ and warnings
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor /W4 /WX- /MP)
    
    # Set runtime library to Multi-threaded DLL for both Debug and Release
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Also set via compiler flags for compatibility with older CMake behavior
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " /MDd")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /MD")
    
    # Enable Hot Reload for MSVC compilers if supported
    if(POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT 
            "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()
    
    # Disable specific warnings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# GCC/Clang specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(assimp CONFIG REQUIRED)

add_compile_definitions(PLATFORM_WIN32=1)

set(anito_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE INTERNAL "Src directory of AnitoEngine" FORCE)

add_subdirectory(src/Objects)
add_subdirectory(src/Asset)
add_subdirectory(src/ObjectSystems/Serialization)

add_subdirectory(src/UI)
add_subdirectory(src/Rendering)

add_subdirectory(src/Tools)

# Add source to this project's executable.
add_executable (AnitoTracer_Rebuild WIN32 "AnitoTracer_Rebuild.cpp" "AnitoTracer_Rebuild.h")
set_property(TARGET AnitoTracer_Rebuild PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AnitoTracer_Rebuild PROPERTY CXX_STANDARD 20)
endif()

if(WIN32 AND MSVC)
  set_target_properties(AnitoTracer_Rebuild PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /IGNORE:4099 /FORCE:MULTIPLE"
  )
endif()

target_include_directories(AnitoTracer_Rebuild
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_compile_definitions(AnitoTracer_Rebuild PRIVATE UNICODE)

target_link_libraries(AnitoTracer_Rebuild PRIVATE 
				glm::glm
				Vulkan::Vulkan
                assimp::assimp
				Diligent-GraphicsEngineVk-shared
				Diligent-Imgui

                $<LINK_LIBRARY:WHOLE_ARCHIVE,anito::objects>
                anito::rendering
                anito::ui
                anito::asset
)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
set(SHADER_DST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders")

file(GLOB_RECURSE SHADER_FILES 
     CONFIGURE_DEPENDS 
     "${SHADER_SRC_DIR}/*"
)

add_custom_target(CopyShadersTarget
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_SRC_DIR}" "${SHADER_DST_DIR}"
    DEPENDS ${SHADER_FILES}
    COMMENT "Syncing shaders: checking for edits, additions, or deletions..."
)

add_dependencies(AnitoTracer_Rebuild CopyShadersTarget)

# Copy Assets folder to build output directory
add_custom_command(TARGET AnitoTracer_Rebuild POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/Assets
	${CMAKE_CURRENT_BINARY_DIR}/Assets
	COMMENT "Copying Assets folder to build directory"
)



copy_required_dlls(AnitoTracer_Rebuild)
#add_custom_command(
#    TARGET AnitoTracer_Rebuild POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#    "${SHADER_SRC_DIR}"
#    "${SHADER_DST_DIR}"
#    COMMENT "Copying Shaders to build directory..."
#)

