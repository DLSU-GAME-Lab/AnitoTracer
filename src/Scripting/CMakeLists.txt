#SETUP
set(CUR_TARGET_SUBLIBNAME scripting)
set(CUR_TARGET_NAME anito_${CUR_TARGET_SUBLIBNAME})

#ANTLR GRAMMAR -> C++ (regenerated whenever a .g4 changes)
find_program(ANTLR_JAVA_EXECUTABLE java)
if(NOT ANTLR_JAVA_EXECUTABLE)
	message(FATAL_ERROR "Java is required to generate the script parser. Run setup.bat to install it.")
endif()
if(NOT EXISTS "${ANTLR_JAR}")
	message(FATAL_ERROR "ANTLR jar not found at ${ANTLR_JAR}. Run setup.bat to download it.")
endif()

set(ANTLR_GRAMMAR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/grammar")
set(ANTLR_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ANTLR_GRAMMARS
	"${ANTLR_GRAMMAR_DIR}/AnitoScriptLexer.g4"
	"${ANTLR_GRAMMAR_DIR}/AnitoScriptParser.g4"
)
set(ANTLR_GEN_SRC
	"${ANTLR_GEN_DIR}/AnitoScriptLexer.cpp"
	"${ANTLR_GEN_DIR}/AnitoScriptParser.cpp"
	"${ANTLR_GEN_DIR}/AnitoScriptParserVisitor.cpp"
	"${ANTLR_GEN_DIR}/AnitoScriptParserBaseVisitor.cpp"
)
add_custom_command(
	OUTPUT ${ANTLR_GEN_SRC}
	COMMAND ${CMAKE_COMMAND} -E make_directory "${ANTLR_GEN_DIR}"
	COMMAND ${ANTLR_JAVA_EXECUTABLE} -jar "${ANTLR_JAR}" -Dlanguage=Cpp -visitor -no-listener
		-package anito_script -Xexact-output-dir -o "${ANTLR_GEN_DIR}" ${ANTLR_GRAMMARS}
	DEPENDS ${ANTLR_GRAMMARS} "${ANTLR_JAR}"
	COMMENT "Generating AnitoScript parser with ANTLR"
	VERBATIM
)

# Language core: grammar, parser, analyzer, and transpiler. Engine-facing API metadata is compiled
# into a separate target so neither the analyzer nor the transpiler includes component headers.
set(LANG_TARGET anito_scriptlang)
add_library(${LANG_TARGET} STATIC
	ScriptDescriptor.hpp
	ScriptParser.cpp
	ScriptParser.hpp
	ScriptAnalyzer.cpp
	ScriptAnalyzer.hpp
	ScriptTranspiler.cpp
	ScriptTranspiler.hpp
	${ANTLR_GRAMMARS}
	${ANTLR_GEN_SRC}
)
add_library(anito::scriptlang ALIAS ${LANG_TARGET})
set_source_files_properties(${ANTLR_GRAMMARS} PROPERTIES HEADER_FILE_ONLY TRUE)
target_include_directories(${LANG_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${LANG_TARGET} SYSTEM PRIVATE "${ANTLR_GEN_DIR}")
target_link_libraries(${LANG_TARGET} PUBLIC glm::glm PRIVATE antlr4_static)

# This order is the cross-DLL HostAPI layout. Change it only with an ABI version bump.
# Entries are log, component:<category>, or static:<field>:<type>:<binding>:<header>.
set(SCRIPT_HOST_API_LAYOUT
	log
	component:transform
	component:physics
	component:camera
	component:audio
	static:object:ObjectAPI:object:Object
	static:scene:SceneAPI:scene:Scene
	static:script:ScriptAPI:scripts:Script
	static:input:InputAPI:input:Runtime
	static:time:TimeAPI:time:Runtime
	static:gui:GuiAPI:gui:Runtime
	static:field:FieldAPI:field:Runtime
)

# Component annotations are compiled separately so the language core consumes metadata without
# including engine component headers in ScriptAnalyzer.cpp.
set(SCRIPT_API_CATEGORIES)
foreach(entry IN LISTS SCRIPT_HOST_API_LAYOUT)
	if(entry MATCHES "^component:([a-z][a-z0-9_]*)$")
		set(category "${CMAKE_MATCH_1}")
		if(category IN_LIST SCRIPT_API_CATEGORIES)
			message(FATAL_ERROR "Duplicate script API host category '${category}'.")
		endif()
		list(APPEND SCRIPT_API_CATEGORIES "${category}")
	elseif(entry MATCHES "^component:")
		message(FATAL_ERROR "Invalid script API layout entry '${entry}'.")
	endif()
endforeach()
list(LENGTH SCRIPT_API_CATEGORIES SCRIPT_API_CATEGORY_COUNT)
set(SCRIPT_API_CATEGORY_INITIALIZERS "")
foreach(category IN LISTS SCRIPT_API_CATEGORIES)
	string(APPEND SCRIPT_API_CATEGORY_INITIALIZERS "    \"${category}\",\n")
	if(NOT category MATCHES "^[a-z][a-z0-9_]*$")
		message(FATAL_ERROR "Invalid script API host category '${category}'. Use a lowercase C++ identifier.")
	endif()
endforeach()
configure_file(
	ScriptApiCategories.hpp.in
	"${CMAKE_CURRENT_BINARY_DIR}/ScriptApiCategories.hpp"
	@ONLY
)

add_library(anito_scriptapi STATIC
	ScriptApiMetadata.cpp
	ScriptApiMetadata.hpp
	"${CMAKE_CURRENT_BINARY_DIR}/ScriptApiCategories.hpp"
)
add_library(anito::scriptapi ALIAS anito_scriptapi)
target_include_directories(anito_scriptapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(anito_scriptapi PRIVATE
	"${CMAKE_CURRENT_BINARY_DIR}"
	${CMAKE_SOURCE_DIR}/src/Objects
	${CMAKE_SOURCE_DIR}/src/ObjectSystems/Serialization
	${CMAKE_SOURCE_DIR}/src/ObjectSystems/Event
	${CMAKE_SOURCE_DIR}/src/Asset
	${CMAKE_SOURCE_DIR}/src/Audio
	${CMAKE_SOURCE_DIR}/src/Physics
	${CMAKE_SOURCE_DIR}/src/Tools/Asset
	${CMAKE_SOURCE_DIR}/src/Tools/Event
	${CMAKE_SOURCE_DIR}/src/Tools/Serialization
	${CMAKE_SOURCE_DIR}/src/Tools
	${CMAKE_SOURCE_DIR}/src/UI
)
get_target_property(SCRIPT_API_IMGUI_INCLUDE_DIRS Diligent-Imgui INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(SCRIPT_API_IMGUI_COMPILE_DEFINITIONS Diligent-Imgui INTERFACE_COMPILE_DEFINITIONS)
target_include_directories(anito_scriptapi PRIVATE ${SCRIPT_API_IMGUI_INCLUDE_DIRS})
target_compile_definitions(anito_scriptapi PRIVATE ${SCRIPT_API_IMGUI_COMPILE_DEFINITIONS})
target_link_libraries(anito_scriptapi PUBLIC anito::common PRIVATE glaze::glaze)
target_compile_definitions(anito_scriptapi PRIVATE
	ANITO_COMPONENT_INCLUDES="Components/ComponentIncludes.hpp"
	ANITO_SERIALIZATION_INCLUDES="SerializationIncludes.hpp"
	ANITO_EVENT_INCLUDES="EventIncludes.hpp"
)
target_compile_definitions(anito_scriptapi PRIVATE ANITO_SCRIPT_METADATA_ONLY)
target_link_libraries(${LANG_TARGET} PRIVATE anito::scriptapi)

# Generate component SDK/ABI/bridge surfaces grouped by stable host category.
set(SCRIPT_API_GENERATED_ROOT "${CMAKE_CURRENT_BINARY_DIR}/script_api_generated")
set(SCRIPT_API_GENERATED_SDK "${SCRIPT_API_GENERATED_ROOT}/sdk")
set(SCRIPT_API_GENERATED_COMPONENT_BINDINGS
	"${SCRIPT_API_GENERATED_SDK}/ScriptHostAPI/Bindings/ComponentBindings.hpp"
)
set(SCRIPT_API_COMPONENT_ROOT "${CMAKE_SOURCE_DIR}/src/Objects")
set(SCRIPT_API_GENERATED_OUTPUTS
	"${SCRIPT_API_GENERATED_COMPONENT_BINDINGS}"
	"${SCRIPT_API_GENERATED_SDK}/anito/generated/host/HostIncludes.hpp"
	"${SCRIPT_API_GENERATED_SDK}/anito/generated/host/HostFields.inc"
	"${SCRIPT_API_GENERATED_SDK}/generated/ScriptHostAPI/HostInitializers.inc"
)
set(SCRIPT_API_GENERATED_BRIDGES)
foreach(category IN LISTS SCRIPT_API_CATEGORIES)
	string(SUBSTRING "${category}" 0 1 category_first)
	string(TOUPPER "${category_first}" category_first)
	string(SUBSTRING "${category}" 1 -1 category_rest)
	set(category_name "${category_first}${category_rest}")
	set(category_api "${SCRIPT_API_GENERATED_SDK}/anito/generated/api/${category_name}.hpp")
	set(category_host "${SCRIPT_API_GENERATED_SDK}/anito/generated/host/${category_name}.hpp")
	set(category_host_wrapper "${SCRIPT_API_GENERATED_SDK}/anito/host/${category_name}.hpp")
	set(category_bridge "${SCRIPT_API_GENERATED_SDK}/generated/ScriptHostAPI/${category_name}.cpp")
	list(APPEND SCRIPT_API_GENERATED_OUTPUTS "${category_api}" "${category_host}" "${category_host_wrapper}" "${category_bridge}")
	list(APPEND SCRIPT_API_GENERATED_BRIDGES "${category_bridge}")
endforeach()

add_executable(anito_scriptapigen tools/script_api_codegen.cpp)
target_link_libraries(anito_scriptapigen PRIVATE anito::scriptapi)
set_target_properties(anito_scriptapigen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ScriptTools")

add_custom_command(
	OUTPUT ${SCRIPT_API_GENERATED_OUTPUTS}
	COMMAND $<TARGET_FILE:anito_scriptapigen>
		"${SCRIPT_API_GENERATED_SDK}" "${SCRIPT_API_COMPONENT_ROOT}" ${SCRIPT_HOST_API_LAYOUT}
	DEPENDS anito_scriptapigen
	COMMENT "Generating component script SDK and host bindings"
	VERBATIM
)
add_custom_target(anito_scriptapi_codegen
	DEPENDS ${SCRIPT_API_GENERATED_OUTPUTS}
)

# Warnings in generated code are not ours to fix.
set_source_files_properties(${ANTLR_GEN_SRC} PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/w>")

# Command-line compiler: scriptc <out.cpp> <script.ascript>...
add_executable(anito_scriptc tools/scriptc.cpp)
target_link_libraries(anito_scriptc PRIVATE anito::scriptlang)
set_target_properties(anito_scriptc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ScriptTools")

file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" ANITO_SCRIPT_COMPILER)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk" ANITO_SCRIPT_SDK_INCLUDE)
file(TO_CMAKE_PATH "${SCRIPT_API_GENERATED_SDK}" ANITO_SCRIPT_SDK_GENERATED_INCLUDE)
configure_file(ScriptBuildConfig.hpp.in ScriptBuildConfig.hpp @ONLY)

set(${CUR_TARGET_NAME}_src
	ScriptBuildDriver.cpp
	ScriptBuildDriver.hpp
	ScriptBuildConfig.hpp.in
	ScriptRegistry.cpp
	ScriptRegistry.hpp
	ScriptComponent.cpp
	ScriptComponent.hpp
	ScriptHostAPI.cpp
	ScriptHostAPI.hpp
	ScriptHostAPI/Bindings.hpp
	ScriptHostAPI/Bindings/Fields.hpp
	ScriptHostAPI/Bindings/Gui.hpp
	ScriptHostAPI/Bindings/Input.hpp
	ScriptHostAPI/Bindings/Log.hpp
	ScriptHostAPI/Bindings/Object.hpp
	ScriptHostAPI/Bindings/Scene.hpp
	ScriptHostAPI/Bindings/Scripts.hpp
	ScriptHostAPI/Bindings/Time.hpp
	ScriptHostAPI/Common.hpp
	${SCRIPT_API_GENERATED_COMPONENT_BINDINGS}
	ScriptHostAPI/Object.cpp
	ScriptHostAPI/Scene.cpp
	ScriptHostAPI/Scripts.cpp
	ScriptHostAPI/Input.cpp
	ScriptHostAPI/Time.cpp
	ScriptHostAPI/Gui.cpp
	ScriptHostAPI/Fields.cpp
	ScriptModule.cpp
	ScriptModule.hpp
	sdk/AnitoScriptSDK.hpp
	sdk/anito/Api.hpp
	sdk/anito/Host.hpp
	sdk/anito/Math.hpp
	${SCRIPT_API_GENERATED_BRIDGES}
)

add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
add_dependencies(${CUR_TARGET_NAME} anito_scriptapi_codegen)
target_include_directories(${CUR_TARGET_NAME}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sdk "${SCRIPT_API_GENERATED_SDK}"
)
target_include_directories(${CUR_TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(${CUR_TARGET_NAME}
	PRIVATE
	anito::rendering
)

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	anito::scriptlang
	glm::glm
	anito::objects
	anito::serialization
	gbe::tools
)

if(BUILD_TESTING)
	add_executable(script_api_layout_tests tests/ScriptApiLayoutTests.cpp)
	add_dependencies(script_api_layout_tests anito_scriptapi_codegen)
	target_include_directories(script_api_layout_tests PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/sdk" "${SCRIPT_API_GENERATED_SDK}"
	)
	add_test(NAME script_api_layout_tests COMMAND script_api_layout_tests)
	add_test(NAME script_api_codegen_tests
		COMMAND ${CMAKE_COMMAND}
			"-DGENERATOR=$<TARGET_FILE:anito_scriptapigen>"
			"-DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/script_api_codegen_tests"
			"-DCOMPONENT_ROOT=${SCRIPT_API_COMPONENT_ROOT}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/tests/ScriptApiCodegenTests.cmake"
	)
	if(WIN32)
		set(sample_script "${CMAKE_SOURCE_DIR}/SampleProjects/Scripting/Scripts/SpinAndBob.ascript")
		set(sample_source "${CMAKE_CURRENT_BINARY_DIR}/script_api_tests/SpinAndBob.gen.cpp")
		add_custom_command(
			OUTPUT "${sample_source}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/script_api_tests"
			COMMAND $<TARGET_FILE:anito_scriptc> "${sample_source}" "${sample_script}"
			DEPENDS anito_scriptc "${sample_script}"
			VERBATIM
		)
		add_library(script_api_sample_module MODULE "${sample_source}")
		add_dependencies(script_api_sample_module anito_scriptapi_codegen)
		target_include_directories(script_api_sample_module PRIVATE
			"${CMAKE_CURRENT_SOURCE_DIR}/sdk" "${SCRIPT_API_GENERATED_SDK}"
		)
		add_executable(script_api_module_tests tests/ScriptApiModuleTests.cpp)
		add_dependencies(script_api_module_tests anito_scriptapi_codegen script_api_sample_module)
		target_include_directories(script_api_module_tests PRIVATE
			"${CMAKE_CURRENT_SOURCE_DIR}/sdk" "${SCRIPT_API_GENERATED_SDK}"
		)
		add_test(NAME script_api_module_tests
			COMMAND script_api_module_tests "$<TARGET_FILE:script_api_sample_module>")
	endif()
endif()
