#include "SceneIO.hpp"

SceneIO* SceneIO::sharedInstance = nullptr;
SceneIO* SceneIO::getInstance()
{
	return sharedInstance;
}

void SceneIO::initialize()
{
	sharedInstance = new SceneIO();
}

void SceneIO::destroy()
{
	delete sharedInstance;
}