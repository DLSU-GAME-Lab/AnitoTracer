// Command-line compiler: scriptc <out.cpp> <script.ascript>...
#include "ScriptParser.hpp"
#include "ScriptTranspiler.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: scriptc <out.cpp> <script.ascript>..." << std::endl;
        return 2;
    }

    std::vector<ScriptDescriptor> scripts;
    std::vector<std::string> errors;
    for (int i = 2; i < argc; ++i) {
        std::ifstream stream(argv[i]);
        if (!stream) {
            std::cerr << "cannot open " << argv[i] << std::endl;
            return 2;
        }
        std::stringstream buffer;
        buffer << stream.rdbuf();
        for (auto& script : ScriptParser::Parse(buffer.str(), std::filesystem::absolute(argv[i]).lexically_normal(), errors)) {
            scripts.push_back(std::move(script));
        }
    }

    for (const std::string& error : errors) std::cerr << error << std::endl;
    if (!errors.empty()) return 1;

    std::vector<const ScriptDescriptor*> pointers;
    for (const auto& script : scripts) pointers.push_back(&script);

    std::ofstream out(argv[1], std::ios::binary);
    out << BuildModuleSource(pointers);
    return out ? 0 : 2;
}
