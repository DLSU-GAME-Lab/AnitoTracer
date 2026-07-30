#include "FileDialogue.hpp"

#include "portable-file-dialogs.h"
#include <vector>

void gbe::FileDialogue::Init()
{
	// Portable File Dialogs does not require manual system initialization.
}

std::string gbe::FileDialogue::GetFilePath(OpType optype, std::string extension)
{
	std::string outPathStr = "";

	// Build PFD filter vector: {"Label", "wildcard_pattern"}
	std::vector<std::string> filters;
	if (!extension.empty()) {
		std::string pattern = (extension[0] == '.') ? ("*" + extension) : ("*." + extension);
		filters = { "Files (" + pattern + ")", pattern, "All Files", "*" };
	}
	else {
		filters = { "All Files", "*" };
	}

	switch (optype)
	{
	case gbe::FileDialogue::OPEN: {
		// pfd::open_file returns std::vector<std::string>
		auto selection = pfd::open_file("Open File", "", filters).result();
		if (!selection.empty()) {
			outPathStr = selection[0];
		}
		break;
	}
	case gbe::FileDialogue::SAVE: {
		// pfd::save_file returns std::string directly
		outPathStr = pfd::save_file("Save File", "", filters).result();
		break;
	}
	case gbe::FileDialogue::FOLDER: {
		// pfd::select_folder returns std::string directly
		outPathStr = pfd::select_folder("Select Folder", "").result();
		break;
	}
	}

	if (!outPathStr.empty() && !extension.empty())
	{
		if (outPathStr.ends_with(extension) == false)
			return "";
	}

	return outPathStr;
}