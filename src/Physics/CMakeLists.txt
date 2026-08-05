set(CUR_TARGET_SUBLIBNAME physics)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	IPhysicsEngine.hpp
	JoltPhysicsEngine.hpp
	JoltPhysicsEngine.cpp
	JoltPhysicsBody.hpp
	JoltPhysicsBody.cpp
)

add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})

target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${jolt_SOURCE_DIR})

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
	Jolt
)