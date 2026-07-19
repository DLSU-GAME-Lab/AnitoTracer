# CMakeList.txt : CMake project for AnitoTracer_Rebuild, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AnitoTracer_Rebuild")

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

FetchContent_Declare(
    DiligentCore
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore"
    GIT_TAG master
    GIT_SHALLOW ON
)
FetchContent_Declare(
    DiligentTools
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentTools.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentTools"
    GIT_TAG master
    GIT_SHALLOW ON
)
FetchContent_Declare(
    DiligentFX
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentFX.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentFX"
    GIT_TAG master
    GIT_SHALLOW ON
)

# FORCE DILIGENT ENGINE TO USE DYNAMIC CRT
set(DILIGENT_MSVC_CRT_LINKAGE "Dynamic" CACHE STRING "Force Diligent to use dynamic CRT" FORCE)
FetchContent_MakeAvailable(DiligentCore DiligentTools DiligentFX)

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

add_compile_definitions(PLATFORM_WIN32=1)


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
				Diligent-GraphicsEngineVk-shared
				Diligent-Imgui
)

copy_required_dlls(AnitoTracer_Rebuild)
