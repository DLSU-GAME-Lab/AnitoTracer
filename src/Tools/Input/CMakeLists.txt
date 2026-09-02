#SETUP
set(CUR_TARGET_SUBLIBNAME input)
set(CUR_TARGET_NAME gbe_${CUR_TARGET_SUBLIBNAME})

set(${CUR_TARGET_NAME}_src
	"InputSystem.cpp"
)
add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(gbe::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#PUBLIC
target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	gbe::event
)