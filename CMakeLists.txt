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

if(MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
endif()

find_package(bgfx CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (AnitoTracer_Rebuild "AnitoTracer_Rebuild.cpp" "AnitoTracer_Rebuild.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET AnitoTracer_Rebuild PROPERTY CXX_STANDARD 20)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(AnitoTracer_Rebuild PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

target_link_libraries(AnitoTracer_Rebuild PRIVATE 
						bgfx::bgfx
						imgui::imgui 
						glfw 
						glm::glm
)

add_subdirectory(shaders)

add_dependencies(AnitoTracer_Rebuild CopyShaders)
