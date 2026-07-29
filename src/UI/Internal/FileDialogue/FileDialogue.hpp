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

		static std::string GetFilePath(OpType optype, std::string extension = "");
	};

	inline static bool FileDialogueInit = []() {
		FileDialogue::Init();
		return true;
		}();
}