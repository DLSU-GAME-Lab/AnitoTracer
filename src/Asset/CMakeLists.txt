#SETUP
set(CUR_TARGET_SUBLIBNAME asset)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	AssetPipeline.cpp
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PRIVATE
target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	anito::serialization
	gbe::tools
)

#ADDITIONAL STUFF