set(SERIALIZATION_SRC
	AnitoInstanceInitializer.cpp
)

add_subdirectory(Internal)

set(CUR_TARGET_NAME anito_serialization)

add_library(${CUR_TARGET_NAME} STATIC ${SERIALIZATION_SRC})
add_library(anito::serialization ALIAS ${CUR_TARGET_NAME})

target_link_libraries(${CUR_TARGET_NAME}
			PRIVATE
	Objects
)

target_compile_definitions(${CUR_TARGET_NAME} PUBLIC ANITOSERIALIZATIONINCLUDE="AnitoSerializationInclude.hpp")

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::serialization
)


target_include_directories(${CUR_TARGET_NAME}
PRIVATE
    ${diligentcore_SOURCE_DIR}
    ${diligenttools_SOURCE_DIR}
    ${diligentfx_SOURCE_DIR}
)

target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})