#include "MenuBar.hpp"

#include "HierarchyManager.hpp"
#include "ProjectLoader.hpp"

#include "FileDialogue.hpp"
#include "RendererManager.hpp"

#include "CreateInstance.hpp"

namespace Diligent {

    void MenuBar::Draw(bool& appRunning, const std::vector<std::unique_ptr<BasePanel>>& panels)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    appRunning = false;
                }
                if (ImGui::MenuItem("Load Project", "Alt+F4"))
                {
                    std::string outPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::OPEN, "aproject");
                    ProjectLoader::LoadProject(outPath);
                }
                if (ImGui::MenuItem("Quick Save", "Ctrl+S", false, !ProjectLoader::GetCurrentSceneFile().empty()))
                {
                    ProjectLoader::QuickSave();
                }
                if (ImGui::MenuItem("Save Scene As..."))
                {
                    std::string outPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::SAVE);
                    ProjectLoader::SaveSceneAs(outPath);
                }
                if (ImGui::MenuItem("Load Scene", "Alt+F4"))
                {
                    std::string outPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::OPEN, "ascene");
                    ProjectLoader::RequestSceneLoad(outPath);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Add Empty Object"))
                {
                    ObjectFactory::GetInstance().CreateRootObjectWithTransform("Empty Object");
                }

                if (ImGui::BeginMenu("Primmitives")) {
                    if (ImGui::MenuItem("Box")) {
                        ObjectFactory::GetInstance().CreateCubePrimitive("Box");
                    }
                    if (ImGui::MenuItem("Sphere")) {
                        ObjectFactory::GetInstance().CreateSpherePrimitive("Sphere");
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Lights")) {
                    if (ImGui::MenuItem("Direction")) {
                        ObjectFactory::GetInstance().CreateDirectionalLightObject("Direction Light");
                    }
                    if (ImGui::MenuItem("Point")) {
                        ObjectFactory::GetInstance().CreatePointLightObject("Point Light");
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Load Model"))
                {
                    std::string filepath = FileDialog::OpenModelFile();

                    if (!filepath.empty())
                    {
                        std::cout << "Successfully selected model: " << filepath << std::endl;
                        ObjectFactory::GetInstance().CreateModelObject("Loaded Model", filepath);
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                bool rtSupported = RendererManager::GetInstance().IsRayTracingSupported();

                if (ImGui::MenuItem("Basic Lit Pipeline"))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(RendererChangeArgs::BASIC_LIT)
                    );
                }

                if (ImGui::MenuItem("Deferred Pipeline"))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(RendererChangeArgs::DEFERRED)
                    );
                }

                if (ImGui::MenuItem("Hybrid Pipeline (RayTraced)", nullptr, false, rtSupported))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(RendererChangeArgs::HYBRID)
                    );
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows"))
            {
                for (const auto& panel : panels)
                {
                    ImGui::MenuItem(panel->GetName().c_str(), NULL, &panel->GetVisible());
                }
                ImGui::EndMenu();
            }
            
            // Dynamically populate the Windows menu based on registered panels
            if (ImGui::BeginMenu("Launch"))
            {
                if (ImGui::MenuItem("Play Project"))
                {
                    gbe::CreateInstance(" -release --project \"" + ProjectLoader::GetCurrentProjectFile().string() + "\"");
                }
                if (ImGui::MenuItem("Play Scene"))
                {
                    gbe::CreateInstance(" -release --project \"" + ProjectLoader::GetCurrentProjectFile().string() + "\"" + " --scene \"" + HierarchyManager::GetInstance().GetSceneFile().string() + "\"");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) &&
            !ImGui::GetIO().WantTextInput)
        {
            ProjectLoader::QuickSave();
        }

        // TODO: Move scene-load confirmation into a reusable modal/service so non-menu callers
        // can request guarded loads without depending on MenuBar rendering.
        if (ProjectLoader::HasPendingSceneLoad())
        {
            ImGui::OpenPopup("Unsaved Scene Changes");
        }
        if (ImGui::BeginPopupModal("Unsaved Scene Changes", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("The current scene has unsaved changes.");
            ImGui::TextWrapped("Load %s anyway?", ProjectLoader::GetPendingSceneFile().filename().string().c_str());
            if (ImGui::Button("Save and Load"))
            {
                ProjectLoader::ResolvePendingSceneLoad(true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Without Saving"))
            {
                ProjectLoader::ResolvePendingSceneLoad(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ProjectLoader::CancelPendingSceneLoad();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}