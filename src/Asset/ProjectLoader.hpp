#pragma once


#include <filesystem>
#include "File/Parser.hpp"
#include <fstream>
#include <string>

#include "AssetPipeline.hpp"
#include "FileDialogue.hpp"
#include "HierarchyManager.hpp"
#include "ProjectSettings.hpp"

#include <iostream>

class ProjectLoader {
	struct ProjectInfo {
		std::string entryscene;
		ProjectSettings settings;
	};

	inline static std::filesystem::path currentProjectDir;
	inline static std::filesystem::path currentSceneFile;
	inline static std::filesystem::path currentProjectFile;
	inline static std::string projectEntryScene;
	inline static ProjectSettings projectSettings;
	// TODO: Replace static loader state with an instance/service that owns the active project session.
	inline static std::filesystem::path pendingSceneFile;
	inline static bool pendingNewScene = false;
	inline static std::filesystem::path pendingProjectDirectory;
	inline static gbe::SerializedData savedSceneData;
	inline static bool hasSavedSceneData = false;

public:
	inline static std::filesystem::path GetCurrentProjectDir() { return currentProjectDir; }
	inline static std::filesystem::path GetCurrentSceneFile() { return currentSceneFile; }
	inline static std::filesystem::path GetCurrentProjectFile() { return currentProjectFile; }
	inline static ProjectSettings& GetProjectSettings() { return projectSettings; }
	inline static bool CanQuickSave() { return !currentSceneFile.empty(); }

	inline static bool CreateProject(const std::filesystem::path& projectDirectory) {
		if (projectDirectory.empty() || !std::filesystem::is_directory(projectDirectory)) {
			return false;
		}
		if (!currentProjectFile.empty() && !SaveProject()) {
			return false;
		}

		const std::string projectName = projectDirectory.filename().string().empty()
			? "Project"
			: projectDirectory.filename().string();
		const auto projectFile = projectDirectory / (projectName + ".aproject");
		const auto sceneFile = projectDirectory / "Main.ascene";

		ProjectInfo projectInfo;
		projectInfo.entryscene = sceneFile.filename().string();
		projectInfo.settings = ProjectSettings{};
		if (!WriteProjectInfo(projectFile, projectInfo)) {
			return false;
		}

		CreateNewSceneNow();
		HierarchyManager::GetInstance().SerializeToFile(sceneFile);
		currentProjectDir = projectDirectory;
		currentSceneFile = sceneFile;
		currentProjectFile = projectFile;
		projectEntryScene = projectInfo.entryscene;
		projectSettings = projectInfo.settings;
		AssetPipeline::IncludeFolder(currentProjectDir);
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
		return true;
	}

	inline static void RequestCreateProject(const std::filesystem::path& projectDirectory) {
		if (projectDirectory.empty() || !std::filesystem::is_directory(projectDirectory)) {
			return;
		}

		if (!std::filesystem::is_empty(projectDirectory)) {
			pendingProjectDirectory = projectDirectory;
			return;
		}

		CreateProject(projectDirectory);
	}

	inline static bool HasPendingProjectCreation() { return !pendingProjectDirectory.empty(); }
	inline static std::filesystem::path GetPendingProjectDirectory() { return pendingProjectDirectory; }

	inline static void ResolvePendingProjectCreation(bool proceed) {
		const auto projectDirectory = pendingProjectDirectory;
		pendingProjectDirectory.clear();
		if (proceed) {
			CreateProject(projectDirectory);
		}
	}

	inline static std::filesystem::path GetAbsolutePath(const std::filesystem::path& relativePath) {
		return std::filesystem::absolute(currentProjectDir / relativePath);
	}

	inline static bool IsCurrentSceneDirty() {
		// TODO: Query a centralized change-tracking service instead of serializing the entire hierarchy.
		return hasSavedSceneData &&
			HierarchyManager::GetInstance().Serialize().serialized_variables !=
			savedSceneData.serialized_variables;
	}

	inline static bool QuickSave() {
		if (currentSceneFile.empty()) {
			const std::string outPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::SAVE, "ascene");
			if (outPath.empty()) {
				return false;
			}
			return SaveSceneAs(outPath);
		}
		HierarchyManager::GetInstance().QuickSave();
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
		return SaveProject();
	}

	inline static bool SaveSceneAs(const std::filesystem::path& path) {
		if (path.empty()) return false;
		currentSceneFile = std::filesystem::absolute(path).lexically_normal();
		HierarchyManager::GetInstance().SerializeToFile(currentSceneFile);
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
		return SaveProject();
	}

	inline static bool SaveProject() {
		if (currentProjectFile.empty()) {
			return true;
		}

		ProjectInfo projectInfo;
		projectInfo.entryscene = projectEntryScene;
		projectInfo.settings = projectSettings;
		return WriteProjectInfo(currentProjectFile, projectInfo);
	}

	inline static void RequestSceneLoad(const std::filesystem::path& path) {
		// TODO: Move load requests and save/discard policy into a scene-session controller.
		if (path.empty()) return;
		const auto target = path.is_absolute() ? path.lexically_normal() : GetAbsolutePath(path);
		if (target != currentSceneFile && IsCurrentSceneDirty()) {
			pendingNewScene = false;
			pendingSceneFile = target;
			return;
		}
		LoadSceneNow(target);
	}

	inline static void RequestCreateNewScene() {
		if (IsCurrentSceneDirty()) {
			pendingSceneFile.clear();
			pendingNewScene = true;
			return;
		}
		CreateNewSceneNow();
	}

	inline static void CreateNewScene() {
		RequestCreateNewScene();
	}

	inline static bool HasPendingSceneLoad() { return !pendingSceneFile.empty() || pendingNewScene; }
	inline static std::filesystem::path GetPendingSceneFile() { return pendingSceneFile; }
	inline static bool IsPendingNewScene() { return pendingNewScene; }

	inline static void ResolvePendingSceneLoad(bool saveChanges) {
		if (!HasPendingSceneLoad()) return;
		const auto target = pendingSceneFile;
		const bool shouldCreateNewScene = pendingNewScene;
		pendingSceneFile.clear();
		pendingNewScene = false;
		if (saveChanges) QuickSave();
		if (shouldCreateNewScene) {
			CreateNewSceneNow();
			return;
		}
		LoadSceneNow(target);
	}

	inline static void CancelPendingSceneLoad() {
		pendingSceneFile.clear();
		pendingNewScene = false;
	}

	static inline void LoadProject(std::filesystem::path path) {
		ProjectInfo newinfo;

		if(path.empty()) return;

		path = std::filesystem::absolute(path).lexically_normal();
		if (!gbe::Parser::PopulateClass(newinfo, path)) {
			return;
		}

		if (path != currentProjectFile && !currentProjectFile.empty() && !SaveProject()) {
			return;
		}

		currentProjectDir = path.parent_path();
		currentSceneFile = std::filesystem::absolute(currentProjectDir / newinfo.entryscene).lexically_normal();
		currentProjectFile = path;
		projectEntryScene = newinfo.entryscene;
		projectSettings = newinfo.settings;

		AssetPipeline::IncludeFolder(currentProjectDir);
		LoadSceneNow(currentSceneFile);
		
	}

private:
	inline static bool WriteProjectInfo(const std::filesystem::path& path, const ProjectInfo& projectInfo) {
		std::ofstream projectStream(path);
		if (!projectStream) {
			std::cerr << "Failed to open project file for writing: " << path << std::endl;
			return false;
		}

		projectStream << gbe::Parser::ExportClassStr(projectInfo) << '\n';
		if (!projectStream) {
			std::cerr << "Failed to write project file: " << path << std::endl;
			return false;
		}
		return true;
	}

	inline static void CreateNewSceneNow() {
		HierarchyManager::GetInstance().CreateNewScene();
		currentSceneFile.clear();
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
	}

	inline static void LoadSceneNow(const std::filesystem::path& path) {
		currentSceneFile = path.lexically_normal();
		HierarchyManager::GetInstance().LoadScene(currentSceneFile);
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
	}
};