#SETUP
set(CUR_TARGET_SUBLIBNAME event)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	EventPipeline.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::event
	anito::objects
)

target_compile_definitions(${CUR_TARGET_NAME} PUBLIC ANITO_EVENT_INCLUDES="EventIncludes.hpp")