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
		if (owned)
			this->sceneGraph.push_back(std::move(owned));
	}

	if (sceneIndex < 0 || static_cast<size_t>(sceneIndex) >= SceneList::AllScenes.size()) return;

	SceneList::CameraInitialState camState{
	};

	if (auto rt = RayTracer::getInstance())
	{
		auto userSettings = rt->getUserSettings();
		camState.FieldOfView = userSettings.FieldOfView;
		camState.Aperture = userSettings.Aperture;
		camState.FocusDistance = userSettings.FocusDistance;
	}

	auto factory = std::get<1>(SceneList::AllScenes[sceneIndex]);
	if (factory)
	{
		factory(camState);
	}

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void LoadSceneCommand::undo()
{
	auto modelMgr = ModelManager::getInstance();
	if (!modelMgr) return;

	auto currentRoots = modelMgr->getSceneGraph();
	for (auto rootPtr : currentRoots)
	{
		modelMgr->removeObject(rootPtr);
	}

	// Restore previously stored scene roots (move ownership back into the ModelManager)
	for (auto &ownedObj : sceneGraph)
	{
		if (ownedObj)
		{
			modelMgr->addObject(std::move(ownedObj));
		}
	}

	sceneGraph.clear();

	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}