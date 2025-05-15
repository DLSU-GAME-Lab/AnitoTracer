#include "FurnitureMenuScreen.h"

#include <sstream>
#include <glm/vec3.hpp>

#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/TextureLibrary.h"
#include "Utilities/FileUtils.h"

FurnitureMenuScreen::FurnitureMenuScreen() : AUIScreen(UINames::FURNITURE_MENU_SCREEN)
{
	mPath = FileUtils::getAssetsFolderPath();
	mSettings.extensionsFilter = { ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3mf", ".stl" };
}

void FurnitureMenuScreen::drawUI()
{

	// if (ImGui::Begin("Assets"))
	// {
	//
	// 	static float padding = 16.0f;
	// 	static float thumbnailSize = 128.0f;
	// 	float cellSize = thumbnailSize + padding;
	//
	// 	float panelWidth = ImGui::GetContentRegionAvail().x;
	// 	int columnCount = (int)(panelWidth / cellSize);
	// 	if (columnCount < 1)
	// 		columnCount = 1;
	//
	// 	ImGui::Columns(columnCount, 0, false);
	//
	// 	ImDrawList* list = ImGui::GetWindowDrawList();
	// 	for (int i = 0; i < 100; i++)
	// 	{
	// 		ImGui::PushID(i);
	//
	// 		ImGui::ImageButton((ImTextureID)0, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
	//
	// 		ImGui::NextColumn();
	// 		ImGui::PopID();
	// 	}
	//
	// 	ImGui::End();
	// }

	namespace ImGui = ::ImGui;
	if (ImGui::Begin("Assets", &enabled))
	{
		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		const float cellSize = thumbnailSize + padding;

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = static_cast<int>(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(1, nullptr, false);

		// Show the current path with clickable button for each folder to travel through
		ImGui::Text("Current path : ");
		std::filesystem::path fullpath;
		for (const std::filesystem::path& path : mPath)
		{
			fullpath /= path;
			ImGui::SameLine();
			if (ImGui::Button(path.string().c_str()))
			{
				setPath(fullpath);
				break;
			}
		}

		ImGui::Separator();

		ImGui::Columns(columnCount, nullptr, false);

		auto buttonWithIcon = [](const std::string& name)
			{
				ImGui::PushID(name.c_str());
				bool clicked = false;
				//const ImTextureID imTex = ;
				clicked |= ImGui::ImageButton(reinterpret_cast<ImTextureID>(2), ImVec2(thumbnailSize, thumbnailSize));
				clicked |= ImGui::Button(name.c_str());
				ImGui::NextColumn();
				ImGui::PopID();
				return clicked;
			};
		auto buttonFile = [&, this](const std::string& name) { return buttonWithIcon(name); };
		auto buttonFolder = [&, this](const std::string& name) { return buttonWithIcon(name); };
		auto isMatchingFilter = [this](const std::string& extension)
			{
				return mSettings.extensionsFilter.empty() || std::ranges::any_of(mSettings.extensionsFilter,
					[&](const std::string& ext)
					{
						return ext == extension;
					});
			};

		if (mPath.has_parent_path())
		{
			const std::filesystem::path parent = mPath.parent_path();
			if (buttonFolder(".."))
			{
				setPath(parent);
			}
		}

		while (!std::filesystem::exists(mPath))
			mPath = mPath.parent_path();

		for (const auto& entry : std::filesystem::directory_iterator(mPath))
		{
			const std::filesystem::path& path = entry.path();
			const std::string entryName = std::filesystem::relative(path, mPath).string();
			if (entry.is_directory())
			{
				if (buttonFolder(entryName))
				{
					setPath(path);
					break;
				}
			}
			else if (entry.is_regular_file())
			{
				if (!mSettings.foldersOnly)
				{
					if (isMatchingFilter(path.filename().extension().string()))
					{
						if (buttonFile(entryName))
						{
							static std::string name = "GameObject";
							GameObject::PrimitiveType type = GameObject::CUBE;

							auto model = Assets::Model::LoadModel(path.generic_string());
							std::shared_ptr<GameObject> gameObject = std::make_shared<GameObject>(name, type, std::make_shared<Assets::Model>(model));

							ModelManager::getInstance()->addObject(gameObject);
							// if (mOnPicked)
							// 	mOnPicked(path);
							//Hide();
						}
					}
				}
			}
			else
			{
				ImGui::Text(entryName.c_str());
			}
		}
		// if (mSettings.foldersOnly)
		// {
		// 	if (ImGui::Button("Select this folder"))
		// 	{
		// 		if (mOnPicked)
		// 			mOnPicked(mPath);
		// 		Hide();
		// 	}
		// }
	}
	ImGui::End();
}

void FurnitureMenuScreen::setPath(const std::filesystem::path& path)
{
	mPath = path;
	// Remove ending / if any
	if (mPath.wstring().back() == std::filesystem::path::preferred_separator)
		mPath = mPath.parent_path();
}

// const sf::Texture& FurnitureMenuScreen::GetFileIcon(const std::string& extension) const
// {
// 	const auto it = mFileIconPerExtension.find(extension);
// 	return it != mFileIconPerExtension.cend() ? it->second : mDefaultFileIcon;
// }

void FurnitureMenuScreen::pickFolder(const std::filesystem::path& startingPath, const OnPicked& onPicked)
{
	Settings settings;
	settings.foldersOnly = true;
	startPicking(startingPath, onPicked, settings);
}
void FurnitureMenuScreen::pickFile(const std::filesystem::path& startingPath, const OnPicked& onPicked, const std::vector<std::string>& extensionFilters/* = std::vector<std::string>()*/)
{
	Settings settings;
	settings.extensionsFilter = extensionFilters;
	startPicking(startingPath, onPicked, settings);
}
void FurnitureMenuScreen::startPicking(const std::filesystem::path& startingPath, const OnPicked& onPicked, const Settings& settings/* = Settings()*/)
{
	setPath(startingPath);
	Show();
	mOnPicked = onPicked;
	mSettings = settings;
}
