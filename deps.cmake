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

