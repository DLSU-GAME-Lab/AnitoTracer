#include "EditorTheme.hpp"

/* 
	Comments Without Values = unchanged styles from default 
	Add styles if needed 
*/

EditorTheme DarkTheme =
{
	"Inter-Regular.ttf", //Editor Theme
	"MaterialIcons-Regular.ttf", //Icon Font

	ImVec4(0.7686f, 0.7686f, 0.7686f, 1.0f), // Text
	ImVec4(0.7686f, 0.7686f, 0.7686f, 1.0f), //TextDisabled (no value yet)

	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), // WindowBG
	ImVec4(0.196f, 0.196f, 0.196f, 1.0f), // CHILD_BG
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //PopupBG

	ImVec4(0.f, 0.f, 0.f, 0.f), //Border
	ImVec4(0.165f, 0.165f, 0.165f, 1.0f), //BorderShadow

	ImVec4(0.165f, 0.165f, 0.165f, 1.0f), //Frame BG
	ImVec4(0.404f, 0.404f, 0.404f, 1.0f), //Frame BG Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Frame BG Active

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG Active
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG Collapsed

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Menu Bar BG

	ImVec4(0.165f, 0.165f, 0.165f, 1.0f), //Scrollbar BG
	ImVec4(0.345f, 0.345f, 0.345f, 1.0f), //Scrollbar Grab
	ImVec4(0.404f, 0.404f, 0.404f, 1.0f), //Scrollbar Grab Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Scrollbar Grab Active (Unity has only blue outline)

	ImVec4(0.8431f, 0.8431f, 0.8431f, 1.0f), //Checkmark

	ImVec4(0.6000f, 0.6000f, 0.6000f, 1.0f), //Slider Grab
	ImVec4(0.9176f, 0.9176f, 0.9176f, 1.0f), //Slider Grab Active

	ImVec4(0.345f, 0.345f, 0.345f, 1.0f), //Button
	ImVec4(0.504f, 0.504f, 0.504f, 0.698f), //Button Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Button Active 

	ImVec4(0.196f, 0.196f, 0.196f, 1.0f), //Header
	ImVec4(0.118f, 0.118f, 0.118f, 1.0f), //Header Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Header Active

	ImVec4(0.122f, 0.122f, 0.122f, 1.0f), //Separator
	ImVec4(0.122f, 0.122f, 0.122f, 1.0f), //Separator Hovered
	ImVec4(0.122f, 0.122f, 0.122f, 1.0f), //Separator Active

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Resize Grip
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Resize Grip Hovered
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Resize Grip Active

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Tab
	ImVec4(0.118f, 0.118f, 0.118f, 1.0f), //Tab Hovered
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //Tab Active
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Tab Unfocused
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //Tab Unfocused Active

	ImVec4(0.486f, 0.095, 0.095, 1.0f),		//Plot Lines
	ImVec4(0.486f, 0.095, 0.095, 1.0f),		//PLot Lines Hoverered
	ImVec4(0.486f, 0.095, 0.095, 0.75f),	//Plot Histogram
	ImVec4(0.486f, 0.095, 0.095, 0.75f),	// Plot Histogram Hoverered
	
	ImVec4(0.176f, 0.176f, 0.176f, 1.0f),	//Table Header BG
	ImVec4(0.196f, 0.196f, 0.196f, 0.67f),	//Table Border Strong
	ImVec4(0.176f, 0.176f, 0.176f, 0.67f),	//Table Border Light
	ImVec4(0.176f, 0.176f, 0.176f, 1.0f),	//Table Row BG
	ImVec4(0.196f, 0.196f, 0.196f, 1.0f),	//Table Row BG ALT

	ImVec4(0.769f, 0.769f, 0.769f, 1.0f), //Text Selected BG
	ImVec4(0.267f, 0.267f, 0.267f, 1.0f), //Drag Drop Target

	ImVec4(0.1647f, 0.5176f, 0.8235f, 0.45f), //Nav Highlight
	ImVec4(0.1647f, 0.5176f, 0.8235f, 0.85f), //Windowing Highlight
	ImVec4(0.0f, 0.0f, 0.0f, 0.25f), // Windowing Dim BG (Not From Unity)
	ImVec4(0.0f, 0.0f, 0.0f, 0.60f), //Modal Window Dim BG stronger dim
};