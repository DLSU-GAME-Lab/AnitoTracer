cmake_minimum_required(VERSION 3.10)

project(AnitoTracer)

set(CMAKE_DEBUG_POSTFIX d)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_BINARY_DIR}/lib)
endforeach()

set (CMAKE_CXX_STANDARD 20)

if (WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif ()

if (MSVC)
	foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		if (${flag_var} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif()
	endforeach()
	
	add_compile_options("/MP")
	add_compile_options("/utf-8")
endif()

if (UNIX)
	
	add_definitions(-DUNIX)
	add_compile_options("-Wall")
	add_compile_options("-fvisibility=hidden")
	add_compile_options("-finput-charset=UTF-8 -fexec-charset=UTF-8")
	
	find_package(Backtrace REQUIRED)

	string(COMPARE EQUAL ${Backtrace_HEADER} "backtrace.h" BacktraceRequired)

	if (BacktraceRequired)
		add_compile_definitions(BOOST_STACKTRACE_USE_BACKTRACE)
	endif (BacktraceRequired)

endif ()

SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "build/vcpkg.windows/installed/x64-windows-static/include")
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "build/vcpkg.windows/installed/x64-windows-static/lib")
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "build/vcpkg.windows/installed/x64-windows-static/share")
# Stb is weird idk why!!!
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/vcpkg.windows/installed/x64-windows-static/share/stb")

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

find_package(Boost REQUIRED COMPONENTS exception program_options) 
find_package(freetype CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Assimp REQUIRED)
find_package(Vulkan REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(TBB CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)

# add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

IF (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
    message(STATUS ${Vulkan_LIBRARY})
ENDIF()

find_program(Vulkan_GLSLANG_VALIDATOR 
	NAMES glslangValidator 
	HINTS ENV VULKAN_SDK 
	PATH_SUFFIXES bin)
	
if (NOT Vulkan_GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found!")
endif()

set(MAIN_PROJECT "AnitoTracer")
add_subdirectory(assets)

# OIDN
set(OIDN_APPS OFF CACHE BOOL "" FORCE)
set(OIDN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OIDN_TESTS OFF CACHE BOOL "" FORCE)

if (WIN32)
  set(ISPC_EXECUTABLE "${CMAKE_SOURCE_DIR}/Dependencies/ispc/bin/ispc.exe"
      CACHE FILEPATH "" FORCE)
endif()

add_subdirectory(Dependencies/oidn)

add_definitions(-DSOLUTION_DIR="${CMAKE_SOURCE_DIR}")
add_subdirectory(src)

set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT ${MAIN_PROJECT})
