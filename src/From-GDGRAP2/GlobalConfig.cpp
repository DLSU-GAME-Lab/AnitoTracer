#include "GlobalConfig.h"
#include <iostream>

GlobalConfig* GlobalConfig::sharedInstance = nullptr;

GlobalConfig* GlobalConfig::getInstance()
{
	return sharedInstance;
}

void GlobalConfig::initialize()
{
	sharedInstance = new GlobalConfig();
}

void GlobalConfig::destroy()
{
	delete sharedInstance;
}

GlobalConfig::GlobalConfig()
{
	this->configStorage = std::make_unique<Parameters>("GlobalConfig");
}

void GlobalConfig::encodeBool(const String keyName, bool item) const
{
	this->configStorage->encodeBool(keyName, item);
}

void GlobalConfig::encodeInt(const String keyName, int item) const
{
	this->configStorage->encodeInt(keyName, item);
}

void GlobalConfig::encodeFloat(const String keyName, float item) const
{
	this->configStorage->encodeFloat(keyName, item);
}

bool GlobalConfig::getBoolData(const String keyName, bool defaultValue) const
{
	return this->configStorage->getBoolData(keyName, defaultValue);
}

int GlobalConfig::getIntData(const String keyName, int defaultValue) const
{
	return this->configStorage->getIntData(keyName, defaultValue);
}

float GlobalConfig::getFloatData(const String keyName, float defaultValue) const
{
	return this->configStorage->getFloatData(keyName, defaultValue);
}
