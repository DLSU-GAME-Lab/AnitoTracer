#include "AnitoTracer_Rebuild.h"
#include "src/AnitoTracer_App.hpp"

#if PLATFORM_WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    AnitoTracer_App app;

#ifdef _DEBUG
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif

    std::vector<std::string> args = {};

#if PLATFORM_WIN32
    args = std::vector<std::string>(__argv, __argv + __argc);
    if (app.Initialize(static_cast<void*>(hInstance), nCmdShow, args))
#else
    args = std::vector<std::string>(argv, argv + argc);
    if (app.Initialize(nullptr, 0, args))
#endif
    {
        app.Run();
    }

    app.Shutdown();
    return 0;
}