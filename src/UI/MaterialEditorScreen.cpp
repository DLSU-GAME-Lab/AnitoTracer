#include "MaterialEditorScreen.h"

#include <algorithm>

#include "imgui_internal.h"
#include "imgui_impl_vulkan.h"
#include "Vulkan/SingleTimeCommands.hpp"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/EventNames.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/TextureLibrary.h"

#include "ButtonTexture.hpp"
#include "UIManager.h"

using namespace gdeng03;

MaterialEditorScreen::MaterialEditorScreen() : AUIScreen(UINames::MATERIAL_EDITOR_SCREEN)
{
	//textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(0));
	////tex_dset = ImGui_ImplVulkan_AddTexture(textureimg->Sampler().Handle(), textureimg->ImageView().Handle(), VK_IMAGE_LAYOUT_GENERAL);
	//Assets::ButtonTexture buttonImg = Assets::ButtonTexture(textureimg);
	//currTexId = (ImTextureID)(buttonImg.textureDset);

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
	originalMat = mat->MaterialModel;

	if (mat->MaterialModel == Material::Enum::Dielectric) {
		this->dielectric = true;
		this->refractionIndex = mat->RefractionIndex;
	}

	textureimg = nullptr;

	if (mat->DiffuseTextureId == -1)
		textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(0));
	else
		textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(mat->DiffuseTextureId));

	Assets::ButtonTexture newButtonimg = Assets::ButtonTexture(textureimg);
	currTexId = 0;
	currTexId = (ImTextureID)(newButtonimg.textureDset);

	if (mat->MaterialModel == Material::Enum::Lambertian) {
		this->fuzziness = 0;
	}
	else {
		this->fuzziness = 1 - mat->Fuzziness;
	}
	
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
	//TEXTURE
	//ImGui::Image((ImTextureID)(intptr_t)&TextureLibrary::getInstance()->getTextureLibraryList()[selectedMaterial->DiffuseTextureId], ImVec2(100, 50));

	if (ImGui::ImageButton("Texture", currTexId, ImVec2(40,40)))
	{
		//int newTextureId;
		this->textureChanged = TextureLibrary::getInstance()->loadTextureFromFile(this->textureId);
		selectedMaterial->DiffuseTextureId = this->textureId;

		if (this->textureChanged) 
		{
	/*		textureimg = nullptr;

			if (selectedMaterial->DiffuseTextureId == -1)
				textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(0));
			else
				textureimg = new Assets::TextureImage(*UIManager::getInstance()->commandPool, TextureLibrary::getInstance()->getTextureById(selectedMaterial->DiffuseTextureId));

			Assets::ButtonTexture newButtonimg = Assets::ButtonTexture(textureimg);
			currTexId = 0;
			currTexId = (ImTextureID)(newButtonimg.textureDset);*/
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);

		}
	}
	
	ImGui::SameLine();
	ImGui::Text("Texture");

	ImGui::NewLine();
	//COLOR
	if (ImGui::ColorButton("Color", diffuse, 0, ImVec2(50, 50)))
	{
		isColorPickerOpen = !isColorPickerOpen;
	}
	ImGui::SameLine();
	ImGui::Text("Color");

	ImGui::NewLine();

	//slider size
	ImGui::PushItemWidth(100);
	//METALLIC
	if (ImGui::SliderFloat("Metallic", &this->fuzziness, 0, 1, " %.05f"))
	{
		//EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

	ImGui::NewLine();

	if (ImGui::Checkbox("Dielectric", &this->dielectric)) 
	{

	}
	if (ImGui::SliderFloat("Refraction Index", &this->refractionIndex, 0, 15, "%1.0f"))
	{

	}



	//ImGui::SameLine();
	//if (ImGui::SliderFloat("Refraction Index", &selectedMaterial->RefractionIndex, 0, 255))
	//	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);

	//ImGui::NewLine();


	ImGui::PopItemWidth();
	ImGui::NewLine();

	if (ImGui::Button("Apply")) 
	{
		if (this->dielectric) {
			selectedMaterial->MaterialModel = Material::Enum::Dielectric;
			selectedMaterial->RefractionIndex = this->refractionIndex;
		}
		else {
			selectedMaterial->MaterialModel = Material::Enum::Lambertian;
		}

		selectedMaterial->Fuzziness = 1 - this->fuzziness;

		if (selectedMaterial->MaterialModel == Material::Enum::Metallic || selectedMaterial->MaterialModel == Material::Enum::Lambertian) {
			if (selectedMaterial->Fuzziness < 1)
			{
				selectedMaterial->MaterialModel = Material::Enum::Metallic;
			}
			if (selectedMaterial->Fuzziness == 1)
			{
				selectedMaterial->MaterialModel = Material::Enum::Lambertian;
			}
		}

		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}

}
