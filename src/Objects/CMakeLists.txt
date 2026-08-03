#SETUP
set(CUR_TARGET_SUBLIBNAME objects)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Components/ComponentBase.cpp
	Components/ComponentBase.hpp
	Components/Transform.cpp
	Components/Transform.hpp
	Components/Camera.cpp
	Components/Camera.hpp
	Components/ModelComponent.cpp
	Components/ModelComponent.hpp

	Components/Lights/LightBase.cpp
	Components/Lights/LightBase.hpp
	Components/Lights/DirectionLight.cpp
	Components/Lights/DirectionLight.hpp

	HierarchyObject.hpp
	HierarchyObject.cpp

	HierarchyManager.hpp
	HierarchyManager.cpp

	ObjectFactory.hpp
	ObjectFactory.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	glm::glm
	anito::serialization
	gbe::tools
)

#PRIVATE
target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	anito::rendering
)