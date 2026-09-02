#include "CreateInstance.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>
#include <iostream>

namespace gbe
{
    void CreateInstance(const std::string& args) {
#ifdef _WIN32
        // 1. Get the path to the current engine executable
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);

        // 2. Format the command line arguments
        // Wrapping paths in quotes handles spaces in directory names
        std::string cmdLine = std::string("\"") + exePath + "\"" + args;

        // CreateProcess requires a mutable string buffer
        std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back('\0');

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // 3. Spawn the detached process
        // CREATE_NEW_CONSOLE opens a new terminal window for the game, useful for debugging
        if (CreateProcessA(
            NULL,                   // Application name (use command line instead)
            cmdBuffer.data(),       // Command line arguments
            NULL,                   // Process handle not inheritable
            NULL,                   // Thread handle not inheritable
            FALSE,                  // Set handle inheritance to FALSE
            CREATE_NEW_CONSOLE,     // Creation flags
            NULL,                   // Use parent's environment block
            NULL,                   // Use parent's starting directory 
            &si,                    // Pointer to STARTUPINFO structure
            &pi                     // Pointer to PROCESS_INFORMATION structure
        )) {
            // Close handles to avoid memory leaks in the editor
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            std::cout << "Launched Game Instance: " << cmdLine << std::endl;
        }
        else {
            std::cerr << "Failed to launch game instance. Error: " << GetLastError() << std::endl;
        }
#else
        // Linux/macOS implementation using fork() and exec() would go here
        std::cerr << "LaunchGameInstance not implemented for this OS." << std::endl;
#endif
    }
}