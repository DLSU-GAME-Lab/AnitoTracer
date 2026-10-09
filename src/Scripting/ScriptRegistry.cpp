#include "ScriptRegistry.hpp"

#include "ScriptComponent.hpp"
#include "ScriptParser.hpp"
#include "ScriptTranspiler.hpp"

#include "TypeRegistry.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

ScriptRegistry& ScriptRegistry::GetInstance() {
    static ScriptRegistry instance;
    return instance;
}

void ScriptRegistry::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    gbe::TypeRegistry::Register(typeid(ScriptComponent).name(), [](gbe::SerializedData& data) {
        GBE_CREATE(newobj, ScriptComponent, ComponentBase, (data));
        return newobj;
    });
}

void ScriptRegistry::SetProjectDirectory(const std::filesystem::path& projectDir) {
    const std::filesystem::path scriptsDir = projectDir.empty() ? std::filesystem::path{} : projectDir / "Scripts";
    if (scriptsDir == m_scriptsDir) return;

    m_scriptsDir = scriptsDir;
    m_files.clear();
    m_scripts.clear();
    m_lastScan = {};
}

bool ScriptRegistry::Refresh() {
    using namespace std::chrono_literals;
    const auto now = std::chrono::steady_clock::now();
    if (m_lastScan != std::chrono::steady_clock::time_point{} && now - m_lastScan < 500ms) return false;
    m_lastScan = now;

    std::error_code ec;
    std::set<std::filesystem::path> seen;
    bool changed = false;

    if (!m_scriptsDir.empty() && std::filesystem::is_directory(m_scriptsDir, ec)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(m_scriptsDir, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".ascript") continue;

            const auto path = entry.path().lexically_normal();
            const auto writeTime = std::filesystem::last_write_time(path, ec);
            seen.insert(path);

            auto cached = m_files.find(path);
            if (cached != m_files.end() && cached->second.writeTime == writeTime) continue;

            std::ifstream stream(path);
            std::stringstream buffer;
            buffer << stream.rdbuf();

            std::vector<std::string> errors;
            FileEntry fileEntry;
            fileEntry.writeTime = writeTime;
            fileEntry.scripts = ScriptParser::Parse(buffer.str(), path, errors);
            for (auto& script : fileEntry.scripts) script.revision = ++m_revisionCounter;
            for (const auto& error : errors) std::cerr << "[Script] " << error << std::endl;
            fileEntry.errors = std::move(errors);

            m_files[path] = std::move(fileEntry);
            changed = true;
        }
    }

    for (auto it = m_files.begin(); it != m_files.end();) {
        if (seen.count(it->first) == 0) {
            it = m_files.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed) {
        m_scripts.clear();
        for (const auto& [path, file] : m_files) {
            for (const auto& script : file.scripts) {
                if (!m_scripts.emplace(script.name, script).second) {
                    std::cerr << "[Script] Duplicate component '" << script.name << "' in " << path.string() << std::endl;
                }
            }
        }
        WriteGeneratedSource();
    }
    return changed;
}

void ScriptRegistry::WriteGeneratedSource() {
    // Keep the last good output while any script has errors.
    for (const auto& [path, file] : m_files) {
        if (!file.errors.empty()) return;
    }
    if (m_scriptsDir.empty()) return;

    std::vector<const ScriptDescriptor*> scripts;
    for (const auto& [name, script] : m_scripts) {
        if (script.cppSource.empty()) return;
        scripts.push_back(&script);
    }

    const std::string source = BuildModuleSource(scripts);
    const std::filesystem::path outPath = m_scriptsDir.parent_path() / "Library" / "Scripts" / "Scripts.gen.cpp";

    std::ifstream existing(outPath, std::ios::binary);
    std::stringstream current;
    current << existing.rdbuf();
    if (existing && current.str() == source) return;

    std::error_code ec;
    std::filesystem::create_directories(outPath.parent_path(), ec);
    std::ofstream out(outPath, std::ios::binary);
    if (!out) {
        std::cerr << "[Script] Could not write " << outPath.string() << std::endl;
        return;
    }
    out << source;
    std::cerr << "[Script] Generated " << outPath.string() << std::endl;
}

const std::vector<std::string>& ScriptRegistry::GetErrors(const std::filesystem::path& path) const {
    static const std::vector<std::string> none;
    const auto it = m_files.find(path.lexically_normal());
    return it != m_files.end() ? it->second.errors : none;
}

const ScriptDescriptor* ScriptRegistry::Find(const std::string& name) const {
    const auto it = m_scripts.find(name);
    return it != m_scripts.end() ? &it->second : nullptr;
}
