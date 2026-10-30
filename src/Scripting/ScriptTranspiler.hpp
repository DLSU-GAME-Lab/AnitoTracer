#pragma once

#include "ScriptDescriptor.hpp"

#include <string>
#include <vector>

// Packages analyzed component classes with field layouts and factories into the stable SDK module
// entry point; generated scripts compile as an independent DLL translation unit.
std::string BuildModuleSource(const std::vector<const ScriptDescriptor*>& scripts);
