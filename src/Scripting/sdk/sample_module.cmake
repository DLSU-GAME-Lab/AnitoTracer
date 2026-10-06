# Stand-in for the transpiled script module until the transpiler exists.
add_library(anito_script_sample MODULE "${CMAKE_CURRENT_LIST_DIR}/samples/SpinAndBob.gen.cpp")
target_include_directories(anito_script_sample PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
set_target_properties(anito_script_sample PROPERTIES
	OUTPUT_NAME Scripts
	PREFIX ""
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ScriptModules"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ScriptModules"
)
# Both sides use the shared CRT so std::string fields can cross the boundary.
# The PDB is looked up beside each loaded copy, so the build output is never locked.
target_link_options(anito_script_sample PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/PDBALTPATH:Scripts.pdb>")
target_compile_definitions(${CUR_TARGET_NAME} PRIVATE "ANITO_SCRIPT_MODULE_PATH=\"$<TARGET_FILE:anito_script_sample>\"")
