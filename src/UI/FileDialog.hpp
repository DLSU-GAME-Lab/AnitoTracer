#pragma once

#include <string>

namespace Diligent {

    class FileDialog
    {
    public:
        // Opens a native file dialog and returns the absolute path to the selected file.
        // Returns an empty string if the user cancels or an error occurs.
        static std::string OpenModelFile();
    };

}