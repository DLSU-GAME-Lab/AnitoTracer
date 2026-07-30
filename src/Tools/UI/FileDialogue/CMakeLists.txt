set(GBE_SERIALIZATION_SRC
	FileDialogue.cpp
)

set(CUR_TARGET_NAME gbe_filedialogue)

add_library(${CUR_TARGET_NAME} STATIC ${GBE_SERIALIZATION_SRC})
add_library(gbe::filedialogue ALIAS ${CUR_TARGET_NAME})

target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
