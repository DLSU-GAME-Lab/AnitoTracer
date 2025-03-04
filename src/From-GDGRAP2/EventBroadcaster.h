#pragma once
#include "EventNames.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Observer;
class Parameters;
class EventBroadcaster
{
public:
	typedef std::string String;
	static EventBroadcaster* getInstance();
	static void initialize();
	static void destroy();

	void addObserver(String name, Observer* observer);
	void removeObserver(String name);
	void broadcastEvent(String name);
	void broadcastEventWithParams(String name, std::shared_ptr<Parameters> parameters);

private:
	EventBroadcaster();
	~EventBroadcaster();
	EventBroadcaster(EventBroadcaster const&) {}; // copy constructor is private
	EventBroadcaster& operator=(EventBroadcaster const&) {}; // assignment operator is private*/
	static EventBroadcaster* sharedInstance;

	typedef std::vector<Observer*> ObserverList;
	typedef std::unordered_map <String, ObserverList> HashTable;
	HashTable observerTable;
};

class Observer
{
public:
	typedef std::string String;
private:
	virtual void onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters = nullptr) = 0;
	friend class EventBroadcaster;
};

class Parameters
{
private:
	typedef std::string String;
public:
	Parameters(const String eventName);
	~Parameters();
	void encodeBool(const String keyName, bool item);
	void encodeInt(const String keyName, int item);
	void encodeFloat(const String keyName, float item);

	bool getBoolData(const String keyName, bool defaultValue);
	int getIntData(const String keyName, int defaultValue);
	float getFloatData(const String keyName, float defaultValue);

private:
	String eventName;
	typedef std::unordered_map <String, bool> BoolTable;
	typedef std::unordered_map <String, int> IntTable;
	typedef std::unordered_map <String, float> FloatTable;

	BoolTable boolTable;
	IntTable intTable;
	FloatTable floatTable;
};

