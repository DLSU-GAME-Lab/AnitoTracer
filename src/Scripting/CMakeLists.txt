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

# Language core: parser, analyzer, transpiler. No engine dependencies so the CLI compiler can reuse it.
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

# Warnings in generated code are not ours to fix.
set_source_files_properties(${ANTLR_GEN_SRC} PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/w>")

# Command-line compiler: scriptc <out.cpp> <script.ascript>...
add_executable(anito_scriptc tools/scriptc.cpp)
target_link_libraries(anito_scriptc PRIVATE anito::scriptlang)
set_target_properties(anito_scriptc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/ScriptTools")

file(TO_CMAKE_PATH "${CMAKE_CXX_COMPILER}" ANITO_SCRIPT_COMPILER)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk" ANITO_SCRIPT_SDK_INCLUDE)
configure_file(ScriptBuildConfig.hpp.in ScriptBuildConfig.hpp @ONLY)

set(${CUR_TARGET_NAME}_src
	ScriptBuildDriver.cpp
	ScriptBuildDriver.hpp
	ScriptBuildConfig.hpp.in
	ScriptRegistry.cpp
	ScriptRegistry.hpp
	ScriptComponent.cpp
	ScriptComponent.hpp
	ScriptModule.cpp
	ScriptModule.hpp
	sdk/AnitoScriptSDK.hpp
)

add_library(${CUR_TARGET_NAME} STATIC ${${CUR_TARGET_NAME}_src})
add_library(anito::${CUR_TARGET_SUBLIBNAME} ALIAS ${CUR_TARGET_NAME})
target_include_directories(${CUR_TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
target_include_directories(${CUR_TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(${CUR_TARGET_NAME}
	PUBLIC
	anito::scriptlang
	glm::glm
	anito::objects
	anito::serialization
	gbe::tools
)
