#include "SceneChanger.hpp"
#include <iostream>

#include "../Scene/SceneManager.hpp"

SceneChanger::SceneChanger(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("SceneChanger", owner) {}

SceneChanger::~SceneChanger() = default;

void SceneChanger::ChangeScene() {
    std::cout << "SceneChanger: Switching to scene -> " << m_targetScene << std::endl;

    // =========================================================
    // INSERT YOUR ENGINE'S SCENE LOADING SYSTEM HERE
    // e.g., SceneManager::GetInstance().LoadScene(m_targetScene);
    // =========================================================

	SceneManager::GetInstance().RequestSceneChange(m_targetScene);
}