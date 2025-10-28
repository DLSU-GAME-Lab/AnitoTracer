#pragma once

namespace Hotkey
{
	enum class Action
	{
		Camera_Forward = 0,
		Camera_Backward,
		Camera_StrafeLeft,
		Camera_StrafeRight,
		Camera_Up,
		Camera_Down,

		Toggle_SettingsScreenVisibility,
		Toggle_AllUIVisibility,
		Toggle_Raytracing, //Go Rasterized 
		Toggle_RayVisibility,
		RefreshScene,

		SceneTool_View,
		SceneTool_Move,
		SceneTool_Rotate,
		SceneTool_Scale,
		SceneTool_Rect,
		SceneTool_Transform
	};
}