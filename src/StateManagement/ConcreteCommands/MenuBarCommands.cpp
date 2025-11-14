#include "MenuBarCommands.hpp"

#include "From-GDGRAP2/ModelManager.h"
#include "SceneList.hpp"
#include "RayTracer.hpp"
#include <tuple>

LoadSceneCommand::LoadSceneCommand(int sceneIndex) : sceneIndex(sceneIndex)
{

}


void LoadSceneCommand::execute()
{
	auto modelMgr = ModelManager::getInstance();
	if (!modelMgr) return;

	// Capture current root objects (move ownership into this command so we can restore on undo)
	auto currentRoots = modelMgr->getSceneGraph(); // snapshot of raw pointers

	for (auto rootPtr : currentRoots)
	{ 
		auto owned = modelMgr->removeObject(rootPtr);
		if (owned)	this->oldSceneGraph.push_back(std::move(owned));
	}

	if (!this->newSceneGraph.empty())
	{
		// Restore previously stored scene roots (move ownership back into the ModelManager)
		for (auto &ownedObj : newSceneGraph)
		{
			if (ownedObj)
			{
				modelMgr->addObject(std::move(ownedObj));
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
	auto modelMgr = ModelManager::getInstance();
	if (!modelMgr) return;

	auto currentRoots = modelMgr->getSceneGraph();

	for (auto rootPtr : currentRoots) // record new scene roots
	{
		auto owned = modelMgr->removeObject(rootPtr);
		if (owned)	this->newSceneGraph.push_back(std::move(owned));
	}

	// Restore previously stored scene roots (move ownership back into the ModelManager)
	for (auto &ownedObj : oldSceneGraph)
	{
		if (ownedObj)
		{
			modelMgr->addObject(std::move(ownedObj));
		}
	}

	oldSceneGraph.clear();

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}