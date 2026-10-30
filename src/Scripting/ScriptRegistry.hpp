#pragma once

#include "ScriptDescriptor.hpp"

#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

// Discovers .ascript files under <project>/Scripts and exposes their component declarations.
class ScriptRegistry {
public:
    static ScriptRegistry& GetInstance();

    // Registers the generic script component type with the serialization TypeRegistry.
    void Initialize();

    // No-op when the directory is unchanged.
    void SetProjectDirectory(const std::filesystem::path& projectDir);

    // Rescans changed files (throttled). Returns true if the script set changed.
    bool Refresh();

    const ScriptDescriptor* Find(const std::string& name) const;

    // Parse errors from the last scan of the file; empty if clean or unknown.
    const std::vector<std::string>& GetErrors(const std::filesystem::path& path) const;
    const std::map<std::string, ScriptDescriptor>& GetScripts() const { return m_scripts; }

private:
    ScriptRegistry() = default;

    void WriteGeneratedSource();

    struct FileEntry {
        std::filesystem::file_time_type writeTime;
        std::vector<ScriptDescriptor> scripts;
        std::vector<std::string> errors;
    };

    std::filesystem::path m_scriptsDir;
    std::map<std::filesystem::path, FileEntry> m_files;
    std::map<std::string, ScriptDescriptor> m_scripts;
    std::chrono::steady_clock::time_point m_lastScan{};
    uint64_t m_revisionCounter = 0;
    bool m_initialized = false;
};
