set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

#========glaze========#
FetchContent_Declare(glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v2.6.9
)
FetchContent_MakeAvailable(glaze)
add_compile_definitions(NOMINMAX)