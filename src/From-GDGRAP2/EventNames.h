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
	inline static const String ON_RESET_ACCUMULATOR = "ON_RESET_ACCUMULATOR";
	inline static const String ON_OBJECT_CREATED = "ON_OBJECT_CREATED";
	inline static const String ON_OBJECT_DELETED = "ON_OBJECT_DELETED";
	inline static const String RAYS_START_RENDER = "RAYS_START_RENDER";
	inline static const String RAYS_END_RENDER = "RAYS_END_RENDER";
	inline static const String ON_SAMPLE_PROGRESS = "ON_SAMPLE_PROGRESS"; // Broadcasts at configurable percentage intervals (default 1%) with progress data
	inline static const String ON_SWAP_RENDERER = "ON_SWAP_RENDERER"; // Broadcasts when user switches between Legacy and Compute Shader renderers
	inline static const String ON_SHADOW_SETTINGS_CHANGED = "ON_SHADOW_SETTINGS_CHANGED"; // Broadcasts when per-light shadow settings are edited in the Inspector
};

