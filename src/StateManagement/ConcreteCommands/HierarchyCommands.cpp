#include "HierarchyCommands.hpp"
#include "From-GDGRAP2/GameObject.h"

ReparentCommand::ReparentCommand(GameObject* child, GameObject* oldParent, int oldIndex, GameObject* newParent, int newIndex)
	: child(child), oldParent(oldParent), oldIndex(oldIndex), newParent(newParent), newIndex(newIndex)
{

}

void ReparentCommand::execute()
{
	oldParent = child->getParent();
	//add get index function to GameObject
}

void ReparentCommand::undo()
{
}
