#include "MenuScreen.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <iostream>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"
#include "UserInterface.hpp"
#include "UserSettings.hpp"
#include "Utilities/FileUtils.h"

// #include "GameObjectManager.h"

MenuScreen::MenuScreen() : AUIScreen("MenuScreen")
{
	// this->openSceneDialog = new ImGui::FileBrowser();
	// this->openSceneDialog->SetTitle("Open Scene");
	// this->openSceneDialog->SetTypeFilters({ ".iet"});
	//
	// this->saveSceneDialog = new ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename);
	// this->saveSceneDialog->SetTitle("Save Scene");
	// this->saveSceneDialog->SetTypeFilters({ ".iet" });
}

MenuScreen::~MenuScreen()
{
	// delete this->openSceneDialog;
	// delete this->saveSceneDialog;
}

void MenuScreen::drawUI()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
			{
				//GameObjectManager::get()->applyAction(ActionHistory::get()->undoAction());
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y"))
			{
				//GameObjectManager::get()->applyAction(ActionHistory::get()->redoAction());
			}
			if (ImGui::MenuItem("Open..", "Ctrl+O")) {
				//this->openSceneDialog->Open();
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				//this->saveSceneDialog->Open();
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
				//this->saveSceneDialog->Open();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit Editor", "Ctrl+W"))
			{
				// Walnut::Application::Get().Close();
				exit(0); // temp exit lol 
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::MenuItem("Refresh Scene", "F5")) { EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); }
			if (ImGui::MenuItem("Load Sphere World")) { this->OnLoadSphereWorld(); }
			if (ImGui::MenuItem("Load Box World")) { this->OnLoadBoxWorld(); }
			if (ImGui::MenuItem("Load Cornell Box")) { this->OnLoadCornellBox(); }
			if (ImGui::MenuItem("Load AnitoTracer Demo")) { this->OnLoadAnitoTracerDemo(); }
			if (ImGui::MenuItem("Load Sponza Scene")) { this->OnLoadSponza(); }
			if (ImGui::MenuItem("Delete All Objects in Current Scene")) { this->OnLoadEmpty(); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Game Object")) {
			if (ImGui::MenuItem("Create Sphere")) { this->OnCreateSphereClicked(); }
			if (ImGui::MenuItem("Create Cube")) { this->OnCreateCubeClicked(); }
			if (ImGui::MenuItem("Create Capsule")) { onCreateCapsuleClicked(); }
			if (ImGui::MenuItem("Create Cylinder")) { onCreateCylinderClicked(); }
			//if (ImGui::MenuItem("Create Textured Cube")) { this->OnCreateTexturedCubeClicked(); } // add texture component
			if (ImGui::MenuItem("Create Plane")) { this->OnCreatePlaneClicked(); }
			if (ImGui::MenuItem("Create Game Object From File...", nullptr, isLoadObjOpen))
			{
				isLoadObjOpen = !isLoadObjOpen;
			}
			if (ImGui::MenuItem("Create Game Object Group From File...", nullptr, isLoadSceneOpen))
			{
				isLoadSceneOpen = !isLoadSceneOpen;
			}

			if (ImGui::BeginMenu("Light")) {
				if (ImGui::MenuItem("Point Light")) { OnCreateLightClicked(Light::PointLight); }
				if (ImGui::MenuItem("Directional Light")) { OnCreateLightClicked(Light::DirectionalLight); }
				if (ImGui::MenuItem("Spot Light")) { OnCreateLightClicked(Light::SpotLight); }
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Components")) {
			if (ImGui::MenuItem("Material")) { this->OnMaterialComponentClicked(); }
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Editor Settings", nullptr, UIManager::getInstance()->getEnabled(UINames::SETTINGS_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::SETTINGS_SCREEN);
			}
			if (ImGui::MenuItem("Inspector", nullptr, UIManager::getInstance()->getEnabled(UINames::INSPECTOR_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::INSPECTOR_SCREEN);
			}
			if (ImGui::MenuItem("Hierarchy", nullptr, UIManager::getInstance()->getEnabled(UINames::HIERARCHY_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::HIERARCHY_SCREEN);
			}
			if (ImGui::MenuItem("Profiler", nullptr, UIManager::getInstance()->getEnabled(UINames::PROFILER_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::PROFILER_SCREEN);
			}
			if (ImGui::MenuItem("Debug Console", nullptr, UIManager::getInstance()->getEnabled(UINames::CONSOLE_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::CONSOLE_SCREEN);
			}
			if (ImGui::MenuItem("Playback Options", nullptr, UIManager::getInstance()->getEnabled(UINames::PLAYBACK_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::PLAYBACK_SCREEN);
			}
			if (ImGui::BeginMenu("Viewport"))
			{
				// if (ImGui::MenuItem("Create Viewport"))
				// {
				// 	ViewportManager::get()->createViewport();
				// }
				// if (ImGui::MenuItem("Single Viewport"))
				// {
				// 	ViewportManager::get()->setNumViewports(1);
				// }
				// if (ImGui::MenuItem("2 Viewports"))
				// {
				// 	ViewportManager::get()->setNumViewports(2);
				// }
				// if (ImGui::MenuItem("3 Viewports"))
				// {
				// 	ViewportManager::get()->setNumViewports(3);
				// }
				// if (ImGui::MenuItem("4 Viewports"))
				// {
				// 	ViewportManager::get()->setNumViewports(4);
				// }
				// if (ImGui::MenuItem("Delete All Viewports"))
				// {
				// 	ViewportManager::get()->deleteAllViewports();
				// }
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Material Editor", nullptr, UIManager::getInstance()->getEnabled(UINames::MATERIAL_EDITOR_SCREEN)))
			{
				UIManager::getInstance()->toggleEnabled(UINames::MATERIAL_EDITOR_SCREEN);
			}

			if (ImGui::MenuItem("Color Picker", nullptr, isColorPickerOpen))
			{
				isColorPickerOpen = !isColorPickerOpen;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Toggle All Tool Windows", "F3"))
			{
				UIManager::getInstance()->toggleAllUI();
			}

			ImGui::EndMenu();
		}

		if (isLoadObjOpen)
			ShowLoadObjMenu();
		if (isLoadSceneOpen)
			ShowLoadObjMenu();
		if (isColorPickerOpen)
			ShowColorPickerWindow();

		ImGui::EndMainMenuBar();
	}

	// this->openSceneDialog->Display();
	// this->saveSceneDialog->Display();
	//
	// if (this->saveSceneDialog->HasSelected())
	// {
	// 	SceneWriter writer = SceneWriter(this->saveSceneDialog->GetSelected().string());
	// 	writer.writeToFile();
	//
	// 	this->saveSceneDialog->ClearSelected();
	// 	this->saveSceneDialog->Close();
	// }
	//
	// else if (this->openSceneDialog->HasSelected()) {
	// 	SceneReader reader = SceneReader(this->openSceneDialog->GetSelected().string());
	// 	reader.readFromFile();
	//
	// 	this->openSceneDialog->ClearSelected();
	// 	this->openSceneDialog->Close();
	// }
}

void MenuScreen::OnCreateCubeClicked()
{
	//initialize vertex for object
	// GameObjectManager::getInstance()->createObject(AGameObject::PrimitiveType::CUBE);
	ModelManager::getInstance()->createObject(GameObject::PrimitiveType::CUBE);
}

void MenuScreen::OnCreateTexturedCubeClicked()
{
	// GameObjectManager::getInstance()->createObject(AGameObject::PrimitiveType::TEXTURED_CUBE);
}

void MenuScreen::OnCreateSphereClicked()
{
	//std::cout << "Creating sphere placeholder. \n";
	ModelManager::getInstance()->createObject(GameObject::PrimitiveType::SPHERE);
}

void MenuScreen::onCreateCapsuleClicked()
{
	ModelManager::getInstance()->createObject(GameObject::PrimitiveType::CAPSULE);
}

void MenuScreen::onCreateCylinderClicked()
{
	ModelManager::getInstance()->createObject(GameObject::PrimitiveType::CYLINDER);
}

void MenuScreen::ShowLoadObjMenu()
{
	ImGui::SetNextWindowSize(ImVec2(500, 200));

	if (ImGui::Begin("Create GameObject from File", &isLoadObjOpen))
	{
		static std::string name = "GameObject";
		//GameObject::PrimitiveType type;
		static float position[3] = { 0, 0, 0 };
		static float rotation[3] = { 0, 0, 0 };
		static float scale[3] = { 1, 1, 1 };

		ImGui::Text("Spawn with the following attributes: ");
		ImGui::InputTextWithHint("GameObject Name", "Name...", &name);
		//ImGui::SameLine();
		ImGui::InputFloat3("Position", position);
		//ImGui::SameLine();
		ImGui::InputFloat3("Rotation", rotation);
		//ImGui::SameLine();	
		ImGui::InputFloat3("Scale", scale);
		ImGui::Separator();


		if (ImGui::Button("Select File...", ImVec2(150, 25)))
		{

			ModelManager::getInstance()->createObjectFromFile(
				name,
				GameObject::PrimitiveType::CUBE,
				glm::vec3(position[0], position[1], position[2]),
				glm::vec3(rotation[0], rotation[1], rotation[2]),
				glm::vec3(scale[0], scale[1], scale[2])
			);

		}
	}

	else if (ImGui::Begin("Create GameObject Group from File", &isLoadSceneOpen))
	{
		static std::string name = "GameObject";
		//GameObject::PrimitiveType type;
		static float position[3] = { 0, 0, 0 };
		static float rotation[3] = { 0, 0, 0 };
		static float scale[3] = { 1, 1, 1 };

		ImGui::Text("Spawn with the following attributes: ");
		ImGui::InputTextWithHint("GameObject Name", "Name...", &name);
		//ImGui::SameLine();
		ImGui::InputFloat3("Position", position);
		//ImGui::SameLine();
		ImGui::InputFloat3("Rotation", rotation);
		//ImGui::SameLine();	
		ImGui::InputFloat3("Scale", scale);
		ImGui::Separator();


		if (ImGui::Button("Select File...", ImVec2(150, 25)))
		{

			ModelManager::getInstance()->createObjectGroupFromFile(
				name,
				GameObject::PrimitiveType::CUBE,
				glm::vec3(position[0], position[1], position[2]),
				glm::vec3(rotation[0], rotation[1], rotation[2]),
				glm::vec3(scale[0], scale[1], scale[2])
			);

		}
	}

	ImGui::End();
}

void MenuScreen::OnCreatePlaneClicked()
{
	//initialize vertex for object
	ModelManager::getInstance()->createObject(GameObject::PLANE);
	// GameObjectManager::getInstance()->createObject(AGameObject::PrimitiveType::QUAD);
}

void MenuScreen::OnCreateLightClicked(Light::LightType type)
{
	switch (type)
	{
	case Light::PointLight:
		ModelManager::getInstance()->createObject(GameObject::POINT_LIGHT);
		break;
	case Light::DirectionalLight:
		ModelManager::getInstance()->createObject(GameObject::DIRECTIONAL_LIGHT);
		break;
	case Light::SpotLight:
		ModelManager::getInstance()->createObject(GameObject::SPOT_LIGHT);
		break;
	}
}

void MenuScreen::OnMaterialComponentClicked()
{
	// Debug::Log("Creating material placeholder.");
}

void MenuScreen::OnLoadSphereWorld()
{
	// GameObjectManager::getInstance()->clearAll();
	// RayTracingProper::getInstance()->generateSphereWorld();
	// RayTracingProper::getInstance()->renderSceneFromHierarchy();

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 6);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadBoxWorld()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 8);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadCornellBox()
{
	// GameObjectManager::getInstance()->clearAll();
	// RayTracingProper::getInstance()->generateCornellBox();
	//
	// RayTracingProper::getInstance()->renderSceneFromHierarchy();

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 7);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadAnitoTracerDemo()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 9);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadSponza()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 10);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadEmpty()
{
	ModelManager::getInstance()->clearAllObjects();
	// std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	// parameters->encodeInt("SCENE_INDEX", 11);
	// EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::ShowColorPickerWindow()
{
	if (ImGui::Begin("Color Picker", &isColorPickerOpen))
	{
		static ImVec4 color(1.0f, 0.0f, 1.0f, 0.5f);
		ImGui::SameLine();
		ImGui::ColorPicker4("MyColor##4", reinterpret_cast<float*>(&color), 0);
	}
	ImGui::End();
}
