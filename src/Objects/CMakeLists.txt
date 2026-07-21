set(OBJ_SRC
	Models/ModelStructs.hpp
	Models/ModelManager.hpp
	Models/ModelManager.cpp

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

add_library(Objects STATIC ${OBJ_SRC})

target_link_libraries(Objects
			PUBLIC
			glm::glm
			assimp::assimp
			Diligent-Common
			Diligent-GraphicsEngineVk-shared
			Diligent-Imgui
			Diligent-TextureLoader
)

target_include_directories(Objects
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_include_directories(Objects PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})