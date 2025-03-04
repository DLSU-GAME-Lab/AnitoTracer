#include "UIManager.h"

#include "AssetExplorerScreen.h"
#include "ConsoleScreen.h"
#include "From-GDGRAP2/RTConfig.h"
#include "MenuScreen.h"
#include "From-GDGRAP2/Debug.h"
#include "HierarchyScreen.h"
#include "InspectorScreen.h"
#include "MaterialEditorScreen.h"
#include "PlaybackScreen.h"
#include "ProfilerScreen.h"
#include "ViewportScreen.h"

UIManager* UIManager::sharedInstance = NULL;

UIManager* UIManager::getInstance()
{
    return sharedInstance;
}

void UIManager::initialize()
{
    sharedInstance = new UIManager();
}

void UIManager::destroy()
{
    delete sharedInstance;
}

void UIManager::drawAllUI()
{
	for (int i = 0; i < this->uiList.size(); i++) {
		this->uiList[i]->drawUI();
	}
}

bool* UIManager::getEnabled(const std::string& name)
{
	if (!this->uiTable[name])
		return nullptr;

	return &this->uiTable[name]->enabled;
}

void UIManager::setEnabled(String uiName, bool flag)
{
	if(this->uiTable[uiName] != nullptr)
	{
		this->uiTable[uiName]->SetEnabled(flag);
	}
}

std::shared_ptr<AUIScreen> UIManager::findUIByName(String uiName)
{
	if(this->uiTable[uiName] != nullptr)
	{
		return this->uiTable[uiName];
	}
	else
	{
		return nullptr;
	}
}

UIManager::UIManager()
{
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

    //populate UI table
	//UIs that will show during runtime
	UINames uiNames;

	std::shared_ptr<MenuScreen> menuScreen = std::make_shared<MenuScreen>();
	this->uiTable[uiNames.MENU_SCREEN] = menuScreen;
	this->uiList.push_back(menuScreen);

	std::shared_ptr<HierarchyScreen> hierarchyScreen = std::make_shared<HierarchyScreen>();
	this->uiTable[uiNames.HIERARCHY_SCREEN] = hierarchyScreen;
	this->uiList.push_back(hierarchyScreen);

	std::shared_ptr<InspectorScreen> inspectorScreen = std::make_shared<InspectorScreen>();
	this->uiTable[uiNames.INSPECTOR_SCREEN] = inspectorScreen;
	this->uiList.push_back(inspectorScreen);

	std::shared_ptr<ConsoleScreen> consoleScreen = std::make_shared<ConsoleScreen>();
	this->uiTable[uiNames.CONSOLE_SCREEN] = consoleScreen;
	this->uiList.push_back(consoleScreen);
	Debug::assignConsole(consoleScreen);

	std::shared_ptr<ProfilerScreen> profilerScreen = std::make_shared<ProfilerScreen>();
	this->uiTable[uiNames.PROFILER_SCREEN] = profilerScreen;
	this->uiList.push_back(profilerScreen);

	//std::shared_ptr<gdeng03::PlaybackScreen> playbackScreen = std::make_shared<gdeng03::PlaybackScreen>();
	//this->uiTable[uiNames.PLAYBACK_SCREEN] = playbackScreen;
	//this->uiList.push_back(playbackScreen);

	// nawt working yet lol!
	std::shared_ptr<gdeng03::MaterialEditorScreen> materialEditorScreen = std::make_shared<gdeng03::MaterialEditorScreen>();
	this->uiTable[uiNames.MATERIAL_EDITOR_SCREEN] = materialEditorScreen;
	this->uiList.push_back(materialEditorScreen);

	// std::shared_ptr<AssetExplorerScreen> assetExplorerScreen = std::make_shared<AssetExplorerScreen>();
	// this->uiTable[uiNames.ASSET_EXPLORER_SCREEN] = assetExplorerScreen;
	// this->uiList.push_back(assetExplorerScreen);

	// nawt working yet lol!
	// std::shared_ptr<ViewportScreen> viewportScreen = std::make_shared<ViewportScreen>();
	// this->uiTable[uiNames.VIEWPORT_SCREEN] = viewportScreen;
	// this->uiList.push_back(viewportScreen);

	//
	// MaterialScreen* materialScreen = new MaterialScreen();
	// this->uiTable[uiNames.MATERIAL_SCREEN] = materialScreen;
	// this->uiList.push_back(materialScreen);
	// materialScreen->SetEnabled(false);

	Debug::Log("Initialized UIs!");
}

UIManager::~UIManager()
{
}
