#pragma once
#include "AUIScreen.h"
#include "Engine/LightSystem/Light.h"
#include "Utilities/Glm.hpp"

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
	void OnCreateRProbe();
	void OnCreateTProbe();
	void OnCreateMProbe();

	void onCreateBunnyClicked();
	void onCreateTeapotClicked(); 
	void onCreateLucyClicked(); 
	void onCreateCornellClicked();
	void ShowSaveSceneAsMenu();
	void ShowSaveLayoutAsMenu();
	void ShowLoadLayoutAsMenu();

	void ShowLoadObjMenu();
	void OnMaterialComponentClicked();

	void ShowLoadingPopUp();
	void OnLoadSphereWorld();
	void OnLoadRTIOW();
	void OnLoadBoxWorld();
	void OnLoadCornellBox();
	void OnLoadAnitoTracerDemo();
	void OnLoadShowcase();
	void OnLoadSponza();
	void OnLoadSanMiguel();
	void OnLoadVokselia();
	void OnLoadBreakfast();
	void OnLoadBathroom();
	void OnLoadGallery();
	void OnLoadEmpty();
	void ShowColorPickerWindow();

	friend class UIManager;

	bool isLoadObjOpen = false;
	bool isLoadSceneOpen = false;
	bool isColorPickerOpen = false;
	bool isOpen = false;
	bool isSaveSceneAsOpen = false;
	bool isSaveLayoutOpen = false;
	bool isLoadLayoutOpen = false;

	bool openSceneSelected = false;
	bool isLoading = false;

	// ImGui::FileBrowser* saveSceneDialog;
	// ImGui::FileBrowser* openSceneDialog;
};

