set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

#========Diligent========#
FetchContent_Declare(
    DiligentCore
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore"
    GIT_TAG 7cd667b06703516ac210779cd1919bd174afd0b9
    GIT_SHALLOW OFF
)
FetchContent_Declare(
    DiligentTools
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentTools.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentTools"
    GIT_TAG a65fe94e0f12e680c81ea86fe2ebe0de6b867b4b
    GIT_SHALLOW OFF
)
FetchContent_Declare(
    DiligentFX
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentFX.git
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentFX"
    GIT_TAG eb616a8e30efa5193baba71ff1edae85bc6230a1
    GIT_SHALLOW OFF
)
# FORCE DILIGENT ENGINE TO USE DYNAMIC CRT
set(DILIGENT_MSVC_CRT_LINKAGE "Dynamic" CACHE STRING "Force Diligent to use dynamic CRT" FORCE)
FetchContent_MakeAvailable(DiligentCore DiligentTools DiligentFX)

#========glaze========#
FetchContent_Declare(glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.6.9
)
FetchContent_MakeAvailable(glaze)
add_compile_definitions(NOMINMAX)