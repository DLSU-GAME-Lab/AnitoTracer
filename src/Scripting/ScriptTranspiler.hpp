#pragma once

#include "ScriptDescriptor.hpp"

#include <string>
#include <vector>

// Assembles one C++ translation unit (all component classes plus the exported entry table) for a DLL.
std::string BuildModuleSource(const std::vector<const ScriptDescriptor*>& scripts);
