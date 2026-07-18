cmake_minimum_required(VERSION 3.10)
project(AnitoTracer_Shaders)

find_program(BGFX_SHADERC_EXE 
    NAMES shaderc 
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/bgfx" 
    REQUIRED
)

if(MSVC)
    set(VS_PROFILE "spirv") 
    set(FS_PROFILE "spirv")  
    set(PLATFORM "windows")  
else()
    set(VS_PROFILE "spirv")  
    set(FS_PROFILE "spirv")  
    set(PLATFORM "linux")
endif()

set(VARYING_DEF "${CMAKE_CURRENT_SOURCE_DIR}/varying.def.sc")
set(SHADER_OUT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled")

file(GLOB_RECURSE VERTEX_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/v_*.sc")
file(GLOB_RECURSE FRAGMENT_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/f_*.sc")

set(COMPILED_SHADER_BINARIES "")

foreach(SHADER ${VERTEX_SHADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${SHADER}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    get_filename_component(SHADER_NAME "${SHADER}" NAME_WE)
    
    set(OUTPUT_FILE "${SHADER_OUT_BASE_DIR}/${REL_DIR}/${SHADER_NAME}.bin")
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_BASE_DIR}/${REL_DIR}"
        COMMAND ${BGFX_SHADERC_EXE} -f ${SHADER} -o ${OUTPUT_FILE} --type vertex -p ${VS_PROFILE} --platform ${PLATFORM} -i "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/bgfx" --varyingdef ${VARYING_DEF}
        DEPENDS ${SHADER} ${VARYING_DEF}
        COMMENT "Compiling vertex shader: ${REL_PATH}"
    )
    list(APPEND COMPILED_SHADER_BINARIES ${OUTPUT_FILE})
endforeach()

foreach(SHADER ${FRAGMENT_SHADERS})
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${SHADER}")
    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    get_filename_component(SHADER_NAME "${SHADER}" NAME_WE)
    
    set(OUTPUT_FILE "${SHADER_OUT_BASE_DIR}/${REL_DIR}/${SHADER_NAME}.bin")
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_BASE_DIR}/${REL_DIR}"
        COMMAND ${BGFX_SHADERC_EXE} -f ${SHADER} -o ${OUTPUT_FILE} --type fragment -p ${FS_PROFILE} --platform ${PLATFORM} -i "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/bgfx" --varyingdef ${VARYING_DEF}
        DEPENDS ${SHADER} ${VARYING_DEF}
        COMMENT "Compiling fragment shader: ${REL_PATH}"
    )
    list(APPEND COMPILED_SHADER_BINARIES ${OUTPUT_FILE})
endforeach()

add_custom_target(CopyShaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_OUT_BASE_DIR}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    DEPENDS ${COMPILED_SHADER_BINARIES}
    COMMENT "Deploying compiled nested shaders to runtime directory"
)