#pragma once

#include "AnitoScriptParser.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Performs semantic analysis and C++ lowering in one tree walk; there is no separate typed AST.
// The input must be syntactically valid, and generated classes are returned only as a clean set.
// Errors are appended as `path(line:col): message`. The result maps component name to class source
// and is only filled when no semantic errors were found.
std::map<std::string, std::string> AnalyzeScript(anito_script::AnitoScriptParser::ScriptContext* script,
                                                 const std::filesystem::path& path,
                                                 std::vector<std::string>& errors);
