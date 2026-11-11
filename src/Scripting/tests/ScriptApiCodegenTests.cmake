set(layout
	log
	component:transform component:physics component:camera component:audio
	static:object:ObjectAPI:object:Object
	static:scene:SceneAPI:scene:Scene
	static:script:ScriptAPI:scripts:Script
	static:input:InputAPI:input:Runtime
	static:time:TimeAPI:time:Runtime
	static:gui:GuiAPI:gui:Runtime
	static:field:FieldAPI:field:Runtime
)

function(run_generator expected_error)
	execute_process(
		COMMAND "${GENERATOR}" "${OUTPUT_ROOT}" "${COMPONENT_ROOT}" ${ARGN}
		RESULT_VARIABLE result ERROR_VARIABLE error
	)
	if(expected_error STREQUAL "")
		if(NOT result STREQUAL "0")
			message(FATAL_ERROR "Generator failed: ${error}")
		endif()
	elseif(result STREQUAL "0" OR NOT error MATCHES "${expected_error}")
		message(FATAL_ERROR "Expected '${expected_error}', got result '${result}': ${error}")
	endif()
endfunction()

run_generator("" ${layout})

function(expect_file path expected)
	file(READ "${OUTPUT_ROOT}/${path}" actual)
	if(NOT actual STREQUAL expected)
		message(FATAL_ERROR "Unexpected contents of ${path}:\n${actual}")
	endif()
endfunction()

expect_file("anito/generated/host/HostFields.inc"
	"    void (*Log)(const char* message);\n    TransformAPI transform;\n    PhysicsAPI physics;\n    CameraAPI camera;\n    AudioAPI audio;\n    ObjectAPI object;\n    SceneAPI scene;\n    ScriptAPI script;\n    InputAPI input;\n    TimeAPI time;\n    GuiAPI gui;\n    FieldAPI field;\n")
expect_file("generated/ScriptHostAPI/HostInitializers.inc"
	"    ScriptHost::Bindings::Log,\n    ScriptHost::Bindings::transform::GetAPI(),\n    ScriptHost::Bindings::physics::GetAPI(),\n    ScriptHost::Bindings::camera::GetAPI(),\n    ScriptHost::Bindings::audio::GetAPI(),\n    ScriptHost::Bindings::object::GetAPI(),\n    ScriptHost::Bindings::scene::GetAPI(),\n    ScriptHost::Bindings::scripts::GetAPI(),\n    ScriptHost::Bindings::input::GetAPI(),\n    ScriptHost::Bindings::time::GetAPI(),\n    ScriptHost::Bindings::gui::GetAPI(),\n    ScriptHost::Bindings::field::GetAPI(),\n")
expect_file("anito/generated/host/HostIncludes.hpp"
	"#pragma once\n\n#include \"anito/host/Transform.hpp\"\n#include \"anito/host/Physics.hpp\"\n#include \"anito/host/Camera.hpp\"\n#include \"anito/host/Audio.hpp\"\n#include \"anito/host/Object.hpp\"\n#include \"anito/host/Scene.hpp\"\n#include \"anito/host/Script.hpp\"\n#include \"anito/host/Runtime.hpp\"\n")
foreach(category Transform Physics Camera Audio)
	expect_file("anito/host/${category}.hpp"
		"#pragma once\n\n#include \"anito/generated/host/${category}.hpp\"\n")
endforeach()

run_generator("duplicate component header" ${layout} component:audio)
run_generator("missing log" component:audio)
run_generator("duplicate log" log log)
run_generator("invalid component category" log component:class)
run_generator("invalid HostAPI layout entry" log bad:entry)
run_generator("duplicate HostAPI field" ${layout} static:audio:OtherAPI:other:Other)
run_generator("conflicts with handwritten header" log component:runtime static:other:OtherAPI:other:Runtime)
run_generator("conflicts with handwritten binding" log component:scripts static:other:OtherAPI:scripts:Other)
run_generator("duplicate HostAPI table type: HostAPI" log component:host)
run_generator("no annotated component uses host category: missing" ${layout} component:missing)
run_generator("declares host category.*SCRIPT_HOST_API_LAYOUT" log component:audio)
