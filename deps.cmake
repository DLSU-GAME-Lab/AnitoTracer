set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

#========Diligent========#
FetchContent_Declare(
    DiligentCore
    SYSTEM
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore"
    GIT_TAG 7cd667b06703516ac210779cd1919bd174afd0b9
    GIT_SHALLOW OFF
    UPDATE_COMMAND "" 
)
FetchContent_Declare(
    DiligentTools
    SYSTEM
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentTools.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentTools"
    GIT_TAG a65fe94e0f12e680c81ea86fe2ebe0de6b867b4b
    GIT_SHALLOW OFF
    GIT_SUBMODULES_RECURSE ON
)
FetchContent_Declare(
    DiligentFX
    SYSTEM
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentFX.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentFX"
    GIT_TAG eb616a8e30efa5193baba71ff1edae85bc6230a1
    GIT_SHALLOW OFF
    UPDATE_COMMAND "" 
)
# FORCE DILIGENT ENGINE TO USE DYNAMIC CRT
set(DILIGENT_MSVC_CRT_LINKAGE "Dynamic" CACHE STRING "Force Diligent to use dynamic CRT" FORCE)

#FetchContent_MakeAvailable(DiligentCore DiligentTools DiligentFX)

FetchContent_MakeAvailable(DiligentCore)

FetchContent_GetProperties(DiligentTools)
#All this just to get the docking functionality of imgui GDI
if(NOT diligenttools_POPULATED)
    FetchContent_Populate(DiligentTools)
    
    file(REMOVE_RECURSE "${diligenttools_SOURCE_DIR}/ThirdParty/imgui")
    
    execute_process(
        COMMAND git clone --branch docking --depth 1 https://github.com/ocornut/imgui.git ThirdParty/imgui
        WORKING_DIRECTORY ${diligenttools_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )

    add_subdirectory(${diligenttools_SOURCE_DIR} ${diligenttools_BINARY_DIR})
endif()

FetchContent_MakeAvailable(DiligentFX)

#========glaze========#
FetchContent_Declare(glaze
    SYSTEM
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.6.9
    UPDATE_COMMAND "" 
)
FetchContent_MakeAvailable(glaze)
add_compile_definitions(NOMINMAX)

#========ANTLR4 C++ runtime========#
# Must match the generator jar version downloaded by setup.bat (ANTLR_VERSION).
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(WITH_STATIC_CRT OFF CACHE BOOL "" FORCE) # Match project's dynamic CRT
FetchContent_Declare(antlr4
    SYSTEM
    GIT_REPOSITORY https://github.com/antlr/antlr4.git
    GIT_TAG 4.13.2
    GIT_SHALLOW ON
    SOURCE_SUBDIR runtime/Cpp
    UPDATE_COMMAND ""
)
FetchContent_MakeAvailable(antlr4)
target_include_directories(antlr4_static SYSTEM PUBLIC "${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src")
target_compile_definitions(antlr4_static PUBLIC ANTLR4CPP_STATIC)
# ANTLR 4.13.2 omits <chrono> in ProfilingATNSimulator.cpp, which newer MSVC no longer includes transitively.
if(MSVC)
    target_compile_options(antlr4_static PRIVATE /FIchrono)
endif()

set(ANTLR_JAR "${CMAKE_SOURCE_DIR}/antlr-4.13.2-complete.jar" CACHE FILEPATH "ANTLR generator jar")

