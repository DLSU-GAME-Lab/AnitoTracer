
set(CUR_TARGET_SUBLIBNAME common)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
		MathDefs.hpp
		Common.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})

target_include_directories(${CUR_TARGET_NAME} 
	PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
)

