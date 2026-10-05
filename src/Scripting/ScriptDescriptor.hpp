#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

enum class ScriptFieldType { Float, Int, Bool, String, Vec3 };

using ScriptValue = std::variant<float, int, bool, std::string, glm::vec3>;

struct ScriptField {
    std::string name;
    std::string displayName;
    ScriptFieldType type = ScriptFieldType::Float;
    ScriptValue defaultValue = 0.0f;
};

// Declaration-level view of one `component` found in an .ascript file.
struct ScriptDescriptor {
    std::string name;
    std::filesystem::path path;
    std::vector<ScriptField> fields;
    std::vector<std::string> events;   // `on Update(...)` handlers
    std::vector<std::string> methods;  // `@method fn` entries
    uint64_t revision = 0;             // Bumped whenever the file is re-parsed
};
