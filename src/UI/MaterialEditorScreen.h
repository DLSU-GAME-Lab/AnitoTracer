#pragma once

#include <imgui.h>
#include <wrl/client.h>
#include <string>
#include <shobjidl.h> 
#include <vector>

#include "AUIScreen.h"
#include "From-GDGRAP2/GameObject.h"
#include "Assets/Material.hpp"

namespace gdeng03
{
	class MenuScreen;

	using Assets::Material;

	class MaterialEditorScreen :
		public AUIScreen
	{
	public:
		MaterialEditorScreen();
		//bool* getMaterialEditorOpen();

		bool canSelectMaterial() const;
		void updateMaterial(Material* mat);
		void unselectMaterial();

	private:
		void drawUI() override;
		void showColorPickerWindow();
		void updateSelectedMaterial();
		void showMaterialEditorWindow();

		void setSelectedMaterial(Material* mat);
		// static std::vector<unsigned char> getPixelData(const TexturePtr& texture);
		// static bool isNormalImage(const TexturePtr& texture);
		//
		// void loadTextureFile(TexturePtr& texture);
		// void loadDefaultTextures();

	private:
		Material* selectedMaterial = nullptr;
		GameObject* selectedObject = nullptr;

		bool isMaterialChanged = false;
		bool isColorPickerOpen = false;
		//bool isMaterialEditorOpen = false;

		ImVec4 diffuse = ImVec4(1, 1, 1, 1);
		int textureId;
		//
		// // Base material
		// //glm::vec4 Diffuse;
		// int32_t diffuseTextureId;
		//
		// // Metal fuzziness
		// float fuzziness;
		//
		// // Dielectric refraction index
		// float refractionIndex;
		//
		// // Which material are we dealing with
		// Material::Enum materialModel;

		// float metallic = 0;
		// float smoothness = 0;
		// float flatness = 0;
		// ImVec2 tiling = { 1,1 };
		// ImVec2 offset = { 0,0 };

		// TexturePtr albedoTexture;
		// TexturePtr metallicTexture;
		// TexturePtr smoothnessTexture;
		// TexturePtr normalTexture;

		//friend class MenuScreen;
		//friend class InspectorScreen;
	};
}

