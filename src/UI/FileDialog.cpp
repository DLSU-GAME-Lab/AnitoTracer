#include "FileDialog.hpp"

#if PLATFORM_WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace Diligent {

    std::string FileDialog::OpenModelFile()
    {
#if PLATFORM_WIN32
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
        ofn.lStructSize = sizeof(OPENFILENAMEA);
        // HWND can be null, but if you pass your main window handle here, it makes the dialog modal
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);

        // Filter for common 3D model formats
        ofn.lpstrFilter = "3D Models\0*.obj;*.gltf;*.glb;*.fbx\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        // Keep the working directory intact
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return std::string(ofn.lpstrFile);
        }
#endif
        // Return empty string for cancellation or non-Windows platforms
        return "";
    }

}