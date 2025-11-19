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
		Camera_SpeedUp,
		Camera_SlowDown,
		Camera_FPSMode,
		Camera_NormalPanMode,
		Camera_SlowPanMode,
		Camera_FastPanMode,
		Camera_ZoomMode,
		Camera_OrbitMode,
		Camera_FocusOnGameObject,
		Camera_FocusOnGameObject_Zoomed,
		Camera_Reset,
		Camera_MoveObjectToView,

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
		SceneTool_Transform,
		SceneTool_Cycle,

		GameObject_ToggleActive,
		GameObject_Delete,
		GameObject_Duplicate,
		GameObject_Cut,
		GameObject_Copy,
		GameObject_Paste,
		GameObject_SetAsFirstSibling,
		GameObject_SetAsLastSibling,
		GameObject_ToggleVisibilityWithDescendants,
		GameObject_TogglePickabilityWithDescendants,

		Undo,
		Redo
	};
}