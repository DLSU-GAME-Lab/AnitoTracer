#pragma once
#include <memory>
#include <string>
#include "UI/ConsoleScreen.h"
#include <vector>
#include "IETSemaphore.h"

class Debug
{
public:
	typedef std::string String;
	typedef std::vector<String> PendingList;

	static void initialize();
	static void destroy();
	static void Log(String msg);
	static void assignConsole(std::shared_ptr<ConsoleScreen> console);

private:
	Debug();
	~Debug();
	Debug(Debug const&) {};             // copy constructor is private
	Debug& operator=(Debug const&) {};  // assignment operator is private*/

	static Debug* sharedInstance;

	std::shared_ptr<ConsoleScreen> console;
	PendingList pendingMessages;
	std::unique_ptr<IETSemaphore> mutex;



};

