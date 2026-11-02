#pragma once

#include "ScriptDescriptor.hpp"

#include <string>
#include <vector>

// Front end for .ascript files: ANTLR supplies the parse tree, this layer extracts editor-facing
// component metadata, and clean trees continue to semantic analysis and C++ generation.
class ScriptParser {
public:
    static std::vector<ScriptDescriptor> Parse(const std::string& source,
                                               const std::filesystem::path& path,
                                               std::vector<std::string>& errors);
};
