# CMakeList.txt : CMake project for AnitoTracerRebuild
cmake_minimum_required (VERSION 3.14)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("AnitoTracerRebuild")

# Set global C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Redirect all FetchContent downloads and builds to build/thirdparty
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/thirdparty" CACHE PATH "Directory for FetchContent dependencies")

# Include FetchContent module
include(FetchContent)

# Turn off examples before making the targets available
set(BGFX_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BGFX_BUILD_TOOLS ON CACHE INTERNAL "")

# Declare the bgfx.cmake repository
FetchContent_Declare(
  bgfx-cmake
  GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake
  GIT_TAG        v1.150.9365-558 
)

# Populate the content manually to resolve folder nesting pathways before compiling
FetchContent_MakeAvailable(bgfx-cmake)

# CRITICAL: Tell bgfx.cmake where the fetched nested directories live
set(BX_DIR "${bgfx-cmake_SOURCE_DIR}/bx" CACHE PATH "" FORCE)
set(BIMG_DIR "${bgfx-cmake_SOURCE_DIR}/bimg" CACHE PATH "" FORCE)
set(BGFX_DIR "${bgfx-cmake_SOURCE_DIR}/bgfx" CACHE PATH "" FORCE)

# ==========================================
# TARGET CONFIGURATION
# ==========================================
# Add source to this project's executable.
add_executable (AnitoTracerRebuild "AnitoTracerRebuild.cpp" "AnitoTracerRebuild.h")

# Link the three mandatory dependencies exposed by bgfx.cmake
target_link_libraries(AnitoTracerRebuild PRIVATE bgfx bimg bx)