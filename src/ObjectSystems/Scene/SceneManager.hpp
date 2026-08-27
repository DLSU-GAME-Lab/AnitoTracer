#include <string>

#include "HierarchyManager.hpp"
#include "ProjectLoader.hpp"

class SceneManager {
public:
    static SceneManager& GetInstance() {
        static SceneManager instance;
        return instance;
    }

    // Static function callable from anywhere in your codebase
    static void RequestSceneChange(const std::string& filepath) {
        GetInstance().QueueSceneChange(filepath);
    }

    // Call this specifically during the designated window in your engine loop
    void ProcessPendingSceneChange() {
        if (!m_hasPendingChange) {
            return;
        }

        // Route through HierarchyManager::LoadScene so post-load guarantees are applied.
        HierarchyManager::GetInstance().LoadScene(
            ProjectLoader::GetAbsolutePath(m_targetScene)
        );

        // Clear the state after processing
        m_targetScene.clear();
        m_hasPendingChange = false;
    }

    // Query if a scene load is currently queued
    bool HasPendingChange() const {
        return m_hasPendingChange;
    }

private:
    SceneManager() = default;
    ~SceneManager() = default;

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    void QueueSceneChange(const std::string& filepath) {
        m_targetScene = filepath;
        m_hasPendingChange = true;
    }

    std::string m_targetScene;
    bool m_hasPendingChange = false;
};