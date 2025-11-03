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

		Toggle_GameObjectEnabled,
		DeleteGameObject,
		DuplicateGameObject,
		CutGameObject,
		CopyGameObject,
		PasteGameObject,
		Hierarchy_SetAsFirstSibling,
		Hierarchy_SetAsLastSibling,
		Hierarchy_ToggleVisibilityWithDescendants,
		Hierarchy_TogglePickabilityWithDescendants,

		GameObject_MoveToView

		
	};
}