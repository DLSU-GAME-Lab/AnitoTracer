
# What Is Automatic

The ANTLR lexer and parser C++ files are generated from the grammar files during the CMake build. In `CMakeLists.txt`, a custom command runs the ANTLR JAR with Java, and declares both `.g4` files and the JAR as dependencies. Generated files go into the build directory, under `src/Scripting/generated`; they are not handwritten source files to edit.

So, after changing a grammar file, **building the project regenerates the ANTLR C++ parser automatically**. The ANTLR command does not normally need to be manually run. Java and the configured ANTLR JAR must be available.

# What Is Handwritten

ANTLR’s generated parser only recognizes the syntax and creates a parse tree. It does not decide what the language means or generate DLL code.

* `AnitoScriptLexer.g4` defines tokens such as keywords, operators and literals.  
* `AnitoScriptParser.g4` defines the language’s syntax rules: declarations, statements, expressions and precedence.  
* `ScriptParser.cpp` runs the generated lexer/parser, captures syntax errors, and extracts component descriptors and exposed fields/default values.  
* `ScriptAnalyzer.cpp` checks names and types and emits C++ for component bodies during the same traversal. It is both the semantic checker and the main component-code generator, rather than a separate typed-AST pass.  
* `ScriptTranspiler.cpp` assembles the generated component classes into a module translation unit, including the DLL entry table and factories. It generally does not need changes for an ordinary new language expression or statement.  
* `AnitoScriptSDK.hpp` defines the functions and types generated scripts can use to call into the engine.

The editor uses `ScriptParser::Parse()` directly; it does not run the `scriptc` executable. That parser/analyzer path produces each component’s C++ source, and `BuildModuleSource()` assembles it into `Scripts.gen.cpp`.

# Adding a Feature

The work depends on whether the new feature changes syntax:

1. **Syntax change:** Update the appropriate `.g4` rule or rules. For new words or symbols, update the lexer grammar too. Pay attention to token conflicts and expression precedence.  
2. **Parser integration:** Build once to regenerate ANTLR. Then update `ScriptParser.cpp` if the feature adds information the editor or descriptor needs. Inspect the generated parser context/accessor names if you need to consume a new grammar rule.  
3. **Meaning and generated code:** Update `ScriptAnalyzer.cpp` to validate the feature and emit equivalent C++. If it calls a new engine operation, extend the SDK and host implementation too.  
4. **Build and exercise it:** Build through CMake Tools, try valid and invalid examples with `anito_scriptc`, and verify the generated C++ compiles. Then test through the editor, where changed `.ascript` files flow into generated C++, a DLL build, and the reload path.

For example, adding `clamp(x, low, high)` does **not** need a grammar change: calls already use generic `name(args)` syntax. You would add its accepted argument types and return type to the analyzer, then emit the corresponding C++ expression. A new syntax form such as `for item in collection` **does** require a grammar change, plus semantic handling and a C++ translation in the analyzer.

# Rule of thumb

**ANTLR automates parser regeneration; it does not automate language-feature implementation.** A grammar edit is often only the first step. The build and editor pipeline then picks up the changes automatically once the handwritten parser/analyzer behavior supports them.

# Summary and Key files

* `AnitoTracer_App.cpp`: coordinates registry, builder and module polling.  
* `ScriptRegistry.cpp`: file discovery, parse cache, error storage and generated-source writing.  
* `ScriptParser.cpp`: ANTLR parsing and descriptor extraction.  
* `ScriptAnalyzer.cpp`: semantic checks and component-class generation.  
* `ScriptTranspiler.cpp`: builds the exported DLL translation unit.  
* `ScriptBuildDriver.cpp`: debounced asynchronous MSVC build.  
* `ScriptModule.cpp`: DLL copy, load, validation, unload and reload.  
* `ScriptComponent.cpp`: field storage, native-instance lifecycle and per-frame calls.  
* `AnitoScriptSDK.hpp`: stable host/module ABI.