
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
	static inline void LoadProject(std::filesystem::path path) {
		ProjectInfo newinfo;

		gbe::Parser::PopulateClass(newinfo, path);

		currentProjectDir = path.parent_path();
		currentSceneFile = currentProjectDir / newinfo.entryscene;
		currentProjectFile = path;

		AssetPipeline::LoadFolder(currentProjectDir);
		HierarchyManager::GetInstance().DeserializeFromFile(currentSceneFile);
		
	}
};