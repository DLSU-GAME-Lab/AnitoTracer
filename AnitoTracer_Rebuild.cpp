#include "AnitoTracer_Rebuild.h"
#include "src/AnitoTracer_App.hpp"

#if PLATFORM_WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
{
    AnitoTracer_App app;

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