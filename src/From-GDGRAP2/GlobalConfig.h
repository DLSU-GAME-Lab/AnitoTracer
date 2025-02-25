#pragma once
#include <memory>
#include <string>
#include <glm/vec3.hpp>

#include "EventBroadcaster.h"

class ConfigKeys
{
public:
	typedef std::string String;
	inline static const String DO_NOT_RESET_CAMERA = "DO_NOT_RESET_CAMERA";
};
class GlobalConfig final
{
private:
	typedef std::string String;

public:
	static GlobalConfig* getInstance();
	static void initialize();
	static void destroy();

	void encodeBool(const String keyName, bool item) const;
	void encodeInt(const String keyName, int item) const;
	void encodeFloat(const String keyName, float item) const;

	bool getBoolData(const String keyName, bool defaultValue) const;
	int getIntData(const String keyName, int defaultValue) const;
	float getFloatData(const String keyName, float defaultValue) const;

private:
	GlobalConfig();
	~GlobalConfig() = default;
	GlobalConfig(GlobalConfig const&) {}; // copy constructor is private
	GlobalConfig& operator=(EventBroadcaster const&) {}; // assignment operator is private*/
	static GlobalConfig* sharedInstance;


	std::unique_ptr<Parameters> configStorage;

};

