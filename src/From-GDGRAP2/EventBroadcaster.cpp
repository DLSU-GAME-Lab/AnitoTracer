#include "EventBroadcaster.h"
#include "Debug.h"

EventBroadcaster* EventBroadcaster::sharedInstance = nullptr;
EventBroadcaster* EventBroadcaster::getInstance()
{
	return sharedInstance;
}

void EventBroadcaster::initialize()
{
	sharedInstance = new EventBroadcaster();
}

void EventBroadcaster::destroy()
{
	delete sharedInstance;
}

EventBroadcaster::EventBroadcaster()
{
	
}

EventBroadcaster::~EventBroadcaster()
{
	
}

Parameters::Parameters(const String eventName)
{
	this->eventName = eventName;
}

Parameters::~Parameters()
{
	this->boolTable.clear();
	this->intTable.clear();
	this->floatTable.clear();
}

void Parameters::encodeBool(const String keyName, bool item)
{
	this->boolTable[keyName] = item;
}

void Parameters::encodeInt(const String keyName, int item)
{
	this->intTable[keyName] = item;
}

void Parameters::encodeFloat(const String keyName, float item)
{
	this->floatTable[keyName] = item;
}

bool Parameters::getBoolData(const String keyName, bool defaultValue)
{
	if (this->boolTable.contains(keyName))
		return this->boolTable[keyName];
	else
	{
		String msg = keyName + " does not exist in parameter of event " + this->eventName + "\n";
		Debug::Log(msg);
		return defaultValue;
	}
}

int Parameters::getIntData(const String keyName, int defaultValue)
{
	if (this->intTable.contains(keyName))
		return this->intTable[keyName];
	else
	{
		String msg = keyName + " does not exist in parameter of event " + this->eventName + "\n";
		Debug::Log(msg);
		return defaultValue;
	}
}

float Parameters::getFloatData(const String keyName, float defaultValue)
{
	if (this->floatTable.contains(keyName))
		return this->floatTable[keyName];
	else
	{
		String msg = keyName + " does not exist in parameter of event " + this->eventName + "\n";
		Debug::Log(msg);
		return defaultValue;
	}
}

void EventBroadcaster::addObserver(String name, Observer* observer)
{
	this->observerTable[name].push_back(observer);
}

void EventBroadcaster::removeObserver(String name)
{
	this->observerTable[name].clear();
	this->observerTable.erase(name);
}

void EventBroadcaster::broadcastEvent(String name)
{
	ObserverList observerList = this->observerTable[name];
	for(int i = 0 ; i < observerList.size(); i++)
	{
		observerList[i]->onTriggeredEvent(name);
	}
}

void EventBroadcaster::broadcastEventWithParams(const String name, std::shared_ptr<Parameters> parameters)
{
	ObserverList observerList = this->observerTable[name];
	for (int i = 0; i < observerList.size(); i++)
	{
		observerList[i]->onTriggeredEvent(name, parameters);
	}
}
