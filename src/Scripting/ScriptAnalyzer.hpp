#pragma once

#include "AnitoScriptParser.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Type-checks a syntactically valid parse tree and transpiles each component to a C++ class.
// Errors are appended as `path(line:col): message`. The result maps component name to class source
// and is only filled when no semantic errors were found.
std::map<std::string, std::string> AnalyzeScript(anito_script::AnitoScriptParser::ScriptContext* script,
                                                 const std::filesystem::path& path,
                                                 std::vector<std::string>& errors);
