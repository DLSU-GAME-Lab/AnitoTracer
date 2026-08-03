#SETUP
set(CUR_TARGET_SUBLIBNAME serialization)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	Initializer/ObjectInitializer.hpp
	Initializer/ComponentInitializer.hpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::serialization
)

#PRIVATE
target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	anito::objects
)

#ADDITIONAL STUFF
target_compile_definitions(${CUR_TARGET_NAME} PUBLIC SERIALIZATION_INCLUDES="SerializationIncludes.hpp")