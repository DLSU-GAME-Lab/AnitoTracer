#include "MenuBarCommands.hpp"

#include "From-GDGRAP2/ModelManager.h"
#include "SceneList.hpp"
#include "RayTracer.hpp"

#include "UI/UIManager.h"
#include <tuple>

LoadSceneCommand::LoadSceneCommand(int sceneIndex) : sceneIndex(sceneIndex)
{

}


void LoadSceneCommand::execute()
{
	auto modelMgr = GameObjectManager::getInstance();
	if (!modelMgr) return;

	// Capture current root objects (move ownership into this command so we can restore on undo)
	auto currentRoots = modelMgr->GetAllRootObjects(); // snapshot of raw pointers

	for (auto rootPtr : currentRoots)
	{ 
		auto owned = modelMgr->RemoveObject(rootPtr);
		if (owned)	this->oldSceneGraph.push_back(std::move(owned));
	}

	if (!this->newSceneGraph.empty())
	{
		// Restore previously stored scene roots (move ownership back into the ModelManager)
		for (auto &ownedObj : newSceneGraph)
		{
			if (ownedObj)
			{
				modelMgr->AddObject(std::move(ownedObj));
			}
		}
		newSceneGraph.clear();
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		return;
	}
	else
	{
		if (sceneIndex < 0 || static_cast<size_t>(sceneIndex) >= SceneList::AllScenes.size()) return;

		std::shared_ptr<Parameters> parameters = std::make_shared<Parameters>(EventNames::ON_SCENE_LOADED);
		parameters->encodeInt("SCENE_INDEX", this->sceneIndex);
		EventBroadcaster::getInstance()->broadcastEventWithParams(EventNames::ON_SCENE_LOADED, parameters);
	}
}

void LoadSceneCommand::undo()
{
	auto modelMgr = GameObjectManager::getInstance();
	if (!modelMgr) return;

	auto currentRoots = modelMgr->GetAllRootObjects();

	for (auto rootPtr : currentRoots) // record new scene roots
	{
		auto owned = modelMgr->RemoveObject(rootPtr);
		if (owned)	this->newSceneGraph.push_back(std::move(owned));
	}

	// Restore previously stored scene roots (move ownership back into the ModelManager)
	for (auto &ownedObj : oldSceneGraph)
	{
		if (ownedObj)
		{
			modelMgr->AddObject(std::move(ownedObj));
		}
	}

	oldSceneGraph.clear();

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

ToggleWindowVisibiltyCommand::ToggleWindowVisibiltyCommand(std::string windowName)
	: windowName(windowName)
{
}

void ToggleWindowVisibiltyCommand::execute()
{
	if (this->windowName == UINames::SETTINGS_SCREEN)
	{
		UIManager::getInstance()->settingsActive = !UIManager::getInstance()->settingsActive;
	}

	if (this->windowName == UINames::PROFILER_SCREEN)
	{
		UIManager::getInstance()->profilerActive = !UIManager::getInstance()->profilerActive;
	}

	UIManager::getInstance()->toggleEnabled(windowName);
}

void ToggleWindowVisibiltyCommand::undo()
{
	if (this->windowName == UINames::SETTINGS_SCREEN)
	{
		UIManager::getInstance()->settingsActive = !UIManager::getInstance()->settingsActive;
	}

	if (this->windowName == UINames::PROFILER_SCREEN)
	{
		UIManager::getInstance()->profilerActive = !UIManager::getInstance()->profilerActive;
	}

	UIManager::getInstance()->toggleEnabled(windowName);
}
