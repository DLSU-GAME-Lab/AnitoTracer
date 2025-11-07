#pragma once
#include "ICommand.hpp"

#include <stack>

class CommandManager
{
public:
    using CommandStack = std::stack<ICommand*>;

    static CommandManager* getInstance();
    static void initialize();
    static void destroy();

    void executeCommand(ICommand* command);
    void undo();
    void redo();

private:
    CommandManager();
    ~CommandManager();
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    static CommandManager* sharedInstance;

    void clearStack(CommandStack stack);

    CommandStack undoStack;
    CommandStack redoStack;
};
