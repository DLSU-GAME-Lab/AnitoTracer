set(GBE_SERIALIZATION_SRC
	ISerializable.cpp
	AutoSerializer.cpp
)

set(CUR_TARGET_NAME gbe_serialization)

add_library(${CUR_TARGET_NAME} STATIC ${GBE_SERIALIZATION_SRC})

target_link_libraries(${CUR_TARGET_NAME}
			PUBLIC
	glaze::glaze
)

target_link_libraries(${CUR_TARGET_NAME}
			PRIVATE
	glm::glm
)

target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(gbe::serialization ALIAS ${CUR_TARGET_NAME})