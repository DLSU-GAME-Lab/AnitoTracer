#include "FurnitureMenuScreen.h"

#include <sstream>

FurnitureMenuScreen::FurnitureMenuScreen() : AUIScreen(UINames::FURNITURE_MENU_SCREEN)
{
	mPath = std::filesystem::absolute(std::filesystem::current_path());

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
	if (ImGui::Begin("Assets", &mVisible))
	{
		// Show the current path with clickable button for each folder to travel through
		ImGui::Text("Current path : ");
		std::filesystem::path fullpath;
		for (const std::filesystem::path& path : mPath)
		{
			fullpath /= path;
			ImGui::SameLine();
			if (ImGui::Button(path.string().c_str()))
			{
				SetPath(fullpath);
				break;
			}
		}

		auto ButtonWithIcon = [](const std::string& name)
			{
				ImGui::PushID(name.c_str());
				bool clicked = false;
				const ImTextureID imTex = nullptr;
				clicked |= ImGui::ImageButton(imTex, ImVec2(16, 16));
				ImGui::SameLine();
				clicked |= ImGui::Button(name.c_str());
				ImGui::PopID();
				return clicked;
			};
		auto ButtonFile = [&, this](const std::string& name) { return ButtonWithIcon(name); };
		auto ButtonFolder = [&, this](const std::string& name) { return ButtonWithIcon(name); };
		auto IsMatchingFilter = [this](const std::string& extension) { return mSettings.extensionsFilter.empty() || std::any_of(mSettings.extensionsFilter.begin(), mSettings.extensionsFilter.end(), [&](const std::string& ext) { return ext == extension; }); };

		if (mPath.has_parent_path())
		{
			const std::filesystem::path parent = mPath.parent_path();
			if (ButtonFolder(".."))
			{
				SetPath(parent);
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
				if (ButtonFolder(entryName))
				{
					SetPath(path);
					break;
				}
			}
			else if (entry.is_regular_file())
			{
				if (!mSettings.foldersOnly)
				{
					if (IsMatchingFilter(path.filename().extension().string()))
					{
						if (ButtonFile(entryName.c_str()))
						{
							if (mOnPicked)
								mOnPicked(path);
							Hide();
						}
					}
				}
			}
			else
			{
				ImGui::Text(entryName.c_str());
			}
		}
		if (mSettings.foldersOnly)
		{
			if (ImGui::Button("Select this folder"))
			{
				if (mOnPicked)
					mOnPicked(mPath);
				Hide();
			}
		}
	}
	ImGui::End();
}

void FurnitureMenuScreen::SetPath(const std::filesystem::path& path)
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

void FurnitureMenuScreen::PickFolder(const std::filesystem::path& startingPath, const OnPicked& onPicked)
{
	Settings settings;
	settings.foldersOnly = true;
	StartPicking(startingPath, onPicked, settings);
}
void FurnitureMenuScreen::PickFile(const std::filesystem::path& startingPath, const OnPicked& onPicked, const std::vector<std::string>& extensionFilters/* = std::vector<std::string>()*/)
{
	Settings settings;
	settings.extensionsFilter = extensionFilters;
	StartPicking(startingPath, onPicked, settings);
}
void FurnitureMenuScreen::StartPicking(const std::filesystem::path& startingPath, const OnPicked& onPicked, const Settings& settings/* = Settings()*/)
{
	SetPath(startingPath);
	Show();
	mOnPicked = onPicked;
	mSettings = settings;
}
