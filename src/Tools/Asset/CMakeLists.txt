#SETUP
set(CUR_TARGET_SUBLIBNAME asset)
set(CUR_TARGET_NAME gbe_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	"IAsset.hpp"
 "IAsset.cpp"
 "AssetLoading/AssetLoader.cpp"
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(gbe::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::serialization
)

if(BUILD_TESTING)
	add_executable(asset_pipeline_tests tests/AssetPipelineTests.cpp)
	target_link_libraries(asset_pipeline_tests PRIVATE gbe::asset)
	add_test(NAME asset_pipeline_tests COMMAND asset_pipeline_tests)
endif()