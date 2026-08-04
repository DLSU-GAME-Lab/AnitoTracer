#SETUP
set(CUR_TARGET_SUBLIBNAME serialization)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Meta/glm_meta.hpp
	Initializer/ObjectInitializer.hpp
	Initializer/ComponentInitializer.hpp
	Initializer/ComponentInitializer.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::serialization
	anito::objects
)

#ADDITIONAL STUFF
target_compile_definitions(${CUR_TARGET_NAME} PUBLIC ANITO_SERIALIZATION_INCLUDES="SerializationIncludes.hpp")