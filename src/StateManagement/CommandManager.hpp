#pragma once
#include "ICommand.hpp"
#include "HotkeySystem/HotkeyListener.hpp"
#include <stack>

class CommandManager : public HotkeyListener
{
public:
    using CommandStack = std::stack<ICommand*>;

    static CommandManager* getInstance();
    static void initialize();
    static void destroy();

    void executeCommand(ICommand* command);
    void undo();
    void redo();

    void OnActionPressed(Hotkey::Action action) override;

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
