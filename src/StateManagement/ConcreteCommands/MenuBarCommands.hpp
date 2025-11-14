#pragma once
#include "StateManagement/ICommand.hpp"
#include "SceneList.hpp"
#include <vector>
#include <memory>

class LoadSceneCommand : public ICommand
{
public:
	LoadSceneCommand(int sceneIndex);
	~LoadSceneCommand() = default;

	void execute() override;
	void undo() override;

private:
	std::vector<std::unique_ptr<GameObject>> oldSceneGraph;
	std::vector<std::unique_ptr<GameObject>> newSceneGraph;
	int sceneIndex;
};