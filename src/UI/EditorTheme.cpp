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
	//TextDisabled

	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), // WindowBG
	ImVec4(0.196f, 0.196f, 0.196f, 1.0f), // CHILD_BG
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //PopupBG

	ImVec4(0.f, 0.f, 0.f, 0.f), //Border
	//BorderShadow

	ImVec4(0.165f, 0.165f, 0.165f, 1.0f), //Frame BG
	ImVec4(0.404f, 0.404f, 0.404f, 1.0f), //Frame BG Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Frame BG Active

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG Active
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Title BG Collapsed

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Menu Bar BG

	//Scrollbar BG
	//Scrollbar Grab
	//Scrollbar Grab Hovered
	//Scrollbar Grab Active

	//Checkmark

	//Slider Grab
	//Slider Grab Active

	ImVec4(0.345f, 0.345f, 0.345f, 1.0f), //Button
	ImVec4(0.504f, 0.504f, 0.504f, 0.698f), //Button Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Button Active 

	ImVec4(0.196f, 0.196f, 0.196f, 1.0f), //Header
	ImVec4(0.118f, 0.118f, 0.118f, 1.0f), //Header Hovered
	ImVec4(0.275f, 0.376f, 0.486f, 0.698f), //Header Active

	//Separator
	//Separator Hovered
	//Separator Active

	//Resize Grip
	//Resize Grip Hovered
	//Resize Grip Active

	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Tab
	ImVec4(0.118f, 0.118f, 0.118f, 1.0f), //Tab Hovered
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //Tab Active
	ImVec4(0.063f, 0.063f, 0.063f, 1.0f), //Tab Unfocused
	ImVec4(0.235f, 0.235f, 0.235f, 1.0f), //Tab Unfocused Active

	//Plot Lines
	//Plot Lines Hovered
	//Plot Histogram
	//Plot Histogram Hovered
	
	//Table Header BG
	//Table Border Strong
	//Table Border Light
	//Table Row BG
	//Table Row BG ALT

	ImVec4(0.7686f, 0.7686f, 0.7686f, 1.0f), //Text Selected BG
	//Drag Drop Target

	//Nav Highlight
	//Nav Windowing Highlight
	//Nav Windowing Dim BG
	//Modal Window Dim BG
};