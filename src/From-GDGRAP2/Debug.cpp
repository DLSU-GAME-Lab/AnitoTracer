#include "Debug.h"
#include <iostream>
#include <sstream>

Debug* Debug::sharedInstance = nullptr;

void Debug::initialize()
{
	sharedInstance = new Debug();
}

void Debug::destroy()
{
	delete sharedInstance;
}

void Debug::Log(String msg)
{
	sharedInstance->mutex->acquire();
	if (sharedInstance->console != nullptr) {
		sharedInstance->console->appendText(msg);
	}
	else {
		sharedInstance->pendingMessages.push_back(msg);
	}

	std::cout << msg;
	sharedInstance->mutex->release();
}

void Debug::assignConsole(std::shared_ptr<ConsoleScreen> console)
{
	sharedInstance->console = console;
	//print first pending messages
	for (int i = 0; i < sharedInstance->pendingMessages.size(); i++) {
		sharedInstance->console->appendText(sharedInstance->pendingMessages[i]);
	}
	sharedInstance->pendingMessages.clear();
}

Debug::Debug()
{
	this->mutex = std::make_unique<IETSemaphore>(1);
}

Debug::~Debug()
{
	this->console = nullptr;
}
