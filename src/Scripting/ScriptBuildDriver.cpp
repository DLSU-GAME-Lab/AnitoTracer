#include "ScriptBuildDriver.hpp"

#include "ScriptBuildConfig.hpp"
#include "ScriptModule.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

std::wstring Quote(const std::filesystem::path& path) {
    return L"\"" + path.wstring() + L"\"";
}

std::filesystem::path FindVcVars(const std::filesystem::path& compiler) {
    auto vcRoot = compiler.parent_path();
    for (int i = 0; i < 6; ++i) vcRoot = vcRoot.parent_path();
    return vcRoot / L"Auxiliary" / L"Build" / L"vcvars64.bat";
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::stringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

} // namespace

ScriptBuildDriver& ScriptBuildDriver::GetInstance() {
    static ScriptBuildDriver instance;
    return instance;
}

ScriptBuildDriver::~ScriptBuildDriver() {
    if (m_worker.joinable()) m_worker.join();
}

void ScriptBuildDriver::SetProjectDirectory(const std::filesystem::path& projectDirectory) {
    if (projectDirectory.empty()) {
        m_sourcePath.clear();
        m_outputPath.clear();
        m_logPath.clear();
        m_observedWrite = {};
        m_builtWrite = {};
        m_changedAt = {};
        return;
    }

    const auto scripts = projectDirectory / "Library" / "Scripts";
    const auto source = scripts / "Scripts.gen.cpp";
    if (source == m_sourcePath) return;

    if (m_worker.joinable()) {
        if (m_running.load()) return;
        m_worker.join();
    }
    m_sourcePath = source;
    m_outputPath = scripts / "Scripts.dll";
    m_logPath = scripts / "Scripts.build.log";
    m_observedWrite = {};
    m_builtWrite = {};
    m_changedAt = {};
}

void ScriptBuildDriver::Poll() {
    using namespace std::chrono_literals;
    const auto now = std::chrono::steady_clock::now();
    if (m_lastPoll != std::chrono::steady_clock::time_point{} && now - m_lastPoll < 200ms) return;
    m_lastPoll = now;

    if (m_running.load()) return;
    if (m_worker.joinable()) FinishBuild();
    if (m_sourcePath.empty()) return;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(m_sourcePath, ec)) return;
    const auto writeTime = std::filesystem::last_write_time(m_sourcePath, ec);
    if (ec || writeTime == m_builtWrite) return;

    if (writeTime != m_observedWrite) {
        m_observedWrite = writeTime;
        m_changedAt = now;
        return;
    }
    if (now - m_changedAt < 700ms) return;

    StartBuild(m_sourcePath, m_outputPath, m_logPath, writeTime);
}

void ScriptBuildDriver::StartBuild(const std::filesystem::path& source, const std::filesystem::path& output,
                                  const std::filesystem::path& log, std::filesystem::file_time_type sourceTime) {
    std::error_code ec;
    std::filesystem::create_directories(output.parent_path(), ec);
    const std::filesystem::path compiler = ANITO_SCRIPT_COMPILER;
    const std::filesystem::path vcvars = FindVcVars(compiler);
    if (!std::filesystem::is_regular_file(vcvars, ec)) {
        std::cerr << "[Script] MSVC environment script not found: " << vcvars.string() << std::endl;
        m_builtWrite = sourceTime;
        return;
    }

    std::wstring command = L"call " + Quote(vcvars) + L" >nul && " + Quote(compiler) +
        L" /nologo /LD /std:c++20 /EHsc /W4 /INCREMENTAL:NO ";
#ifdef _DEBUG
    command += L"/MDd ";
#else
    command += L"/MD ";
#endif
    command += L"/I" + Quote(ANITO_SCRIPT_SDK_INCLUDE) + L" " + Quote(source) +
        L" /link /OUT:" + Quote(output) + L" /PDB:" + Quote(output.parent_path() / L"Scripts.pdb") +
        L" /PDBALTPATH:Scripts.pdb";

    std::wstring fullCommand = L"cmd.exe /d /s /c \"" + command + L" > " + Quote(log) + L" 2>&1\"";
    std::vector<wchar_t> mutableCommand(fullCommand.begin(), fullCommand.end());
    mutableCommand.push_back(L'\0');

    m_running.store(true);
    std::cerr << "[Script] Building " << output.string() << std::endl;
    m_worker = std::thread([this, sourceTime, command = std::move(mutableCommand)]() mutable {
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        const BOOL created = CreateProcessW(L"C:\\Windows\\System32\\cmd.exe", command.data(), nullptr, nullptr,
                                             FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
        if (!created) {
            m_buildSucceeded = false;
            m_builtWrite = sourceTime;
            std::cerr << "[Script] Could not start MSVC build (" << GetLastError() << ")." << std::endl;
        } else {
            WaitForSingleObject(process.hProcess, INFINITE);
            DWORD exitCode = 1;
            GetExitCodeProcess(process.hProcess, &exitCode);
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
            m_buildSucceeded = exitCode == 0;
            m_builtWrite = sourceTime;
        }
        m_running.store(false);
    });
}

void ScriptBuildDriver::FinishBuild() {
    m_worker.join();
    const std::string output = ReadText(m_logPath);
    if (!output.empty()) std::cerr << output;
    if (m_buildSucceeded) {
        std::cerr << "[Script] Build succeeded; DLL will reload automatically." << std::endl;
        ScriptModule::GetInstance().Poll();
    } else {
        std::cerr << "[Script] Build failed; keeping the last working DLL." << std::endl;
    }
}
