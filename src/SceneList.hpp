#pragma once
#include "Utilities/Glm.hpp"
#include <functional>
#include <string>
#include <tuple>
#include <vector>

#include "Engine/LightSystem/Light.h"

namespace Assets
{
	class Model;
	class Texture;
}

typedef std::tuple<std::vector<Assets::Model>, std::vector<Assets::Texture>, std::vector<Assets::LightProperties>> SceneAssets;

class SceneList final
{
public:

	struct CameraInitialState
	{
		glm::mat4 ModelView;
		float FieldOfView;
		float Aperture;
		float FocusDistance;
		float ControlSpeed;
		bool GammaCorrection;
		bool HasSky;
	};

	static SceneAssets CubeAndSpheres(CameraInitialState& camera);
	static SceneAssets RayTracingInOneWeekend(CameraInitialState& camera);
	static SceneAssets PlanetsInOneWeekend(CameraInitialState& camera);
	static SceneAssets LucyInOneWeekend(CameraInitialState& camera);
	static SceneAssets CornellBox(CameraInitialState& camera);
	static SceneAssets CornellBoxLucy(CameraInitialState& camera);

	static SceneAssets GDGRAP2_SphereWorld(CameraInitialState& camera);
	static SceneAssets GDGRAP2_CornellBox(CameraInitialState& camera);
	static SceneAssets GDGRAP2_BoxWorld(CameraInitialState& camera);
	static SceneAssets AnitoTracer_DemoScene(CameraInitialState& camera);
	static SceneAssets Model_Showcase(CameraInitialState& camera);
	static SceneAssets Sponza(CameraInitialState& camera);
	static SceneAssets SanMiguel(CameraInitialState& camera);
	static SceneAssets Empty(CameraInitialState& camera);
	//new sample scenes
	static SceneAssets Vokselia(CameraInitialState& camera);
	static SceneAssets BreakfastRoom(CameraInitialState& camera);
	static SceneAssets SalleDeBain(CameraInitialState& camera);
	static SceneAssets Gallery(CameraInitialState& camera);
	static std::vector<Assets::Texture> AssembleTextureList();

	static const std::vector<std::tuple<std::string, std::function<SceneAssets (CameraInitialState&)>>> AllScenes;
	
};
