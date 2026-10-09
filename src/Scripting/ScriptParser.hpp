#pragma once

#include "ScriptDescriptor.hpp"

#include <string>
#include <vector>

// ANTLR-based reader for .ascript files (grammar in grammar/*.g4). Currently extracts
// component declarations only; function bodies are parsed for syntax but not yet used.
class ScriptParser {
public:
    static std::vector<ScriptDescriptor> Parse(const std::string& source,
                                               const std::filesystem::path& path,
                                               std::vector<std::string>& errors);
};
