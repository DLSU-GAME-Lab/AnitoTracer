#pragma once


#include <filesystem>
#include "File/Parser.hpp"
#include <string>

#include "AssetPipeline.hpp"
#include "FileDialogue.hpp"
#include "HierarchyManager.hpp"

class ProjectLoader {
	struct ProjectInfo {
		std::string entryscene;
	};

	inline static std::filesystem::path currentProjectDir;
	inline static std::filesystem::path currentSceneFile;
	inline static std::filesystem::path currentProjectFile;
	// TODO: Replace static loader state with an instance/service that owns the active project session.
	inline static std::filesystem::path pendingSceneFile;
	inline static bool pendingNewScene = false;
	inline static gbe::SerializedData savedSceneData;
	inline static bool hasSavedSceneData = false;

public:
	inline static std::filesystem::path GetCurrentProjectDir() { return currentProjectDir; }
	inline static std::filesystem::path GetCurrentSceneFile() { return currentSceneFile; }
	inline static std::filesystem::path GetCurrentProjectFile() { return currentProjectFile; }
	inline static bool CanQuickSave() { return !currentSceneFile.empty(); }

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
		return true;
	}

	inline static bool SaveSceneAs(const std::filesystem::path& path) {
		if (path.empty()) return false;
		currentSceneFile = std::filesystem::absolute(path).lexically_normal();
		HierarchyManager::GetInstance().SerializeToFile(currentSceneFile);
		savedSceneData = HierarchyManager::GetInstance().Serialize();
		hasSavedSceneData = true;
		return true;
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

		gbe::Parser::PopulateClass(newinfo, path);

		currentProjectDir = path.parent_path();
		currentSceneFile = std::filesystem::absolute(currentProjectDir / newinfo.entryscene).lexically_normal();
		currentProjectFile = path;

		AssetPipeline::IncludeFolder(currentProjectDir);
		LoadSceneNow(currentSceneFile);
		
	}

private:
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