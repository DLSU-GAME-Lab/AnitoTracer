#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

enum class ScriptFieldType { Float, Int, Bool, String, Vec3 };

using ScriptValue = std::variant<float, int, bool, std::string, glm::vec3>;

// One @field declaration, shared by the inspector/serializer and the generated module ABI.
struct ScriptField {
    std::string name;
    std::string displayName;
    ScriptFieldType type = ScriptFieldType::Float;
    ScriptValue defaultValue = 0.0f;
};

// Bridge between source declarations and the host: editor metadata is available before a DLL
// exists, while cppSource is populated only after a clean semantic analysis.
struct ScriptDescriptor {
    std::string name;
    std::filesystem::path path;
    std::vector<ScriptField> fields;
    std::vector<std::string> events;   // `on Update(...)` handlers
    std::vector<std::string> methods;  // `@method fn` entries
    uint64_t revision = 0;             // Bumped whenever the file is re-parsed
    std::string cppSource;             // Transpiled class; empty if the file had errors
};
