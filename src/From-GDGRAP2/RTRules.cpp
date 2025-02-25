#include "RTRules.h"
#include "RTConfig.h"

RTRules* RTRules::sharedInstance = nullptr;
RTRules* RTRules::getInstance()
{
	return sharedInstance;
}

void RTRules::initialize()
{
	sharedInstance = new RTRules();
}

void RTRules::destroy()
{
	delete sharedInstance;
}

void RTRules::setRayTracingType(RayTracingType type)
{
	this->rayTracingType = type;

	if(this->rayTracingType == RayTracingType::QUICK)
	{
		this->raySamplesPerPixel = RTConfig_Debug::RAY_SAMPLES_PER_PIXEL;
		this->bounces = RTConfig_Debug::BOUNCES;
		this->width = RTConfig_Debug::WIDTH;
		this->aspectRatio = RTConfig_Debug::ASPECT_RATIO;
		this->height = RTConfig_Debug::HEIGHT;
		this->numCores = RTConfig_Debug::NUM_CORES;
		this->widthWindow = RTConfig_Debug::WIDTH_WINDOW;
		this->heightWindow = RTConfig_Debug::HEIGHT_WINDOW;
		this->saveImagePath = RTConfig_Debug::SAVE_IMAGE_PATH;
	}
	else if(this->rayTracingType == RayTracingType::PREVIEW)
	{
		//only image resolution remains same as debug.
		this->raySamplesPerPixel = RTConfig_Production::RAY_SAMPLES_PER_PIXEL;
		this->bounces = RTConfig_Production::BOUNCES;
		this->width = RTConfig_Debug::WIDTH;
		this->aspectRatio = RTConfig_Debug::ASPECT_RATIO;
		this->height = RTConfig_Debug::HEIGHT;
		this->numCores = RTConfig_Production::NUM_CORES;
		this->widthWindow = RTConfig_Debug::WIDTH_WINDOW;
		this->heightWindow = RTConfig_Debug::HEIGHT_WINDOW;
		this->saveImagePath = RTConfig_Production::SAVE_IMAGE_PATH;
	}

	else if (this->rayTracingType == RayTracingType::PRODUCTION)
	{
		this->raySamplesPerPixel = RTConfig_Production::RAY_SAMPLES_PER_PIXEL;
		this->bounces = RTConfig_Production::BOUNCES;
		this->width = RTConfig_Production::WIDTH;
		this->aspectRatio = RTConfig_Production::ASPECT_RATIO;
		this->height = RTConfig_Production::HEIGHT;
		this->numCores = RTConfig_Production::NUM_CORES;
		this->widthWindow = RTConfig_Production::WIDTH_WINDOW;
		this->heightWindow = RTConfig_Production::HEIGHT_WINDOW;
		this->saveImagePath = RTConfig_Production::SAVE_IMAGE_PATH;
	}
}

void RTRules::setPathTracingType(PathTracingType type)
{
	this->pathTracingType = type;
}

std::string RTRules::getSaveImagePath()
{
	return this->saveImagePath;
}

RTRules::RTRules()
{
	this->setRayTracingType(QUICK);
	// this->setRayTracingType(PRODUCTION);

	this->setPathTracingType(SLIDING_WINDOW);
}

RTRules::~RTRules()
{
}
