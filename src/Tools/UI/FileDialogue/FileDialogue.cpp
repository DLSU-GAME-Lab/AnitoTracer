#include "FileDialogue.hpp"

#include <nfd.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

void gbe::FileDialogue::Init()
{
	static bool initialized = false;
	if (initialized)
		return;
	initialized = (NFD_Init() == NFD_OKAY);
}

std::string gbe::FileDialogue::GetFilePath(OpType optype, std::string extension)
{
	std::string outPathStr = "";

	if (!extension.empty() && extension[0] == '.')
		extension.erase(0, 1);

	nfdu8filteritem_t filter[1] = { { "Files", extension.c_str() } };
	const nfdu8filteritem_t* filterList = extension.empty() ? nullptr : filter;
	const nfdfiltersize_t filterCount = extension.empty() ? 0 : 1;

	nfdwindowhandle_t parent = {};
#ifdef _WIN32
	if (HWND hwnd = GetActiveWindow()) {
		parent.type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
		parent.handle = hwnd;
	}
#endif

	nfdu8char_t* outPath = nullptr;
	nfdresult_t result = NFD_ERROR;

	switch (optype)
	{
	case gbe::FileDialogue::OPEN: {
		nfdopendialogu8args_t args = {};
		args.filterList = filterList;
		args.filterCount = filterCount;
		args.parentWindow = parent;
		result = NFD_OpenDialogU8_With(&outPath, &args);
		break;
	}
	case gbe::FileDialogue::SAVE: {
		nfdsavedialogu8args_t args = {};
		args.filterList = filterList;
		args.filterCount = filterCount;
		args.parentWindow = parent;
		result = NFD_SaveDialogU8_With(&outPath, &args);
		break;
	}
	case gbe::FileDialogue::FOLDER: {
		nfdpickfolderu8args_t args = {};
		args.parentWindow = parent;
		result = NFD_PickFolderU8_With(&outPath, &args);
		break;
	}
	}

	if (result == NFD_OKAY && outPath) {
		outPathStr = outPath;
		NFD_FreePathU8(outPath);
	}

	if (!outPathStr.empty() && !extension.empty())
	{
		if (outPathStr.ends_with(extension) == false)
			return "";
	}

	return outPathStr;
}
