#pragma once
#include "AUIScreen.h"
#include "Engine/LightSystem/Light.h"

class MenuScreen :    public AUIScreen
{
public:
	MenuScreen();
	~MenuScreen();

private:
	virtual void drawUI() override;
	void OnCreateCubeClicked();
	void OnCreateTexturedCubeClicked();
	void OnCreateSphereClicked();
	void onCreateCapsuleClicked();
	void onCreateCylinderClicked();
	void OnCreatePlaneClicked();
	void OnCreateLightClicked(Light::LightType type);
	void ShowLoadObjMenu();
	void OnMaterialComponentClicked();

	void OnLoadSphereWorld();
	void OnLoadRTIOW();
	void OnLoadBoxWorld();
	void OnLoadCornellBox();
	void OnLoadAnitoTracerDemo();
	void OnLoadSponza();
	void OnLoadEmpty();
	void ShowColorPickerWindow();

	friend class UIManager;

	bool isLoadObjOpen = false;
	bool isLoadSceneOpen = false;
	bool isColorPickerOpen = false;
	bool isOpen = false;

	bool openSceneSelected = false;

	// ImGui::FileBrowser* saveSceneDialog;
	// ImGui::FileBrowser* openSceneDialog;
};

