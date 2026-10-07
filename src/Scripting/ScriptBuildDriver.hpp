#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

class ScriptBuildDriver {
public:
    static ScriptBuildDriver& GetInstance();

    void SetProjectDirectory(const std::filesystem::path& projectDirectory);
    void Poll();
    ~ScriptBuildDriver();

private:
    ScriptBuildDriver() = default;

    void StartBuild(const std::filesystem::path& source, const std::filesystem::path& output,
                    const std::filesystem::path& log, std::filesystem::file_time_type sourceTime);
    void FinishBuild();

    std::filesystem::path m_sourcePath;
    std::filesystem::path m_outputPath;
    std::filesystem::path m_logPath;
    std::filesystem::file_time_type m_observedWrite{};
    std::filesystem::file_time_type m_builtWrite{};
    std::chrono::steady_clock::time_point m_changedAt{};
    std::chrono::steady_clock::time_point m_lastPoll{};
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    bool m_buildSucceeded = false;
};
