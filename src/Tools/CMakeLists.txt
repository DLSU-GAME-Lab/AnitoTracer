
add_subdirectory(Serialization)
add_subdirectory(UI/FileDialogue)

add_library(gbe_tools INTERFACE)
add_library(gbe::tools ALIAS gbe_tools)

target_link_libraries(gbe_tools
	INTERFACE
	gbe::filedialogue
	gbe::serialization
)

target_include_directories(gbe_tools INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})