# AnitoTracer scripting

## What is automatic

The ANTLR lexer and parser C++ files are generated from the grammar files during the CMake build. A custom command runs the ANTLR JAR with Java and declares both `.g4` files and the JAR as dependencies. Generated files go into the build directory under `src/Scripting/generated`; they are not handwritten source files to edit.

After changing a grammar file, **building the project regenerates the ANTLR C++ parser automatically**. Java and the configured ANTLR JAR must be available.

The engine component API has a separate, partial automation layer. `ANITO_SCRIPT_*` annotations in a component header register its script name, properties, and methods as metadata. `ScriptAnalyzer.cpp` reads that metadata to describe registered component members. For AudioSource, the build also generates the SDK handle, host function table, engine bridge, and conformance checks. Other component handles and bridges remain handwritten for now.

AudioSource is the first generated component API: the build runs `anito_scriptapigen` against the registered annotations and emits its SDK handle, host function table, engine bridge, and conformance checks into the build directory. Generated output is not edited by hand. The initial generator only accepts AudioSource and supports mapped scalar, string, `vec3`, and `quat` signatures; unsupported signatures fail generation instead of producing an unsafe bridge.

## What is handwritten

ANTLR recognizes syntax and creates a parse tree; it does not decide what the language means or generate DLL code.

* `grammar/AnitoScriptLexer.g4` defines tokens such as keywords, operators, and literals.
* `grammar/AnitoScriptParser.g4` defines declarations, statements, expressions, and precedence.
* `ScriptParser.cpp` runs the generated lexer/parser, captures syntax errors, and extracts component descriptors and exposed fields/default values.
* `ScriptAnalyzer.cpp` checks names and types and emits C++ for component bodies during the same traversal. It is both the semantic checker and the main component-code generator, rather than a separate typed-AST pass.
* `ScriptTranspiler.cpp` assembles generated component classes into a module translation unit, including the DLL entry table and factories.
* `sdk/anito/` defines the engine-independent types and inline wrappers visible to generated scripts. Calls into the engine go through the host function table.
* `ScriptHostAPI.cpp` implements the engine side of that function table.

The editor uses `ScriptParser::Parse()` directly, not the `scriptc` executable. That parser/analyzer path produces each component's C++ source, and `BuildModuleSource()` assembles it into `Scripts.gen.cpp`.

## Adding a script API for a C++ component

Use this path when a script API represents an instance of an engine component, for example `audiosource.volume` or `rigidbody.velocity`. AudioSource is the only component whose full API plumbing is generated today. For other components, the annotations still drive analyzer metadata and conformance checks, but the SDK handle and host bridge must also be wired manually.

### 1. Annotate the engine component

Include `ScriptApiRegistry.hpp` in the component header and put annotations after the component class definition. Choose a lower-case script type name and an SDK C++ handle type name. Register only methods and properties intended to be public to scripts.

```cpp
ANITO_SCRIPT_TYPE(MyComponent, "mycomponent", "anito::MyComponent", true, "audio");
ANITO_SCRIPT_PROPERTY(MyComponent, "mycomponent", "speed", GetSpeed, SetSpeed);
ANITO_SCRIPT_PROPERTY_RO(MyComponent, "mycomponent", "isMoving", IsMoving);
ANITO_SCRIPT_METHOD(MyComponent, "mycomponent", "stop", Stop);
```

`ANITO_SCRIPT_TYPE` takes the engine C++ class, script type name, SDK handle's fully qualified name, whether the handle supports `valid()`, and the host API category. A property getter must return a value; a writable property's setter must return `void`, accept one value, and use the same C++ type as the getter. A method can use only types understood by the script API type mappings in `ScriptApiRegistry.hpp`. Existing mappings include `void`, `float`, `int`, `bool`, `std::string`, `glm::vec3`, and `glm::quat`.

The macros register metadata at startup in `anito_scriptapi`; they do not invoke component methods from a script. Do not annotate serialization fields as script API members unless they are meant to be exposed.

Include the component header from `ScriptApiMetadata.cpp` so its annotations are compiled into the metadata target. Do not include generated SDK headers there: the generated bridge contains AudioSource conformance assertions, while `ScriptApiConformance.cpp` holds checks for the still-handwritten component handles.

### 2. Generated AudioSource path and manual fallback

For AudioSource, the generator creates the SDK handle and ABI table directly from its annotations; `sdk/anito/api/AudioSource.hpp` and `sdk/anito/host/Audio.hpp` are source shims that include those build outputs. The `GameObject::audioSource()` accessor remains handwritten in the SDK and should be kept in sync with the handle name.

For another component, create `sdk/anito/api/MyComponent.hpp` with a small `ObjectId` handle whose members call `detail::H()` function pointers. Add it to the SDK umbrella headers and add a `GameObject` accessor in `sdk/anito/api/Objects.hpp`. The corresponding `GameObject` member in the analyzer's `T::GameObject` description is also explicit.

### 3. Manual host bridge for other components

For a component other than AudioSource, add or extend a category function table in `sdk/anito/host/`, include it through the host headers, and implement its function pointers in the matching `ScriptHostAPI` category source. The bridge receives object IDs, resolves the engine component, handles a missing component, converts values at the boundary, and invokes the component. Keep engine headers and types on the host side; generated script DLLs should depend only on the SDK and function-table headers.

### 4. Add conformance checks

For a handwritten component, include the engine and SDK handle headers in `ScriptApiConformance.cpp`. Add `ANITO_SCRIPT_CONFORMS_VALID` for a handle that exposes `valid()`, and add one matching conformance macro for every registered member:

```cpp
ANITO_SCRIPT_CONFORMS_PROPERTY(MyComponent, GetSpeed, SetSpeed,
                               anito::MyComponent, speed, setSpeed);
ANITO_SCRIPT_CONFORMS_READ_ONLY_PROPERTY(MyComponent, IsMoving,
                                         anito::MyComponent, isMoving);
ANITO_SCRIPT_CONFORMS_METHOD(MyComponent, Stop, anito::MyComponent, stop);
```

These compile-time checks compare script-level return and parameter types and arity. AudioSource checks are generated from its annotations; handwritten checks live in `ScriptApiConformance.cpp`. The SDK setter for a writable property is named `set` plus the capitalized property name (`speed` becomes `setSpeed`). Add a `ScriptTypeMapping` for a new value type only if that value type is intentionally supported by the script API, and add its script-analyzer type representation as needed.

### 5. Build and exercise

Build `anito_scriptapi`, `anito_scriptapigen`, and `anito_scripting`, then compile representative `.ascript` files with `anito_scriptc` or the editor. Verify valid usage and a deliberately invalid type/member access. Confirm the generated `Scripts.gen.cpp` compiles and the resulting module loads and invokes the host bridge.

## Adding a static, non-component API

Use a static API when the operation is not naturally owned by one component instance. `Scene.mainCamera()` is an example: it finds the active camera globally and returns its owning GameObject. Static APIs are **manually described**, unlike component members. Keep their analyzer, SDK, and host descriptions in sync.

1. **Analyzer:** Add a static type to `T` and `kStaticNames` in `ScriptAnalyzer.cpp` if it is a new script-visible class. Add its members and methods to the static `Info()` table. For an addition to an existing class, update that class's `TypeInfo`. The analyzer uses these entries for validation and C++ emission.
2. **SDK:** Add or update a class under `sdk/anito/api/`. Its static methods should call the corresponding `detail::H().<category>` function pointer. Add the SDK header to the umbrella include path if it is not already included.
3. **Host ABI:** Declare the function in the appropriate `sdk/anito/host/<Category>.hpp` function table. Include that table in `HostAPI.hpp` if it introduces a new category.
4. **Engine bridge:** Implement the function in `ScriptHostAPI.cpp` and populate the matching function table in `ScriptHost::GetAPI()`. Convert arguments/results at this boundary. Do not let SDK headers depend on engine headers.
5. **Validate:** Build the engine and compile a script using the API, then load it and check the returned values and side effects.

For example, `Scene.mainCamera()` is manually declared in the `T::SScene` `Info()` entry, declared as an inline static method in `sdk/anito/api/Scene.hpp`, represented by `SceneAPI::GetMainCamera`, and implemented by `scene::GetMainCamera` in `ScriptHostAPI.cpp`. Its return value is an object ID wrapped as `GameObject`. Static APIs do not currently use the component conformance macros, so review their analyzer signature, SDK wrapper, host function-table signature, and implementation together.

## Conformance checks and errors

The conformance macros are C++ `static_assert`s: AudioSource checks are generated into its host bridge, and checks for handwritten handles live in `ScriptApiConformance.cpp`. They compare engine component method signatures with SDK handle methods after mapping C++ types to script types. A failing check prevents the engine scripting target from building; it is **not** a `Scripts.dll` linker error. Unsupported annotated types instead fail while compiling the `anito_scriptapi` metadata target.

| Diagnostic or symptom | What it means | What to check |
|---|---|---|
| `Script SDK property getter does not conform to component metadata` | The property getter's script-level return type or parameter list differs between the component and SDK handle. | Compare the named component getter and SDK getter; check the type mapping and `const`/overload selection. |
| `Script SDK property setter does not conform to component metadata` | The writable property getter may match, but its setter signature does not match the component setter. | Confirm both setters accept one value of the property type and return `void`; compare SDK and engine parameter types. |
| `Script SDK method does not conform to component metadata` | The method return type, parameter count, or one or more parameter types differ. | Compare the component method and SDK method named in the macro. |
| `Script SDK handle must expose bool valid() const` | A handle marked as having validity support does not have the exact expected `valid()` member signature. | Implement `bool valid() const` on the SDK handle or set `HasValid` to false if validity is not part of that handle contract. |
| `Type is not supported by the script API` | An annotated C++ getter, setter, or method uses a type without a `ScriptTypeMapping`. | Use a supported type or intentionally add a script type mapping and analyzer representation. This is a compile-time API metadata error. |
| Script analysis reports an unknown type/member or a type mismatch | The analyzer does not know the name/signature, or the script call is invalid. This happens before the generated DLL is linked. | Check component annotations, the analyzer's registered type/type-name plumbing, `GameObject` accessor metadata, and the script source. |
| `Scripts.dll` build reports a compiler error | The generated script C++ is invalid or does not match the SDK declarations. | Read `Library/Scripts/Scripts.build.log`; check generated code, script syntax/types, SDK includes, and whether a new script-facing type was added to the analyzer. |
| Linker reports an unresolved external while building `Scripts.dll` (for example, MSVC `LNK2019`) | The generated translation unit refers to a function that is declared but has no definition available to the script DLL. Script DLLs use the header-only SDK and host function table; they should not link directly to engine implementation symbols. | Make the SDK wrapper inline and route engine work through a host function-table entry. Check for accidental calls to engine-only functions or non-inline SDK declarations. This is a DLL link failure, not a conformance failure. |
| `[Script] LoadLibrary failed (...)` | Windows could not load the just-built DLL, often because a dependent DLL is missing, the binary architecture is wrong, or the file is invalid. | Inspect the Windows error code and dependencies; confirm the script DLL and engine have compatible architectures and runtime setup. |
| `Module is missing AnitoScript_GetModule or has a different ABI version.` | The loader could not obtain the expected module entry point, or the module ABI version differs from the engine's `ANITO_SCRIPT_ABI_VERSION`. | Rebuild scripts with the current SDK. If the cross-DLL `HostAPI`, function-table layout, `ModuleInfo`, or other ABI contract changed, update the ABI version and rebuild both sides. |
| A script component has no running instance although the DLL loaded | The module has no matching script entry, or its field count/types do not match the parsed script descriptor. This is an entry/layout compatibility check, not a DLL link error. | Check that the generated module is current and that script names and exposed fields agree. `ScriptModule::FindEntry()` rejects mismatched field layouts. |

Conformance checks validate component-to-SDK C++ signatures only. They do not prove that the correct host function is wired, that its implementation has the intended behavior, or that a manually described static API matches the host ABI. Test the end-to-end bridge as well.

## Adding a language feature

The work depends on whether the feature changes syntax:

1. **Syntax change:** Update the appropriate `.g4` rule or rules. For new words or symbols, update the lexer grammar too. Pay attention to token conflicts and expression precedence.
2. **Parser integration:** Build once to regenerate ANTLR. Then update `ScriptParser.cpp` if the feature adds information the editor or descriptor needs. Inspect generated parser context/accessor names if you need to consume a new grammar rule.
3. **Meaning and generated code:** Update `ScriptAnalyzer.cpp` to validate the feature and emit equivalent C++. If it calls a new engine operation, extend the SDK and host implementation too.
4. **Build and exercise:** Build through CMake, try valid and invalid examples with `anito_scriptc`, and verify generated C++ compiles. Then test through the editor, where changed `.ascript` files flow into generated C++, a DLL build, and the reload path.

For example, adding `clamp(x, low, high)` does not need a grammar change: calls already use generic `name(args)` syntax. Add its accepted argument and return types to the analyzer and emit the corresponding C++ expression. A new syntax form such as `for item in collection` does require a grammar change, plus semantic handling and a C++ translation in the analyzer.

## Scripting API by category

`.ascript` follows a handle-based model: component and object handles expose instance members, while helper APIs such as `Scene`, `Input`, and `Time` are static.

| Category | Script surface |
|---|---|
| Transform | `transform.position` (world), `localPosition`, `rotation`, `localRotation`, `localScale`, `lossyScale`, `eulerAngles`, `forward/right/up`, `rotate`, `translate`, `lookAt`, `transformPoint/Direction` |
| Physics | `rigidbody.velocity/angularVelocity/mass/simulated/rotation/worldGravity`, `addForce/addTorque/addImpulse/teleport/lockRotation/raycast` (`raycasthit`); `worldGravity` is global gravity accessed through a valid Rigidbody handle |
| Camera | `gameobject.camera` returns a Camera handle; `camera.fov/aspect/nearPlane/farPlane`; `Scene.mainCamera()` returns the active camera's GameObject |
| Audio | `audiosource.play/stop/playDetached/volume/loop/pitch/spatialized/setDistance` |
| Component / GameObject getters | `owner`, `name`, `parent`, `root`, `child(i)`, `findChild`, `transform`, `rigidbody`, `collider`, `audioSource`, `camera`, `getScript/hasScript/addScript`, `hasComponent`, `addRigidbody/addBoxCollider/addSphereCollider/addCapsuleCollider`, `destroy`, `valid()` |
| Scene | `Scene.find/findWithScript/instantiate/createEmpty/createPrimitive/none/mainCamera` |
| Scripts | `script.call/callFloat/callInt/callBool/callVec3/callObject`, `gameobject.sendMessage`; every `fn` with simple parameter types is callable |
| Input / Time | `Input.getAction/mouseDelta/mousePosition`, `Time.time/deltaTime/fixedDeltaTime` |
| GUI / Debug | `Gui.worldToScreen/fillRect/drawRect` (inside `on GUI`), `Debug.log` |
| Math | `Mathf`, `Vector3`, `Quaternion` (degrees), `Random`, plus `vec3`/`quat` members |
| Collections / assets | generic `array<T>` (storable element types): `count`, `add`, `insert`, `removeAt`, `clear`, `get(i)`, `a[i]` (read/write), `contains`, `indexOf`, `prune()` (handle elements); `prefab` and `transform` `@field`s |

Event handlers: `on Start()`, `on Update(dt)`, `on FixedUpdate(dt)`, `on GUI(dt)`, `on CollisionEnter(other)`. Declare the ones a component uses after the colon, e.g. `component Foo : Update, CollisionEnter`.

Note: `transform.position` is world space; use `localPosition` for the parent-relative value.

## Adding a handwritten host binding

Host-side API implementations are grouped by category under `ScriptHostAPI/`. Each category `.cpp` owns the engine-facing implementation for its function table, while `ScriptHostAPI.cpp` assembles those functions into the stable `HostAPI` ABI.

To add a handwritten binding:

1. Add or update the declaration in the matching `ScriptHostAPI/Bindings/<Category>.hpp`. Implement the function in the matching `ScriptHostAPI/<Category>.cpp`; shared `ObjectId` lookup and SDK/engine conversions are available in `ScriptHostAPI/Common.hpp`.
2. Keep domain behavior in the owning engine component or service; a bridge should resolve, convert, forward, and convert the result.
3. Each category implementation provides its own `GetAPI()` function table. Adding a function to an existing category does not require editing `ScriptHostAPI.cpp`. The `ScriptHostAPI/Bindings.hpp` header only includes the category headers.
4. For a new category, add its SDK ABI table under `sdk/anito/host/`, include it in `HostAPI.hpp`, provide a category `GetAPI()`, include its header from `ScriptHostAPI/Bindings.hpp`, add its source/header to `CMakeLists.txt`, and add the category table to the `HostAPI` aggregate in `ScriptHostAPI.cpp`.
5. Add or update the SDK wrapper and analyzer description as needed. If the `HostAPI` layout or function signatures change, increment `ANITO_SCRIPT_ABI_VERSION` and rebuild both the engine and script DLLs.

This category split is an organizational step, not code generation: SDK/analyzer declarations and ABI table declarations are still manual. Existing category functions are self-contained; only introducing a new ABI category requires editing the root table.

## Summary and key files

* `AnitoTracer_App.cpp`: coordinates registry, builder, and module polling.
* `ScriptRegistry.cpp`: file discovery, parse cache, error storage, and generated-source writing.
* `ScriptParser.cpp`: ANTLR parsing and descriptor extraction.
* `ScriptAnalyzer.cpp`: semantic checks and component-class generation, plus static API descriptions.
* `ScriptTranspiler.cpp`: builds the exported DLL translation unit.
* `ScriptBuildDriver.cpp`: debounced asynchronous MSVC build.
* `ScriptModule.cpp`: DLL copy, load, ABI validation, unload, and reload.
* `ScriptComponent.cpp`: field storage, native-instance lifecycle, and per-frame calls.
* `ScriptApiMetadata.cpp`: registered component metadata.
* `ScriptApiConformance.cpp`: conformance checks for handwritten component SDK handles.
* `tools/script_api_codegen.cpp`: emits generated component SDK and host-binding code.
* `../Common/ScriptApiRegistry.hpp`: component registration macros, type mappings, and conformance helpers.
* `sdk/AnitoScriptSDK.hpp`: umbrella include for the script SDK.
* `sdk/anito/host/`: stable host/module ABI; current `ANITO_SCRIPT_ABI_VERSION` is 5.
* `sdk/anito/api/`: handle classes and static wrappers called by generated scripts.
* `sdk/anito/Math.hpp`: `Vec3`, `Quat`, `Mathf`, `Vector3`, `Quaternion`, and `Random`.
* `ScriptHostAPI.cpp`: assembles the host function tables; implementations are grouped under `ScriptHostAPI/`.
