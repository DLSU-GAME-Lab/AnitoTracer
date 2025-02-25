#pragma once
#include <string>
/**
 * \brief Refers to the active ray-tracing rules being used. Production/debug.
 */
class RTRules
{
public:
	static RTRules* getInstance();
	static void initialize();
	static void destroy();

	enum RayTracingType
	{
		QUICK,
		PREVIEW,
		PRODUCTION
	};

	enum PathTracingType
	{
		SLIDING_WINDOW,
		MONTE_CARLO
	};

	enum RayTracingScene
	{
		SPHERE_WORLD,
		CORNELL_BOX
	};

	void setRayTracingType(RayTracingType type);
	void setPathTracingType(PathTracingType type);
	std::string getSaveImagePath();
	friend class RayTracingProper;
	friend class RenderLayer;

private:
	RTRules();
	~RTRules();
	RTRules(RTRules const&) {}; // copy constructor is private
	RTRules& operator=(RTRules const&) {}; // assignment operator is private*/
	static RTRules* sharedInstance;

	int raySamplesPerPixel;
	int bounces;
	int width;
	float aspectRatio;
	int height;
	int numCores;
	int widthWindow;
	int heightWindow;
	std::string saveImagePath;

	RayTracingType rayTracingType;
	PathTracingType pathTracingType;


};

