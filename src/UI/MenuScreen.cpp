#include "MenuScreen.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include <iostream>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "UIManager.h"
#include "UserSettings.hpp"
#include "Utilities/FileUtils.h"

#include "Assets/Material.hpp"
#include "Assets/Model.hpp"

#include "Engine/Scene/SceneIO.hpp"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/HierarchyCommands.hpp"
#include "StateManagement/ConcreteCommands/MenuBarCommands.hpp"

using namespace Assets;
using namespace glm;

MenuScreen::MenuScreen() : AUIScreen(UINames::MENU_SCREEN)
{
	// this->openSceneDialog = new ImGui::FileBrowser();
	// this->openSceneDialog->SetTitle("Open Scene");
	// this->openSceneDialog->SetTypeFilters({ ".iet"});
	//
	// this->saveSceneDialog = new ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename);
	// this->saveSceneDialog->SetTitle("Save Scene");
	// this->saveSceneDialog->SetTypeFilters({ ".iet" });
	SceneIO::getInstance()->ReadFromDirectory();
}

MenuScreen::~MenuScreen()
{
}

void MenuScreen::drawUI()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
			{
				CommandManager::getInstance()->undo();
				CommandManager::getInstance()->clearTopUndo(); // to prevent LayoutChanges on popup
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y"))
			{
				CommandManager::getInstance()->redo();
				CommandManager::getInstance()->clearTopUndo(); // to prevent LayoutChanges on popup
			}
			//if (ImGui::MenuItem("Open..", "Ctrl+O")) {
			//	//this->openSceneDialog->Open();
			//}
			//if (ImGui::MenuItem("Save", "Ctrl+S")) {
			//	//this->saveSceneDialog->Open();
			//}
			//if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
			//	//this->saveSceneDialog->Open();
			//}
			if (ImGui::MenuItem("Save Scene As", nullptr, isSaveSceneAsOpen)) 
			{ 
				isSaveSceneAsOpen = !isSaveSceneAsOpen;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit Editor", "Esc"))
			{
				// Walnut::Application::Get().Close();
				exit(0); // temp exit lol 
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene"))
		{
			if (ImGui::BeginMenu("Demo Scenes")) {
				if (ImGui::MenuItem("Load Ray Tracing In One Weekend")) { CommandManager::getInstance()->clearTopUndo();  this->OnLoadSceneByIndex(1);; ShowLoadingPopUp();  }
				if (ImGui::MenuItem("Load Cornell Box")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(7);  ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load AnitoTracer Demo")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(9);  ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Model Showcase")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(10);  ShowLoadingPopUp(); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Sample Scenes")) {
				//if (ImGui::MenuItem("Load Ray Tracing In One Weekend")) { this->OnLoadRTIOW(); ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Sponza Scene")) { CommandManager::getInstance()->clearTopUndo();  this->OnLoadSceneByIndex(11); ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load San Miguel Scene")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(12);  ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Vokselia")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(13); ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Breakfast Room")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(14); ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Salle De Bain")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(15); ShowLoadingPopUp(); }
				if (ImGui::MenuItem("Load Gallery")) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadSceneByIndex(16);  ShowLoadingPopUp(); }

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Custom Scenes"))
			{
				for (std::string name : SceneIO::getInstance()->getSceneNames())
				{
					if (ImGui::MenuItem(name.c_str())) { CommandManager::getInstance()->clearTopUndo(); this->OnLoadEmpty(); ShowLoadingPopUp(); SceneIO::getInstance()->LoadScene(name); }
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Refresh Scene", "F5")) { EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY); }
			if (ImGui::MenuItem("Delete All Objects in Current Scene")) { this->OnLoadEmpty(); }

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Objects")) 
		{
			if (ImGui::BeginMenu("Primitives")) 
			{
				if (ImGui::MenuItem("Sphere")) { CommandManager::getInstance()->clearTopUndo(); this->OnCreateSphereClicked(); }
				if (ImGui::MenuItem("Cube")) { CommandManager::getInstance()->clearTopUndo(); this->OnCreateCubeClicked(); }
				if (ImGui::MenuItem("Capsule")) { CommandManager::getInstance()->clearTopUndo(); onCreateCapsuleClicked(); }
				if (ImGui::MenuItem("Cylinder")) { CommandManager::getInstance()->clearTopUndo(); onCreateCylinderClicked(); }
				if (ImGui::MenuItem("Plane")) { CommandManager::getInstance()->clearTopUndo(); this->OnCreatePlaneClicked(); }

				if (ImGui::BeginMenu("Reflective Spheres")) 
				{
					if (ImGui::MenuItem("Reflective Sphere")) { this->OnCreateRProbe(); } // todo: fix transparent probe to be transparent instead of reflective
					//if (ImGui::MenuItem("Create Transparent Probe")) { this->OnCreateTProbe(); }
					if (ImGui::MenuItem("Metallic Sphere")) { this->OnCreateMProbe(); }
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			} 

			if (ImGui::BeginMenu("Meshes")) 
			{													// todo: add benchmark/basic meshes
				if (ImGui::MenuItem("Bunny")) { CommandManager::getInstance()->clearTopUndo(); onCreateBunnyClicked(); }
				if (ImGui::MenuItem("Teapot")) { CommandManager::getInstance()->clearTopUndo(); onCreateTeapotClicked(); }
				if (ImGui::MenuItem("Lucy")) { CommandManager::getInstance()->clearTopUndo(); onCreateLucyClicked(); }

				ImGui::Separator();
				if (ImGui::MenuItem("Import Mesh From File...", nullptr, isLoadObjOpen))
				{
					isLoadObjOpen = !isLoadObjOpen;
				}
				if (ImGui::MenuItem("Import Separated Mesh From File...", nullptr, isLoadSceneOpen))
				{
					isLoadSceneOpen = !isLoadSceneOpen;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Lights")) {
				if (ImGui::MenuItem("Point Light")) { CommandManager::getInstance()->clearTopUndo(); OnCreateLightClicked(Light::PointLight); }
				if (ImGui::MenuItem("Directional Light")) { CommandManager::getInstance()->clearTopUndo(); OnCreateLightClicked(Light::DirectionalLight); }
				if (ImGui::MenuItem("Spot Light")) { CommandManager::getInstance()->clearTopUndo(); OnCreateLightClicked(Light::SpotLight); }
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		//if (ImGui::BeginMenu("Components")) {
		//	if (ImGui::MenuItem("Material")) { this->OnMaterialComponentClicked(); }
		//	ImGui::EndMenu();
		//}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Editor Settings", nullptr, UIManager::getInstance()->getEnabled(UINames::SETTINGS_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				UIManager::getInstance()->toggleEnabled(UINames::SETTINGS_SCREEN);
			}
			if (ImGui::MenuItem("Statistics", nullptr, UIManager::getInstance()->getEnabled("Statistics")))
			{
				CommandManager::getInstance()->clearTopUndo();
				UIManager::getInstance()->toggleEnabled("Statistics");
			}
			if (ImGui::MenuItem("Inspector", nullptr, UIManager::getInstance()->getEnabled(UINames::INSPECTOR_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				CommandManager::getInstance()->executeCommand(new ToggleWindowVisibiltyCommand(UINames::INSPECTOR_SCREEN));
			}
			if (ImGui::MenuItem("Hierarchy", nullptr, UIManager::getInstance()->getEnabled(UINames::HIERARCHY_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				CommandManager::getInstance()->executeCommand(new ToggleWindowVisibiltyCommand(UINames::HIERARCHY_SCREEN));
			}
			if (ImGui::MenuItem("Profiler", nullptr, UIManager::getInstance()->getEnabled(UINames::PROFILER_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				CommandManager::getInstance()->executeCommand(new ToggleWindowVisibiltyCommand(UINames::PROFILER_SCREEN));
			}
			if (ImGui::MenuItem("Debug Console", nullptr, UIManager::getInstance()->getEnabled(UINames::CONSOLE_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				CommandManager::getInstance()->executeCommand(new ToggleWindowVisibiltyCommand(UINames::CONSOLE_SCREEN));
			}
			if (ImGui::MenuItem("Material Editor", nullptr, UIManager::getInstance()->getEnabled(UINames::MATERIAL_EDITOR_SCREEN)))
			{
				CommandManager::getInstance()->clearTopUndo();
				CommandManager::getInstance()->executeCommand(new ToggleWindowVisibiltyCommand(UINames::MATERIAL_EDITOR_SCREEN));
			}
			//if (ImGui::MenuItem("Playback Options", nullptr, UIManager::getInstance()->getEnabled(UINames::PLAYBACK_SCREEN)))
			//{
			//	UIManager::getInstance()->toggleEnabled(UINames::PLAYBACK_SCREEN);
			//}
			//if (ImGui::BeginMenu("Viewport"))
			//{
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
			//	ImGui::EndMenu();
			//}
			//if (ImGui::MenuItem("Color Picker", nullptr, isColorPickerOpen))
			//{
			//	isColorPickerOpen = !isColorPickerOpen;
			//}

			ImGui::EndMenu();
		}

		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 100 - 10, 0));

		if (ImGui::BeginMenu("Layout"))
		{
			// if (ImGui::MenuItem("[DEBUG] Save Default Layout"))
			// {
			// 	UIManager::saveDefaultLayout();
			// }
			if (ImGui::MenuItem("Default"))
			{
				UIManager::getInstance()->loadPresetLayout(0);
			}

			if (ImGui::MenuItem("Tall"))
			{
				UIManager::getInstance()->loadPresetLayout(1);
			}

			if (ImGui::MenuItem("Wide"))
			{
				UIManager::getInstance()->loadPresetLayout(2);
			}

			ImGui::Spacing();

			if (ImGui::MenuItem("Save Window Layout"))
			{
				this->isSaveLayoutOpen = true;
			}

			if (ImGui::MenuItem("Load Window Layout"))
			{
				UIManager::getInstance()->loadLayoutFromFile();
				//this->isLoadLayoutOpen = true;
			}
			
			if (ImGui::MenuItem("Reset Window Layout"))
			{
				UIManager::getInstance()->resetLayout();
			}

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
		if (isSaveSceneAsOpen)
			ShowSaveSceneAsMenu();
		if (isSaveLayoutOpen)
			ShowSaveLayoutAsMenu();
		if (isLoadLayoutOpen)
			ShowLoadLayoutAsMenu();

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
	CommandManager::getInstance()->executeCommand(
		new CreatePrimitiveCommand(
			GameObject::PrimitiveType::CUBE,
			"Cube"
		)
	);
}

void MenuScreen::OnCreateSphereClicked()
{
	CommandManager::getInstance()->executeCommand(
		new CreatePrimitiveCommand(
			GameObject::PrimitiveType::SPHERE,
			"Sphere"
		)
	);
}

void MenuScreen::onCreateCapsuleClicked()
{
	CommandManager::getInstance()->executeCommand(
		new CreatePrimitiveCommand(
			GameObject::PrimitiveType::CAPSULE,
			"Capsule"
		)
	);
}

void MenuScreen::onCreateCylinderClicked()
{
	CommandManager::getInstance()->executeCommand(
		new CreatePrimitiveCommand(
			GameObject::PrimitiveType::CYLINDER,
			"Cylinder"
		)
	);
}

void MenuScreen::OnCreatePlaneClicked()
{
	CommandManager::getInstance()->executeCommand(
		new CreatePrimitiveCommand(
			GameObject::PrimitiveType::PLANE,
			"Plane"
		)
	);
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

void MenuScreen::OnCreateLightClicked(Light::LightType type)
{
	switch (type)
	{
	case Light::PointLight:

		CommandManager::getInstance()->executeCommand(
			new CreateLightCommand(
				Light::PointLight,
				"Point Light"
			)
		);
		break;

	case Light::DirectionalLight:

		CommandManager::getInstance()->executeCommand(
			new CreateLightCommand(
				Light::DirectionalLight,
				"Directional Light"
			)
		);
		break;
	case Light::SpotLight:

		CommandManager::getInstance()->executeCommand(
			new CreateLightCommand(
				Light::SpotLight,
				"Spot Light"
			)
		);
		break;
	}
}

void MenuScreen::OnCreateRProbe()
{
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);

	Model sphereModel = Model::CreateSphere(vec3(0, 0, 0), 75.0f, *groundReflectMat, false);
	auto sphere = std::make_unique<GameObject>("Reflection Probe", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphereModel));
	sphere->setLocalPosition(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(sphere));
}

void MenuScreen::OnCreateTProbe()
{
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(-360.0f);


	Model sphereModel = Model::CreateSphere(vec3(0, 0, 0), 75.0f, *groundReflectMat, false);
	auto sphere = std::make_unique<GameObject>("Reflection Probe", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphereModel));
	sphere->setLocalPosition(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(sphere));
}

void MenuScreen::OnCreateMProbe()
{
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);

	Model sphereModel = Model::CreateSphere(vec3(0, 0, 0), 75.0f, *mirror, false);
	auto sphere = std::make_unique<GameObject>("Reflection Probe", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphereModel));
	sphere->setLocalPosition(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(sphere));
}

void MenuScreen::onCreateBunnyClicked()
{
	auto filePath = FileUtils::getAssetsFolderPath().generic_string() + "/models/bunny.obj";

	CommandManager::getInstance()->executeCommand(
		new CreateMeshCommand(
			filePath,
			"Bunny",
			glm::vec3(0),
			glm::vec3(0),
			glm::vec3(100)
		)
	);
}

void MenuScreen::onCreateTeapotClicked()
{
	auto filePath = FileUtils::getAssetsFolderPath().generic_string() + "/models/teapot.obj";

	CommandManager::getInstance()->executeCommand(
		new CreateMeshCommand(
			filePath,
			"Teapot",
			glm::vec3(172.5f, -9, -212.5f),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(5.0f)
		)
	);

	/* Reference intial position */

	//teapot.Transform(
	//	rotate(
	//		scale(
	//			translate(i, vec3(555 - 300 - 165 / 2, -9, -295 - 165 / 2)),
	//			vec3(1)),
	//		radians(75.0f), vec3(0, 1, 0)));

	//teapotObj->setLocalScale(5.0f, 5.0f, 5.0f);
}

void MenuScreen::onCreateLucyClicked()
{
	auto filePath = FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj";

	CommandManager::getInstance()->executeCommand(
		new CreateMeshCommand(
			filePath,
			"Lucy",
			glm::vec3(172.5f, -9, -212.5f),
			glm::vec3(0.0f, 75.0f, 0.0f),
			glm::vec3(0.5f)
		)
	);

	/* Reference intial position */

	//lucy0.Transform(
	//	rotate(
	//		scale(
	//			translate(i, vec3(555 - 300 - 165 / 2, -9, -295 - 165 / 2)),
	//			vec3(0.5)),
	//		radians(75.0f), vec3(0, 1, 0)));
}

void MenuScreen::ShowSaveSceneAsMenu()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400));

	if (ImGui::Begin("Save Scene As", &isSaveSceneAsOpen))
	{
		static std::string name = "New Scene";

		ImGui::Text("Save the current scene as:");
		ImGui::InputTextWithHint("Scene Name", name.c_str(), &name);

		if (ImGui::Button("Save", ImVec2(150, 25)))
		{
			SceneIO::getInstance()->SaveCurrentScene(name);
			isSaveSceneAsOpen = false;
		}
	}
	ImGui::End();
}

void MenuScreen::ShowSaveLayoutAsMenu()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400));

	if (ImGui::Begin("Save Layout", &isSaveLayoutOpen))
	{
		static std::string name = "New Layout";

		ImGui::Text("Save the current layout as:");
		ImGui::InputTextWithHint("Layout Name", name.c_str(), &name);

		if (ImGui::Button("Save", ImVec2(150, 25)))
		{
			UIManager::getInstance()->saveLayout(name);
			isSaveLayoutOpen = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(150, 25)))
		{
			isSaveLayoutOpen = false;
		}
	}
	ImGui::End();
}

void MenuScreen::ShowLoadLayoutAsMenu()
{
	ImGui::SetNextWindowSize(ImVec2(200, 50));

	if (ImGui::Begin("Save Layout", &isLoadLayoutOpen))
	{
		if (ImGui::Button("Select File", ImVec2(200, 25)))
		{
			UIManager::getInstance()->loadLayoutFromFile();
			isLoadLayoutOpen = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(150, 25)))
		{
			isLoadLayoutOpen = false;
		}
	}
	ImGui::End();
}

void MenuScreen::OnMaterialComponentClicked()
{
	// Debug::Log("Creating material placeholder.");
}

void MenuScreen::ShowLoadingPopUp()
{
	isLoading = true;

	static ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMouseInputs |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoDecoration;
	ImGui::SetNextWindowSize(ImVec2(150, 50));
	setWindowAlignment(ScreenAlign::CENTER);

	if (ImGui::Begin("Please wait.", &isLoading, flags))
	{
		ImGui::Text("Loading...");
	}
	ImGui::End();
}

void MenuScreen::OnLoadSphereWorld()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 6);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadRTIOW()
{
	// GameObjectManager::getInstance()->clearAll();
	// RayTracingProper::getInstance()->generateSphereWorld();
	// RayTracingProper::getInstance()->renderSceneFromHierarchy();
	// while (!isLoading) {}
	// isLoading = false;
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 1);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadBoxWorld()
{
	// while (!isLoading) {}
	// isLoading = false;

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 8);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadCornellBox()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 7);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadAnitoTracerDemo()
{
	// while (!isLoading) {}
	// isLoading = false;

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 9);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadShowcase()
{
	// while (!isLoading) {}
	// isLoading = false;

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 10);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadSponza()
{
	// while (!isLoading) {}
	// isLoading = false;

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 11);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadSanMiguel()
{
	// while (!isLoading) {}
	// isLoading = false;

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 12);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadVokselia()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 13);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadBreakfast()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 14);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadBathroom()
{
	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 15);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadGallery()
{

	ModelManager::getInstance()->clearAllObjects();
	std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
	parameters->encodeInt("SCENE_INDEX", 16);
	EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
}

void MenuScreen::OnLoadEmpty()
{
	ModelManager::getInstance()->clearAllObjects();
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

void MenuScreen::OnLoadSceneByIndex(int sceneIndex)
{
	CommandManager::getInstance()->executeCommand(new LoadSceneCommand(sceneIndex));
}
