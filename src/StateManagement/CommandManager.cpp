#include "CommandManager.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"

CommandManager* CommandManager::sharedInstance = nullptr;

CommandManager* CommandManager::getInstance()
{
	return sharedInstance;
}

void CommandManager::initialize()
{
	sharedInstance = new CommandManager();
}

void CommandManager::destroy()
{
	delete sharedInstance;
}

CommandManager::CommandManager() {}

CommandManager::~CommandManager()
{
	clearStack(sharedInstance->undoStack);
	clearStack(sharedInstance->redoStack);
}

void CommandManager::clearStack(CommandStack stack)
{
	while (!stack.empty()) stack.pop();
}

void CommandManager::executeCommand(ICommand* command)
{
	command->execute();
	undoStack.push(command);
	clearStack(this->redoStack);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void CommandManager::undo()
{
	if (this->undoStack.empty()) return;

	auto command = this->undoStack.top();
	this->undoStack.pop();
	command->undo();
	this->redoStack.push(command);
	EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void CommandManager::redo()
{
	if (this->redoStack.empty()) return;

	auto command = this->redoStack.top();
	this->redoStack.pop();
	command->execute();
	this->undoStack.push(command);
}