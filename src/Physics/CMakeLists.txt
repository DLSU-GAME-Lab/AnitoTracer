set(CUR_TARGET_SUBLIBNAME physics)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	IPhysicsBody.hpp
	IPhysicsEngine.hpp
	PhysicsEngine.hpp
	PhysicsEngine.cpp
	JoltPhysicsEngine.hpp
	JoltPhysicsEngine.cpp
	JoltPhysicsBody.hpp
	JoltPhysicsBody.cpp
	JoltContactListener.hpp
	JoltContactListener.cpp)

add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})

target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${jolt_SOURCE_DIR})

find_package(Jolt CONFIG QUIET)
find_package(joltphysics CONFIG QUIET)

if(TARGET Jolt::Jolt)
    set(_JOLT_TARGET Jolt::Jolt)
elseif(TARGET joltphysics::joltphysics)
    set(_JOLT_TARGET joltphysics::joltphysics)
elseif(TARGET joltphysics::Jolt)
    set(_JOLT_TARGET joltphysics::Jolt)
else()
    message(FATAL_ERROR "Jolt not found. Install via vcpkg (e.g. `vcpkg install joltphysics:x64-windows`) and configure CMake with the vcpkg toolchain file.")
endif()

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
	${_JOLT_TARGET}
)