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
	Components/EditorCamera.cpp
	Components/EditorCamera.hpp
	Components/GameCamera.cpp
	Components/GameCamera.hpp
	Components/ModelComponent.cpp
	Components/ModelComponent.hpp
	Components/ITeleportable.hpp

	Components/Lights/LightBase.cpp
	Components/Lights/LightBase.hpp
	Components/Lights/DirectionLight.cpp
	Components/Lights/DirectionLight.hpp
	Components/Lights/PointLight.cpp
	Components/Lights/PointLight.hpp

	Components/Physics/PhysicsBase.hpp
	Components/Physics/PhysicsBase.cpp
	Components/Physics/RigidBody.hpp
	Components/Physics/RigidBody.cpp
	Components/Physics/StaticBody.hpp
	Components/Physics/Collider.hpp
	Components/Physics/Collider.cpp
	Components/Gameplay/PlayerMovementComponent.hpp
	Components/Gameplay/PickupComponent.hpp
	Components/Gameplay/PickupComponent.cpp
	Components/Gameplay/PlacementComponent.hpp
	Components/Gameplay/PlacementComponent.cpp
	Components/Gameplay/PlayerController.hpp
	Components/Gameplay/PlayerController.cpp
	Components/Gameplay/CameraProximityTrigger.cpp
	Components/Gameplay/ParticleSpawnerComponent.cpp
	
	Components/Gameplay/CameraProximityTrigger.cpp
	Components/Gameplay/CameraProximityTrigger.cpp
	
	Components/Global/SceneChanger.cpp
	Components/Global/SceneChanger.hpp

	HierarchyObject.hpp
	HierarchyObject.cpp

	HierarchyManager.hpp
	HierarchyManager.cpp

	HierarchyFeatures/PrefabFeature.hpp
	HierarchyFeatures/PrefabFeature.cpp

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
	anito::asset
	anito::common
	anito::event
)

#PRIVATE
target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	anito::rendering
)

target_compile_definitions(${CUR_TARGET_NAME} PUBLIC ANITO_COMPONENT_INCLUDES="Components/ComponentIncludes.hpp")