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
#include "SettingsScreen.h"
#include "ViewportScreen.h"

UIManager* UIManager::sharedInstance = nullptr;

UIManager* UIManager::getInstance()
{
	return sharedInstance;
}

void UIManager::initialize(UserSettings* userSettings)
{
	sharedInstance = new UIManager();
	sharedInstance->userSettings = userSettings;
}

void UIManager::destroy()
{
	delete sharedInstance;
}

void UIManager::drawAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->enabled)
			i->drawUI();
	}
}

bool UIManager::getEnabled(const std::string& name)
{
	if (!this->uiTable[name])
		return false;

	return this->uiTable[name]->enabled;
}

void UIManager::setEnabled(const String& uiName, const bool flag)
{
	if (this->uiTable[uiName] != nullptr)
	{
		this->uiTable[uiName]->setEnabled(flag);
	}
}

void UIManager::toggleEnabled(const String& uiName)
{
	if (this->uiTable[uiName] != nullptr)
	{
		this->uiTable[uiName]->toggleEnabled();
	}
}

std::shared_ptr<AUIScreen> UIManager::findUIByName(const String& uiName)
{
	if (this->uiTable[uiName] != nullptr)
	{
		return this->uiTable[uiName];
	}

	return nullptr;
}

void UIManager::toggleAllUI()
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(isHidingUI);
	}

	isHidingUI = !isHidingUI;
}

void UIManager::hideAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(false);
	}
}

void UIManager::showAllUI() const
{
	for (const auto& i : this->uiList)
	{
		if (i->name != UINames::MENU_SCREEN)
			i->setEnabled(true);
	}
}

UIManager::UIManager()
{
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	//populate UI table
	//UIs that will show during runtime

	const std::shared_ptr<MenuScreen> menuScreen = std::make_shared<MenuScreen>();
	this->uiTable[UINames::MENU_SCREEN] = menuScreen;
	this->uiList.push_back(menuScreen);

	const std::shared_ptr<HierarchyScreen> hierarchyScreen = std::make_shared<HierarchyScreen>();
	this->uiTable[UINames::HIERARCHY_SCREEN] = hierarchyScreen;
	this->uiList.push_back(hierarchyScreen);

	const std::shared_ptr<InspectorScreen> inspectorScreen = std::make_shared<InspectorScreen>();
	this->uiTable[UINames::INSPECTOR_SCREEN] = inspectorScreen;
	this->uiList.push_back(inspectorScreen);

	const std::shared_ptr<ConsoleScreen> consoleScreen = std::make_shared<ConsoleScreen>();
	this->uiTable[UINames::CONSOLE_SCREEN] = consoleScreen;
	this->uiList.push_back(consoleScreen);
	Debug::assignConsole(consoleScreen);

	const std::shared_ptr<ProfilerScreen> profilerScreen = std::make_shared<ProfilerScreen>();
	this->uiTable[UINames::PROFILER_SCREEN] = profilerScreen;
	this->uiList.push_back(profilerScreen);

	//std::shared_ptr<gdeng03::PlaybackScreen> playbackScreen = std::make_shared<gdeng03::PlaybackScreen>();
	//this->uiTable[uiNames.PLAYBACK_SCREEN] = playbackScreen;
	//this->uiList.push_back(playbackScreen);

	// nawt working yet lol!
	const std::shared_ptr<gdeng03::MaterialEditorScreen> materialEditorScreen = std::make_shared<gdeng03::MaterialEditorScreen>();
	this->uiTable[UINames::MATERIAL_EDITOR_SCREEN] = materialEditorScreen;
	this->uiList.push_back(materialEditorScreen);

	const std::shared_ptr<SettingsScreen> settingsScreen = std::make_shared<SettingsScreen>();
	this->uiTable[UINames::SETTINGS_SCREEN] = settingsScreen;
	this->uiList.push_back(settingsScreen);

	// std::shared_ptr<AssetExplorerScreen> assetExplorerScreen = std::make_shared<AssetExplorerScreen>();
	// this->uiTable[uiNames.ASSET_EXPLORER_SCREEN] = assetExplorerScreen;
	// this->uiList.push_back(assetExplorerScreen);

	// nawt working yet lol!
	// std::shared_ptr<ViewportScreen> viewportScreen = std::make_shared<ViewportScreen>();
	// this->uiTable[uiNames.VIEWPORT_SCREEN] = viewportScreen;
	// this->uiList.push_back(viewportScreen);

	// MaterialScreen* materialScreen = new MaterialScreen();
	// this->uiTable[uiNames.MATERIAL_SCREEN] = materialScreen;
	// this->uiList.push_back(materialScreen);
	// materialScreen->SetEnabled(false);

	Debug::Log("Initialized UIs!");
}

UIManager::~UIManager()
= default;
