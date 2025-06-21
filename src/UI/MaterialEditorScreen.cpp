#include "MaterialEditorScreen.h"

#include <algorithm>

#include "imgui_internal.h"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/EventNames.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"

using namespace gdeng03;

MaterialEditorScreen::MaterialEditorScreen() : AUIScreen(UINames::MATERIAL_EDITOR_SCREEN)
{
	//loadDefaultTextures();
}

bool MaterialEditorScreen::canSelectMaterial() const
{
	return selectedMaterial == nullptr;
}

void MaterialEditorScreen::setSelectedMaterial(Material* mat)
{
	//loadDefaultTextures();
	if (mat == selectedMaterial)
		return;

	diffuse = { mat->Diffuse.x, mat->Diffuse.y, mat->Diffuse.z, mat->Diffuse.w };
	textureId = mat->DiffuseTextureId;
	// diffuseTextureId = mat->DiffuseTextureId;
	// fuzziness = mat->Fuzziness;
	// refractionIndex = mat->RefractionIndex;
	// materialModel = mat->MaterialModel;

	// if (!mat->albedoTexture)
	// 	albedoTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(L"assets/images/default_square.png");
	// else
	// {
	// 	this->albedoTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(mat->albedoTexture->fullPath.c_str());
	// 	//LogUtils::log(this, "AlbedoTexture is not null");
	// }
	//
	// if (!mat->metallicTexture)
	// 	metallicTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(L"assets/images/default_square.png");
	// else
	// 	this->metallicTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(mat->metallicTexture->fullPath.c_str());
	//
	// if (!mat->smoothnessTexture)
	// 	smoothnessTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(L"assets/images/default_square.png");
	// else
	// 	this->smoothnessTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(mat->smoothnessTexture->fullPath.c_str());
	//
	// if (!mat->normalTexture)
	// 	normalTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(L"assets/images/default_square.png");
	// else
	// 	this->normalTexture = GraphicsEngine::get()->getTextureManager()->createTextureFromFile(mat->normalTexture->fullPath.c_str());

	// this->metallic = mat->metallic;
	// this->smoothness = mat->smoothness;
	// this->flatness = mat->flatness;
	//
	// this->tiling = mat->tiling;
	// this->offset = mat->offset;

	selectedMaterial = mat;
}

void MaterialEditorScreen::unselectMaterial()
{
	selectedMaterial = nullptr;
	//loadDefaultTextures();
}

// bool* MaterialEditorScreen::getMaterialEditorOpen()
// {
// 	return &isMaterialEditorOpen;
// }

void MaterialEditorScreen::drawUI()
{
	selectedObject = ModelManager::getInstance()->getSelectedObject().get();
	//setWindowAlignment(ScreenAlign::CENTER_RIGHT);

	ImGui::Begin("Material Editor", &enabled, UISettings::GlobalWindowFlags);

	if (selectedObject != nullptr)
	{
		showMaterialEditorWindow();
		updateSelectedMaterial();
	}
	else
		ImGui::TextWrapped("Select an object to edit its material.");

	ImGui::End();

	if (isColorPickerOpen && !enabled)
		isColorPickerOpen = false;

	if (isColorPickerOpen)
		showColorPickerWindow();
}

void MaterialEditorScreen::showColorPickerWindow()
{
	ImGui::SetNextWindowPos(ImGui::FindWindowByName("Material Editor")->Pos);
	ImGui::SetNextWindowSize(ImVec2(350, 350));
	if (ImGui::Begin("Color Picker", &isColorPickerOpen) && selectedMaterial)
	{
		ImGui::SameLine();
		ImGui::ColorPicker4("Albedo Color##4", reinterpret_cast<float*>(&diffuse), 0);

		if (ImGui::Button("Close & Apply"))
		{
			isColorPickerOpen = false;

			// if (selectedMaterial->Diffuse != glm::vec4(this->diffuse.x, this->diffuse.y, this->diffuse.z, this->diffuse.w))
			// {
			selectedMaterial->Diffuse = { this->diffuse.x, this->diffuse.y, this->diffuse.z, this->diffuse.w };
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
			//}
		}
	}
	ImGui::End();
}

void MaterialEditorScreen::updateMaterial(Material* mat)
{
	setSelectedMaterial(mat);
	unselectMaterial();
}

void MaterialEditorScreen::updateSelectedMaterial()
{
	if (!selectedMaterial)
	{
		//loadDefaultTextures();
		return;
	}

	selectedMaterial->Diffuse = { this->diffuse.x, this->diffuse.y, this->diffuse.z, this->diffuse.w };
	selectedMaterial->DiffuseTextureId = this->textureId;
}

void MaterialEditorScreen::showMaterialEditorWindow()
{
	// ImGui::Text("Select Material");
	// std::vector<const char*> materialNames;
	// materialNames.reserve(5);
	//
	// auto vecMaterials = selectedObject->getModel()->Materials();
	//
	// for (int i = 0; i < vecMaterials.size(); i++)
	// {
	// 	materialNames.push_back(std::string("Material " + std::to_string(i + 1)).data());
	// }
	// static int materialIndex = 0;
	//
	// if (ImGui::BeginCombo("Material", materialNames[materialIndex]))
	// {
	// 	for (int n = 0; n < materialNames.size(); n++)
	// 	{
	// 		Debug::Log("Materials: ");
	// 		Debug::Log(materialNames[n]);
	// 		const bool is_selected = (materialIndex == n);
	// 		if (ImGui::Selectable(materialNames[n], is_selected))
	// 			materialIndex = n;
	//
	// 		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
	// 		if (is_selected)
	// 			ImGui::SetItemDefaultFocus();
	// 	}
	// 	ImGui::EndCombo();
	// }
	//
	// selectedMaterial = &vecMaterials[materialIndex];

	const auto model = selectedObject->getModel();

	if (!model)
	{
		ImGui::Text("Selected object has no model.");
		return;
	}

	setSelectedMaterial(model->getMaterial(0));

	if (!selectedMaterial)
	{
		ImGui::Text("Selected object has no materials.");
		return;
	}

	ImGui::NewLine();

	ImGui::Text("Color");
	if (ImGui::ColorButton("Color", diffuse, 0, ImVec2(100, 50)))
	{
		isColorPickerOpen = !isColorPickerOpen;
	}

	// if (ImGui::InputInt("Texture Id", &textureId))
	// {
	// 	selectedMaterial->DiffuseTextureId = textureId;
	// 	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	// }

	ImGui::NewLine();

	//slider size
	ImGui::PushItemWidth(250);

	if (selectedMaterial->MaterialModel == Material::Enum::Metallic)
	{
		ImGui::SameLine();
		if (ImGui::SliderFloat("Fuzziness", &selectedMaterial->Fuzziness, 0, 0))
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);

		ImGui::NewLine();
	}

	if (selectedMaterial->MaterialModel == Material::Enum::Dielectric)
	{
		ImGui::SameLine();
		if (ImGui::SliderFloat("Refraction Index", &selectedMaterial->RefractionIndex, 0, 0))
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);

		ImGui::NewLine();
	}

	ImGui::PopItemWidth();
}
