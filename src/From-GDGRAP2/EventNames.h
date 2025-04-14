#pragma once
#include <string>

class EventNames
{
public:
	typedef std::string String;
	inline static const String ON_RAY_TRACE_CONFIG_CHANGED = "ON_RAY_TRACE_CONFIG_CHANGED";
	inline static const String ON_RAY_TRACE_COMPUTE_FINISHED = "ON_RAY_TRACE_COMPUTE_FINISHED";
	inline static const String ON_SCENE_LOADED = "ON_SCENE_LOADED";
	inline static const String ON_MARK_SCENE_DIRTY = "ON_MARK_SCENE_DIRTY"; //whenever a certain object/model has been modified, the scene must be reloaded and re-raytraced to account for new possible inter-reflection and bounces.
};

