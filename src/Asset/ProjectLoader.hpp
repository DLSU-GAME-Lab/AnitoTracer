
#include <filesystem>
#include "File/Parser.hpp"
#include <string>

#include "AssetPipeline.hpp"
#include "HierarchyManager.hpp"

class ProjectLoader {
	struct ProjectInfo {
		std::string entryscene;
	};

	inline static std::filesystem::path currentProjectDir;
	inline static std::filesystem::path currentSceneFile;
	inline static std::filesystem::path currentProjectFile;

public:
	inline static std::filesystem::path GetCurrentProjectDir() { return currentProjectDir; }
	inline static std::filesystem::path GetCurrentSceneFile() { return currentSceneFile; }
	inline static std::filesystem::path GetCurrentProjectFile() { return currentProjectFile; }

	static inline void LoadProject(std::filesystem::path path) {
		ProjectInfo newinfo;

		gbe::Parser::PopulateClass(newinfo, path);

		currentProjectDir = path.parent_path();
		currentSceneFile = currentProjectDir / newinfo.entryscene;
		currentProjectFile = path;

		AssetPipeline::IncludeFolder(currentProjectDir);
		HierarchyManager::GetInstance().DeserializeFromFile(currentSceneFile);
		
	}
};