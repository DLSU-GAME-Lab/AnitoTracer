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

#if PLATFORM_WIN32
    if (app.Initialize(hInstance, nCmdShow))
#else
    if (app.Initialize(nullptr, 0)) // Adjust according to platform needs
#endif
    {
        app.Run();
    }

    app.Shutdown();
    return 0;
}