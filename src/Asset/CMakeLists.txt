set(ASSET_SRC
)

add_library(Asset STATIC ${ASSET_SRC})

target_link_libraries(Asset
			PRIVATE
			anito::serialization
			gbe::tools
)

target_include_directories(Asset PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})