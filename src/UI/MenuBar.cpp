#include "MenuBar.hpp"

#include "HierarchyManager.hpp"
#include "ProjectLoader.hpp"

#include "FileDialogue.hpp"
#include "RendererManager.hpp"

#include "CreateInstance.hpp"
#include "../AppState.hpp"
#include "GUIManager.hpp"

#include "Event/EventSystem.hpp"

namespace Diligent {

    void MenuBar::Draw(bool& appRunning, const std::vector<std::unique_ptr<BasePanel>>& panels)
    {
        bool openSaveLayoutPopup = false;
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    appRunning = false;
                }
                if (ImGui::MenuItem("Create New Scene", "Ctrl+N"))
                {
                    ProjectLoader::CreateNewScene();
                }
                if (ImGui::MenuItem("Create Project"))
                {
                    const std::string projectPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::FOLDER);
                    ProjectLoader::RequestCreateProject(projectPath);
                }
                if (ImGui::MenuItem("Load Project", "Alt+F4"))
                {
                    std::string outPath = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::OPEN, "aproject");
                    ProjectLoader::LoadProject(outPath);
                }
                if (ImGui::MenuItem("Quick Save", "Ctrl+S", false, ProjectLoader::CanQuickSave()))
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

            if (!AppState::isReleaseBuild)
            {
                ImGui::Separator();
                if (!AppState::isPlaying)
                {
                    if (ImGui::MenuItem("Play"))
                    {
                        gbe::EventSystem::DispatchTo(EVENT_ON_APP_PLAY, std::make_unique<gbe::EventArgs>());
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Stop"))
                    {
                        gbe::EventSystem::DispatchTo(EVENT_ON_APP_STOPPLAY, std::make_unique<gbe::EventArgs>());
                    }
                }
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, HierarchyManager::GetInstance().CanUndo()))
                {
                    HierarchyManager::GetInstance().Undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, HierarchyManager::GetInstance().CanRedo()))
                {
                    HierarchyManager::GetInstance().Redo();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                bool rtSupported = RendererManager::GetInstance().IsRayTracingSupported();
                auto currentRenderer = UserSettings::GetInstance().GetRendererType();

                if (ImGui::MenuItem("Basic Lit Pipeline", nullptr, currentRenderer == Diligent::PipelineType::BASIC_LIT))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(Diligent::PipelineType::BASIC_LIT)
                    );
                }

                if (ImGui::MenuItem("Deferred Pipeline", nullptr, currentRenderer == Diligent::PipelineType::DEFERRED))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(Diligent::PipelineType::DEFERRED)
                    );
                }

                if (ImGui::MenuItem("Hybrid Pipeline (RayTraced)", nullptr, currentRenderer == Diligent::PipelineType::HYBRID, rtSupported))
                {
                    gbe::EventSystem::DispatchTo(
                        EVENT_RENDER_CHANGE,
                        std::make_unique<RendererChangeArgs>(Diligent::PipelineType::HYBRID)
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
                ImGui::Separator();
                if (ImGui::BeginMenu("Layouts"))
                {
                    auto& gui = GUIManager::GetInstance();
                    const auto layouts = gui.GetAvailableLayouts();
                    for (const auto& layout : layouts)
                    {
                        if (ImGui::MenuItem(layout.c_str()))
                            gui.RequestLoadLayout(layout);
                    }
                    if (layouts.empty())
                        ImGui::TextDisabled("No layouts found");
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save Current Layout As..."))
                        openSaveLayoutPopup = true;
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            
            // Dynamically populate the Windows menu based on registered panels
            if (ImGui::BeginMenu("Launch"))
            {
                std::string default_args = std::string(" -release --project ") + 
                "\"" + ProjectLoader::GetCurrentProjectFile().string() + "\"" +
                " --renderer " + std::to_string(static_cast<int>(UserSettings::GetInstance().GetRendererType()));

                if (ImGui::MenuItem("Play Project"))
                {
                    gbe::CreateInstance(default_args);
                }
                if (ImGui::MenuItem("Play Scene"))
                {
                    gbe::CreateInstance(default_args + " --scene \"" + HierarchyManager::GetInstance().GetSceneFile().string() + "\"");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        static char layoutName[64] = "";
        if (openSaveLayoutPopup)
        {
            layoutName[0] = '\0';
            ImGui::OpenPopup("Save Layout");
        }
        if (ImGui::BeginPopupModal("Save Layout", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Layout name:");
            if (openSaveLayoutPopup || ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            bool submit = ImGui::InputText("##layoutName", layoutName, sizeof(layoutName), ImGuiInputTextFlags_EnterReturnsTrue);
            if (ImGui::Button("Save") || submit)
            {
                if (GUIManager::GetInstance().SaveCurrentLayout(layoutName))
                    ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) &&
            !ImGui::GetIO().WantTextInput)
        {
            ProjectLoader::QuickSave();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N) &&
            !ImGui::GetIO().WantTextInput)
        {
            ProjectLoader::CreateNewScene();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) &&
            !ImGui::GetIO().WantTextInput)
        {
            HierarchyManager::GetInstance().Undo();
        }

        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) &&
            !ImGui::GetIO().WantTextInput)
        {
            HierarchyManager::GetInstance().Redo();
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
            if (ProjectLoader::IsPendingNewScene())
            {
                ImGui::TextWrapped("Create a new blank scene anyway?");
            }
            else
            {
                ImGui::TextWrapped("Load %s anyway?", ProjectLoader::GetPendingSceneFile().filename().string().c_str());
            }
            if (ImGui::Button("Save and Continue"))
            {
                ProjectLoader::ResolvePendingSceneLoad(true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Continue Without Saving"))
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

        if (ProjectLoader::HasPendingProjectCreation())
        {
            ImGui::OpenPopup("Non-Empty Project Folder");
        }
        if (ImGui::BeginPopupModal("Non-Empty Project Folder", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("The selected folder is not empty.");
            ImGui::TextWrapped("Create the project files here anyway?");
            if (ImGui::Button("Proceed"))
            {
                ProjectLoader::ResolvePendingProjectCreation(true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ProjectLoader::ResolvePendingProjectCreation(false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}