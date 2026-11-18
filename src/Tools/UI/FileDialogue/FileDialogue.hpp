#pragma once

#include <typeinfo>
#include <unordered_map>
#include <string>
#include <functional>

#include <stdio.h>
#include <stdlib.h>

namespace gbe {
	class FileDialogue {
	public:
		enum OpType {
			OPEN,
			SAVE,
			FOLDER
		};

		static void Init();

		// extension filters the dialog. Several can be given as a comma separated list ("png,jpg,hdr").
		// Returns an empty string when the dialog is cancelled or the picked file has none of the extensions.
		static std::string GetFilePath(OpType optype, std::string extension = "");
	};

	inline static bool FileDialogueInit = []() {
		FileDialogue::Init();
		return true;
		}();
}