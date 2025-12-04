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
#include "IconsMaterialDesign.h"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/MaterialCommands.hpp"

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
	selectedObject = ModelManager::getInstance()->getSelectedObject();
	//setWindowAlignment(ScreenAlign::CENTER_RIGHT);

	ImGui::Begin("Material Editor", &enabled, UISettings::GlobalWindowFlags);

	if (selectedObject != nullptr)
	{
		showMaterialEditorWindow();
	}

	else
		ImGui::TextWrapped("Select an object to edit its material.");

	ImGui::End();

	if (isColorPickerOpen && !enabled)
		isColorPickerOpen = false;

	if (isColorPickerOpen)
		showColorPickerWindow();

	wasColorPickerOpen = isColorPickerOpen;
}

void MaterialEditorScreen::showColorPickerWindow()
{
	if(!wasColorPickerOpen)
		ImGui::SetNextWindowPos(ImGui::FindWindowByName("Material Editor")->Pos);

	ImGui::SetNextWindowSize(ImVec2(350, 350));
	if (ImGui::Begin("Color Picker", &isColorPickerOpen) && selectedMaterial)
	{
		ImGui::SameLine();
		ImGui::ColorPicker4("Albedo Color##4", reinterpret_cast<float*>(&diffuse), 0);

		if (ImGui::Button("Close & Apply"))
		{
			isColorPickerOpen = false;

			CommandManager::getInstance()->executeCommand(
				new ModifyMaterialPropertyCommand(
					this->selectedMaterial,
					[](Assets::Material* m, const ModifyMaterialPropertyCommand::Variant& v) { m->SetAlbedoColor(std::get<glm::vec4>(v)); },
					this->selectedMaterial->Diffuse,
					glm::vec4(this->diffuse.x, this->diffuse.y, this->diffuse.z, this->diffuse.w)
				));
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

	if(ImGui::CollapsingHeader("Material Inputs", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("SurfaceInputsTable", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
		{
			float labelWidth = ImGui::CalcTextSize("Refraction Index").x;

			ImGui::TableSetupColumn("Drag_Preview", ImGuiTableColumnFlags_WidthFixed, 24.0f);
			ImGui::TableSetupColumn("Texture_Select", ImGuiTableColumnFlags_WidthFixed, 12.0f);
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
			ImGui::TableSetupColumn("InputFields", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow(); // Albedo and Color
			ImGui::TableSetColumnIndex(0);

			if (ImGui::ImageButton("Texture", currTexId, ImVec2(20.0f, ImGui::CalcTextSize("Refraction Index").y)))
			{
				if (TextureLibrary::getInstance()->loadTextureFromFile(this->textureId))
				{
					CommandManager::getInstance()->executeCommand(
						new ModifyMaterialPropertyCommand(
							this->selectedMaterial,
							[](Assets::Material* m, const ModifyMaterialPropertyCommand::Variant& v) { m->SetAlbedoTexture(std::get<int>(v)); },
							this->selectedMaterial->DiffuseTextureId,
							this->textureId
						));
				}
			}

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			ImGui::Dummy(ImVec2(0, 0)); //Unity has drag target + button for opening file view

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(2);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Texture");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(3);

			{
				float previewButtonSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.9f;
				float buttonSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameBorderSize) * 0.1f;

				if (ImGui::ColorButton("Color", diffuse, 0, ImVec2(previewButtonSize, 20.0f)))
				{
					isColorPickerOpen = !isColorPickerOpen;
				}

				ImGui::SameLine();

				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[5]);
				if (ImGui::Button(ICON_MD_COLORIZE, ImVec2(buttonSize, 20.0f)))
				{
					isColorPickerOpen = !isColorPickerOpen;
				}
				ImGui::PopFont();
			}

			/* Metallic */
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Dummy(ImVec2(0, 0));

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			ImGui::Dummy(ImVec2(0, 0));

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(2);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Metallic");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(3);


			{
				float sliderSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.8f;
				float inputSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameBorderSize) * 0.2f;

				ImGui::SetNextItemWidth(sliderSize);
				ImGui::SliderFloat("##Metal_Slider", &this->fuzziness, 0, 1, " %.05f", ImGuiSliderFlags_NoInput);

				ImGui::SameLine();

				ImGui::SetNextItemWidth(inputSize);
				ImGui::InputFloat("##Metal_Input", &this->fuzziness, 0.0f, 0.0f, " %.05f");
			}

			/* Dielectric */
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Dummy(ImVec2(0, 0));

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			ImGui::Dummy(ImVec2(0, 0));

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(2);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Dielectric");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(3);
			ImGui::Checkbox("##DielectricButton", &this->dielectric);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(2);

			if (this->dielectric)
			{
				/* Refraction Index */
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 0));

				ImGui::TableNextColumn();
				ImGui::TableSetColumnIndex(1);
				ImGui::Dummy(ImVec2(0, 0));

				ImGui::TableNextColumn();
				ImGui::TableSetColumnIndex(2);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Refraction Index");

				ImGui::TableNextColumn();
				ImGui::TableSetColumnIndex(3);

				{
					float sliderSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.8f;
					float inputSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameBorderSize) * 0.2f;

					ImGui::PushItemWidth(sliderSize);
					ImGui::SliderFloat("##Refration_Slider", &this->refractionIndex, 0, 15, " %1.0f", ImGuiSliderFlags_NoInput);

					ImGui::SameLine();

					ImGui::PushItemWidth(inputSize);
					ImGui::InputFloat("##Refraction_Input", &this->refractionIndex, 0.0f, 0.0f, " %1.0f");
				}
			}

			ImGui::EndTable();

			if (ImGui::Button("Apply"))
			{
				if (this->dielectric) 
				{
					selectedMaterial->MaterialModel = Material::Enum::Dielectric;

					CommandManager::getInstance()->executeCommand(
						new ModifyMaterialPropertyCommand(
							this->selectedMaterial,
							[](Assets::Material* m, const ModifyMaterialPropertyCommand::Variant& v) { m->SetRefractionIndex(std::get<float>(v)); },
							this->selectedMaterial->RefractionIndex,
							this->refractionIndex
						));
				}
				else 
				{
					CommandManager::getInstance()->executeCommand(
						new ModifyMaterialPropertyCommand(
							this->selectedMaterial,
							[](Assets::Material* m, const ModifyMaterialPropertyCommand::Variant& v) { m->SetFuzziness(std::get<float>(v)); },
							this->selectedMaterial->Fuzziness,
							1 - this->fuzziness
						));
					
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
	} // End Collapsing
}
