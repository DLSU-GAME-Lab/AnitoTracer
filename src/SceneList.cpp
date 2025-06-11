#include "SceneList.hpp"
#include "Assets/Material.hpp"
#include "Assets/Model.hpp"
#include "Assets/Texture.hpp"
#include <functional>
#include <iostream>
#include <random>

#include "Assets/SphereProc.hpp"
#include "Engine/CameraSystem/CameraManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/GlobalConfig.h"
#include "From-GDGRAP2/MaterialLibrary.h"
#include "From-GDGRAP2/MathUtils.h"
#include "From-GDGRAP2/ObjectGroup.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/TextureLibrary.h"
#include "From-GDGRAP2/VectorUtils.h"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"

using namespace glm;
using Assets::Material;
using Assets::Model;
using Assets::Texture;

namespace
{

	void AddRayTracingInOneWeekendCommonScene(std::vector<Assets::Model>& models, const bool& isProc, std::function<float()>& random)
	{
		// Common models from the final scene from Ray Tracing In One Weekend book. Only the three central spheres are missing.
		// Calls to random() are always explicit and non-inlined to avoid C++ undefined evaluation order of function arguments,
		// this guarantees consistent and reproducible behaviour across different platforms and compilers.

		models.push_back(Model::CreateSphere(vec3(0, -1000, 0), 1000, *Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)), isProc));

		for (int i = -11; i < 11; ++i)
		{
			for (int j = -11; j < 11; ++j)
			{
				const float chooseMat = random();
				const float center_y = static_cast<float>(j) + 0.9f * random();
				const float center_x = static_cast<float>(i) + 0.9f * random();
				const vec3 center(center_x, 0.2f, center_y);

				if (length(center - vec3(4, 0.2f, 0)) > 0.9f)
				{
					if (chooseMat < 0.8f) // Diffuse
					{
						const float b = random() * random();
						const float g = random() * random();
						const float r = random() * random();

						models.push_back(Model::CreateSphere(center, 0.2f, *Material::Lambertian(vec3(r, g, b)), isProc));
					}
					else if (chooseMat < 0.95f) // Metal
					{
						const float fuzziness = 0.5f * random();
						const float b = 0.5f * (1 + random());
						const float g = 0.5f * (1 + random());
						const float r = 0.5f * (1 + random());

						models.push_back(Model::CreateSphere(center, 0.2f, *Material::Metallic(vec3(r, g, b), fuzziness), isProc));
					}
					else // Glass
					{
						models.push_back(Model::CreateSphere(center, 0.2f, *Material::Dielectric(1.5f), isProc));
					}
				}
			}
		}
	}

}

const std::vector<std::tuple<std::string, std::function<SceneAssets(SceneList::CameraInitialState&)>>> SceneList::AllScenes =
{
	{"Cube And Spheres", CubeAndSpheres}, 
	{"Ray Tracing In One Weekend", RayTracingInOneWeekend}, // USED
	{"Planets In One Weekend", PlanetsInOneWeekend},
	{"Lucy In One Weekend", LucyInOneWeekend},
	{"Cornell Box", CornellBox},
	{"Cornell Box & Lucy", CornellBoxLucy},
	{"GDGRAP2 - Sphere World", GDGRAP2_SphereWorld}, // USED
	{"GRGRAP2 - Cornell Box", GDGRAP2_CornellBox}, // USED
	{"GDGRAP2 - Box World", GDGRAP2_BoxWorld}, // USED
	{"AnitoTracer - Demo Scene", AnitoTracer_DemoScene}, // USED
	{"Model Showcase - Blank", Model_Showcase}, // USED
	{"Sponza", Sponza}, // USED
	{"San Miguel", SanMiguel}, // USED
	{"Empty", Empty}, // USED
};

SceneAssets SceneList::CubeAndSpheres(CameraInitialState& camera)
{
	// Basic test scene.

	camera.ModelView = translate(mat4(1), vec3(0, 0, -2));
	camera.FieldOfView = 90;
	camera.Aperture = 0.05f;
	camera.FocusDistance = 2.0f;
	camera.ControlSpeed = 2.0f;
	camera.GammaCorrection = false;
	camera.HasSky = true;

	std::vector<Model> models;
	std::vector<Texture> textures;

	models.push_back(Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/cube_multi.obj"));
	models.push_back(Model::CreateSphere(vec3(1, 0, 0), 0.5, *Material::Metallic(vec3(0.7f, 0.5f, 0.8f), 0.2f), true));
	models.push_back(Model::CreateSphere(vec3(-1, 0, 0), 0.5, *Material::Dielectric(1.5f), true));
	models.push_back(Model::CreateSphere(vec3(0, 1, 0), 0.5, *Material::Lambertian(vec3(1.0f), 0), true));

	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/land_ocean_ice_cloud_2048.png", Vulkan::SamplerConfig()));


	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::RayTracingInOneWeekend(CameraInitialState& camera)
{
	// Final scene from Ray Tracing In One Weekend book.

	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.1f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const bool isProc = true;

	std::mt19937 engine(42);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	std::vector<Model> models;

	AddRayTracingInOneWeekendCommonScene(models, isProc, random);

	models.push_back(Model::CreateSphere(vec3(0, 1, 0), 1.0f, *Material::Dielectric(1.5f), isProc));
	models.push_back(Model::CreateSphere(vec3(-4, 1, 0), 1.0f, *Material::Lambertian(vec3(0.4f, 0.2f, 0.1f)), isProc));
	models.push_back(Model::CreateSphere(vec3(4, 1, 0), 1.0f, *Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.0f), isProc));


	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::PlanetsInOneWeekend(CameraInitialState& camera)
{
	// Same as RayTracingInOneWeekend but using textures.

	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.1f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const bool isProc = true;

	std::mt19937 engine(42);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	std::vector<Model> models;
	std::vector<Texture> textures;

	AddRayTracingInOneWeekendCommonScene(models, isProc, random);

	models.push_back(Model::CreateSphere(vec3(0, 1, 0), 1.0f, *Material::Metallic(vec3(1.0f), 0.1f, 2), isProc));
	models.push_back(Model::CreateSphere(vec3(-4, 1, 0), 1.0f, *Material::Lambertian(vec3(1.0f), 0), isProc));
	models.push_back(Model::CreateSphere(vec3(4, 1, 0), 1.0f, *Material::Metallic(vec3(1.0f), 0.0f, 1), isProc));

	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_mars.jpg", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_moon.jpg", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/land_ocean_ice_cloud_2048.png", Vulkan::SamplerConfig()));


	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::LucyInOneWeekend(CameraInitialState& camera)
{
	// Same as RayTracingInOneWeekend but using the Lucy 3D model.

	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 1.0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.05f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const bool isProc = true;

	std::mt19937 engine(42);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	std::vector<Model> models;

	AddRayTracingInOneWeekendCommonScene(models, isProc, random);

	auto lucy0 = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj");
	auto lucy1 = lucy0;
	auto lucy2 = lucy0;

	const auto i = mat4(1);
	const float scaleFactor = 0.0035f;

	lucy0.Transform(
		rotate(
			scale(
				translate(i, vec3(0, -0.08f, 0)),
				vec3(scaleFactor)),
			radians(90.0f), vec3(0, 1, 0)));

	lucy1.Transform(
		rotate(
			scale(
				translate(i, vec3(-4, -0.08f, 0)),
				vec3(scaleFactor)),
			radians(90.0f), vec3(0, 1, 0)));

	lucy2.Transform(
		rotate(
			scale(
				translate(i, vec3(4, -0.08f, 0)),
				vec3(scaleFactor)),
			radians(90.0f), vec3(0, 1, 0)));

	lucy0.SetMaterial(*Material::Dielectric(1.5f));
	lucy1.SetMaterial(*Material::Lambertian(vec3(0.4f, 0.2f, 0.1f)));
	lucy2.SetMaterial(*Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.05f));

	models.push_back(std::move(lucy0));
	models.push_back(std::move(lucy1));
	models.push_back(std::move(lucy2));

	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::CornellBox(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(278, 278, 800), vec3(278, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");

	auto box0 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 165, 0), *white);
	auto box1 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 330, 0), *white);

	box0.Transform(rotate(translate(i, vec3(555 - 130 - 165, 0, -65)), radians(-18.0f), vec3(0, 1, 0)));
	box1.Transform(rotate(translate(i, vec3(555 - 265 - 165, 0, -295)), radians(15.0f), vec3(0, 1, 0)));

	std::vector<Model> models;
	models.push_back(Model::CreateCornellBox(555));
	models.push_back(box0);
	models.push_back(box1);

	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::CornellBoxLucy(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(278, 278, 800), vec3(278, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	const auto i = mat4(1);
	const auto sphere = Model::CreateSphere(vec3(555 - 130, 165.0f, -165.0f / 2 - 65), 80.0f, *Material::Dielectric(1.5f), true);
	auto lucy0 = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj");

	lucy0.Transform(
		rotate(
			scale(
				translate(i, vec3(555 - 300 - 165 / 2, -9, -295 - 165 / 2)),
				vec3(0.6f)),
			radians(75.0f), vec3(0, 1, 0)));

	std::vector<Model> models;
	models.push_back(Model::CreateCornellBox(555));
	models.push_back(sphere);
	models.push_back(lucy0);

	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::vector<Texture>(), std::move(lights));
}

/**
 * \brief Replication of the sphere world from GDGRAP2.
 * \param camera
 * \return
 */
SceneAssets SceneList::GDGRAP2_SphereWorld(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(13, 2, 3), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 20;
	camera.Aperture = 0.01f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 5.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;
	// std::vector<Model> models;

	vec3 pos = vec3(0, -1000, 0); float center = 1000;
	Model sphere1Model = Model::CreateSphere(pos, center, *Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)), isProcedural);
	std::shared_ptr<GameObject> sphere1 = std::make_shared<GameObject>("GroundSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere1Model));
	ModelManager::getInstance()->addObject(sphere1);

	pos = vec3(0, 1, 0); center = 1.0f;
	Model sphere2Model = Model::CreateSphere(pos, 1.0f, *Material::Dielectric(1.5f), isProcedural);
	std::shared_ptr<GameObject> sphere2 = std::make_shared<GameObject>("CenterSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere2Model));
	ModelManager::getInstance()->addObject(sphere2);

	pos = vec3(-8, 2.5f, 1); center = 2.5f;
	Model sphere3Model = Model::CreateSphere(pos, 2.5f, *Material::Metallic(vec3(0.4f, 0.2f, 0.1f), MathUtils::randomFloat(0.0f, 0.2f)), isProcedural);
	std::shared_ptr<GameObject> sphere3 = std::make_shared<GameObject>("LeftSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere3Model));
	ModelManager::getInstance()->addObject(sphere3);

	pos = vec3(4, 1, 0); center = 1.0f;
	Model sphere4Model = Model::CreateSphere(pos, 1.0f, *Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.0f), isProcedural);
	std::shared_ptr<GameObject> sphere4 = std::make_shared<GameObject>("RightSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
	ModelManager::getInstance()->addObject(sphere4);

	for (int repeats = 0; repeats < 2; repeats++)
	{
		for (int a = -11; a < 11; a++)
		{
			for (int b = -11; b < 11; b++)
			{
				float matVal = MathUtils::randomFloat();
				vec3 center(a + 0.9f * MathUtils::randomFloat(), 0.2 + (2 * MathUtils::randomFloat()), b + 0.9 * MathUtils::randomFloat());
				if ((center - vec3(4.0, 0.2f, 0.0f)).length() > 0.9f)
				{
					std::shared_ptr<Material> materialInstance;

					if (matVal < 0.8)
					{
						vec3 albedo = 2.0f * VectorUtils::randomFloatVec3();
						float fuzziness = MathUtils::randomFloat(0.0f, 0.95f);
						materialInstance = Material::Metallic(albedo, fuzziness, MathUtils::randomInt(0, 3));
					}
					else if (matVal < 0.95)
					{
						materialInstance = Material::Dielectric(MathUtils::randomFloat(0.5f, 2.5f));
					}
					else
					{
						vec3 albedo = VectorUtils::randomFloatVec3();
						materialInstance = Material::DiffuseLight(albedo);
					}

					Model modelInstance = Model::CreateSphere(center, MathUtils::randomFloat(0.2f, 0.4f), *materialInstance, isProcedural);
					std::shared_ptr<GameObject> objectInstance = std::make_shared<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(modelInstance));
					ModelManager::getInstance()->addObject(objectInstance);
				}
			}
		}

		for (int a = -5; a < 5; a++)
		{
			for (int b = -5; b < 5; b++)
			{
				vec3 center(a + 0.9f * MathUtils::randomFloat(), 0.2 + (5 * MathUtils::randomFloat()), b + 0.9 * MathUtils::randomFloat());

				//add additional reflective spheres
				std::shared_ptr<Material> materialInstance = Material::Dielectric(1.5f);
				Model modelInstance = Model::CreateSphere(center, MathUtils::randomFloat(0.1f, 0.2f), *materialInstance, isProcedural);
				std::shared_ptr<GameObject> objectInstance = std::make_shared<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(modelInstance));
				ModelManager::getInstance()->addObject(objectInstance);
			}
		}
	}

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();

	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::GDGRAP2_CornellBox(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(0, 0, 1800), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");

	Model box0 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 165, 0), *white);
	Model box1 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 330, 0), *white);

	std::shared_ptr<GameObject> box0_Object = std::make_shared<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box0));
	ModelManager::getInstance()->addObject(box0_Object);
	box0_Object->setLocalPosition(vec3(125, -194, 100));
	box0_Object->setLocalRotation(vec3(0, 50, 0));

	std::shared_ptr<GameObject> box1_Object = std::make_shared<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box1));
	ModelManager::getInstance()->addObject(box1_Object);
	box1_Object->setLocalPosition(vec3(-100, -112, -100));
	box1_Object->setLocalRotation(vec3(0, -60, 0));

	Model cornellBoxModel = Model::CreateCornellBox(555);
	std::shared_ptr<GameObject> cornellBoxObject = std::make_shared<GameObject>("CornellBox", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(cornellBoxModel));
	ModelManager::getInstance()->addObject(cornellBoxObject);

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	// Add light objects
	std::shared_ptr<GameObject> pl1 = std::make_shared<GameObject>("Point Light 1", GameObject::PrimitiveType::POINT_LIGHT);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::GDGRAP2_BoxWorld(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(478, 278, -600), vec3(278, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.73, 0.73, 0.73) * 7.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(1000, 10, 1000), *areaLight);
	std::shared_ptr<GameObject> areaLightObject = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(-250.0f, 600.0f, -500.0f);
	ModelManager::getInstance()->addObject(areaLightObject);

	const int boxesPerSide = 20;
	for (int i = 0; i < boxesPerSide; i++)
	{
		for (int j = 0; j < boxesPerSide; j++)
		{
			float w = 100.0;
			float x0 = -1000.0 + i * w;
			float z0 = -1000.0 + j * w;
			float y0 = 0.0;
			float x1 = x0 + w;
			float y1 = MathUtils::randomFloat(1, 201);
			float z1 = z0 + w;

			std::shared_ptr<Material> groundMat = Material::Metallic(VectorUtils::randomFloatVec3(), MathUtils::randomFloat(0.0, 0.25f));
			Model box = Model::CreateBox(vec3(x0, y0, z0), vec3(x1, y1, z1), *groundMat);
			std::shared_ptr<GameObject> boxGround = std::make_shared<GameObject>("GroundBox", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box));

			boxGround->setLocalPosition(x0, y0, z0);

			ModelManager::getInstance()->addObject(boxGround);

			if (j % 8 == 0)
			{
				vec3 randomPt = vec3(x0 + MathUtils::randomFloat(-x0, x0), y0 + MathUtils::randomFloat(250.0f, 350.0f), z0);
				std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);
				std::shared_ptr<Material> groundMetalMat = Material::Metallic(VectorUtils::randomFloatVec3(), MathUtils::randomFloat(0.0f, 0.4f));

				if (i % 2 == 0)
				{
					Model sphere4Model = Model::CreateSphere(randomPt, 20.0f, *groundMetalMat, false);
					std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
					ModelManager::getInstance()->addObject(sphere);
				}
				else
				{
					Model sphere4Model = Model::CreateSphere(randomPt, 45.0f, *groundReflectMat, false);
					std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>("ReflectedSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
					ModelManager::getInstance()->addObject(sphere);
				}

			}
		}
	}

	std::shared_ptr<Material> diffuseCheckerMat = Material::Lambertian(vec3(1), 3);
	Model textureSphere = Model::CreateSphere(vec3(-280, 280, 300), 160, *diffuseCheckerMat, false);
	std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>("CheckerSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(textureSphere));
	ModelManager::getInstance()->addObject(sphere);

	std::shared_ptr<Material> earthMat = Material::Lambertian(vec3(1), 4);
	Model earthModel = Model::CreateSphere(vec3(400, 400, 400), 200, *earthMat, false);
	std::shared_ptr<GameObject> earthObj = std::make_shared<GameObject>("EarthSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(earthModel));
	ModelManager::getInstance()->addObject(earthObj);

	std::shared_ptr<Material> metalMat = Material::Metallic(VectorUtils::randomFloatVec3(), 0.15f);
	Model metalModel = Model::CreateSphere(vec3(0, 450, 145), 50, *metalMat, false);
	std::shared_ptr<GameObject> metalObj = std::make_shared<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(metalModel));
	ModelManager::getInstance()->addObject(metalObj);

	std::shared_ptr<ObjectGroup> sphereGroup = std::make_shared<ObjectGroup>("SphereGroup");
	for (int i = 0; i < 1000; i++)
	{
		std::shared_ptr<Material> metalMat = Material::Metallic(vec3(0.73, 0.73, 0.73), MathUtils::randomFloat(0.0f, 0.5f));
		Model sphereInstance = Model::CreateSphere(VectorUtils::randomFloatVec3(0, 165), 10.0f, *metalMat, false);
		sphereGroup->addModel(std::make_shared<Model>(sphereInstance));
	}

	sphereGroup->setLocalRotation(0, -45, 0);
	sphereGroup->setLocalPosition(vec3(-200, 300, 450));
	ModelManager::getInstance()->addObject(sphereGroup);

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();

	// Add light objects
	std::shared_ptr<Light> pl1 = std::make_shared<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addObject(pl1);

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::AnitoTracer_DemoScene(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(0, 0, 1000), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.73, 0.73, 0.73) * 7.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(1000, 10, 1000), *areaLight);
	std::shared_ptr<GameObject> areaLightObject = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1000.0f, 500.0f);
	ModelManager::getInstance()->addObject(areaLightObject);

	std::shared_ptr<Camera> camObj = std::make_shared<Camera>("Camera");
	ModelManager::getInstance()->addObject(camObj);
	CameraManager::getInstance()->addCamera(camObj);
	camObj->setLocalPosition(0, 100, 5);

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = MaterialLibrary::getInstance()->getMaterial(L"Mirror");

	Model box0 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 165, 0), *white);
	Model box1 = Model::CreateBox(vec3(0, 0, -165), vec3(165, 330, 0), *white);

	std::shared_ptr<GameObject> box0_Object = std::make_shared<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box0));
	ModelManager::getInstance()->addObject(box0_Object);
	box0_Object->setLocalPosition(vec3(125, -194, -400));
	box0_Object->setLocalRotation(vec3(0, 50, 0));

	std::shared_ptr<GameObject> box1_Object = std::make_shared<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box1));
	ModelManager::getInstance()->addObject(box1_Object);
	box1_Object->setLocalPosition(vec3(-100, -112, -600));
	box1_Object->setLocalRotation(vec3(0, -60, 0));

	Model cornellBoxModel = Model::CreateCornellBox(555);
	std::shared_ptr<GameObject> cornellBoxObject = std::make_shared<GameObject>("CornellBox", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(cornellBoxModel));
	ModelManager::getInstance()->addObject(cornellBoxObject);
	cornellBoxObject->setLocalPosition(0, 0, -500);

	Model lucy = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj");
	std::shared_ptr<GameObject> lucyObject = std::make_shared<GameObject>("Lucy", GameObject::PrimitiveType::MESH, std::make_shared<Model>(lucy));
	ModelManager::getInstance()->addObject(lucyObject);
	lucyObject->setLocalPosition(vec3(-100, -170, -350));
	lucyObject->setLocalRotation(vec3(0, 90, 0));
	lucyObject->setLocalScale(vec3(0.25));

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Model_Showcase(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(0, 0, 800), vec3(0, 0, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.73, 0.73, 0.73) * 7.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(1000, 10, 1000), *areaLight);
	std::shared_ptr<GameObject> areaLightObject = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1000.0f, 500.0f);
	ModelManager::getInstance()->addObject(areaLightObject);

	std::shared_ptr<Camera> camObj = std::make_shared<Camera>("Camera");
	ModelManager::getInstance()->addObject(camObj);
	CameraManager::getInstance()->addCamera(camObj);
	camObj->setLocalPosition(0, 100, 5);

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = MaterialLibrary::getInstance()->getMaterial(L"Mirror");
	//const auto mirror = Material::Dielectric(1.6f, 0.0f); 

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Sponza(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(800, 400, -230), vec3(-350, 200, 65), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);
	//std::shared_ptr<Material> areaLight2 = Material::DiffuseLight(vec3(0, 0, 0.80) * 7.0f);
	//Model areaLightModel2 = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight2);

	//std::shared_ptr<Light> dl = std::make_shared<Light>("Light Source", Light::LightType::DirectionalLight);
	//dl->setLocalPosition(0,10000, 0);
	//dl->setAmbientColor(1, 1, 1, 0.05);
	//ModelManager::getInstance()->addLightObject(dl);

	std::shared_ptr<GameObject> areaLightObject = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotation(0, 0, 0);
	ModelManager::getInstance()->addObject(areaLightObject);
	 
	//std::shared_ptr<GameObject> areaLightObject2 = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel2));
	//areaLightObject2->setLocalPosition(-1500, 1000, -1500);
	//ModelManager::getInstance()->addObject(areaLightObject2);

	std::shared_ptr<Camera> cameraObj = std::make_shared<Camera>("Camera");
	ModelManager::getInstance()->addObject(cameraObj);
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj);

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);

	Model sphere4Model = Model::CreateSphere(vec3(0,0,0), 100.0f, *groundReflectMat, false);
	std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
	ModelManager::getInstance()->addObject(sphere);
	sphere->setLocalPosition(-350, 200, -45);

	Model sphere2Model = Model::CreateSphere(vec3(0, 0, 0), 75.0f, *mirror, false);
	std::shared_ptr<GameObject> sphere2 = std::make_shared<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere2Model));
	ModelManager::getInstance()->addObject(sphere2);
	sphere2->setLocalPosition(-500, 500, -45);


	/*auto rath = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/Rosa.blend");

	rath.Transform(
			rotate(
				scale(
					translate(i, vec3(0, 0, 0)),
					vec3(50.0f)),
				radians(105.0f), vec3(0, 1, 0)));

		std::shared_ptr<GameObject> rathObj;

		if (rath.GetName() == "")
			rathObj = std::make_shared<GameObject>("Sponza", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(rath));
		else
			rathObj = std::make_shared<GameObject>(rath.GetName(), GameObject::PrimitiveType::CUBE, std::make_shared<Model>(rath));

		ModelManager::getInstance()->addObject(rathObj);
		rathObj->setLocalPosition(100, 850, 0);*/
	 
	 
	std::vector<Model> sponza = Model::LoadModelGroup(FileUtils::getAssetsFolderPath().generic_string() + "/models/Sponza/sponza.obj");
	//sponza.SetMaterial(white);

	for (int j = 0; j < sponza.size(); j++) 
	{
		
		sponza[j].Transform(
			rotate(
				scale(
					translate(i, vec3(0, 0, 0)),
					vec3(1)),
				radians(0.0f), vec3(0, 1, 0)));

		std::shared_ptr<GameObject> sponzaObj;

		if (sponza[j].GetName() == "")
			sponzaObj = std::make_shared<GameObject>("Sponza" + j, GameObject::PrimitiveType::CUBE, std::make_shared<Model>(sponza[j]));
		else
			sponzaObj = std::make_shared<GameObject>(sponza[j].GetName(), GameObject::PrimitiveType::CUBE, std::make_shared<Model>(sponza[j]));

		ModelManager::getInstance()->addObject(sponzaObj);
		sponzaObj->setLocalPosition(0, 0, 0);
	}




	////Add light objects
	//std::shared_ptr<Light> pl1 = std::make_shared<Light>("Point Light 1", Light::LightType::PointLight);
	//ModelManager::getInstance()->addLightObject(pl1);

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_moon.jpg", Vulkan::SamplerConfig()));

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

SceneAssets SceneList::SanMiguel(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(800, 400, -230), vec3(-350, 200, 65), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	std::shared_ptr<GameObject> areaLightObject = std::make_shared<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotation(0, 0, 0);
	ModelManager::getInstance()->addObject(areaLightObject);

	std::shared_ptr<Camera> cameraObj = std::make_shared<Camera>("Camera");
	ModelManager::getInstance()->addObject(cameraObj);
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj);

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);

	Model sm = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/San_Miguel/san-miguel.obj");
	std::shared_ptr<GameObject> smObj;

	if (sm.GetName() == "")
		smObj = std::make_shared<GameObject>("San Miguel", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(sm));
	else
		smObj = std::make_shared<GameObject>(sm.GetName(), GameObject::PrimitiveType::CUBE, std::make_shared<Model>(sm));

	ModelManager::getInstance()->addObject(smObj);
	smObj->setLocalPosition(0, 0, 0);
	smObj->setLocalScale(50,50,50);

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}



SceneAssets SceneList::Empty(CameraInitialState& camera)
{
	camera.ModelView = lookAt(vec3(278, 278, 800), vec3(278, 278, 0), vec3(0, 1, 0));
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	std::vector<Model> models = ModelManager::getInstance()->getAllObjectModels();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(models), std::move(textures), std::move(lights));
}

std::vector<Assets::Texture> SceneList::AssembleTextureList()
{
	std::vector<Texture> textures;

	textures.push_back(TextureLibrary::getInstance()->getTexture("2k_mars"));//(L"2k_mars"));
	textures.push_back(TextureLibrary::getInstance()->getTexture("2k_moon"));
	textures.push_back(TextureLibrary::getInstance()->getTexture("land_ocean_ice_cloud_2048"));
	textures.push_back(TextureLibrary::getInstance()->getTexture("checker"));
	textures.push_back(TextureLibrary::getInstance()->getTexture("earthmap"));

	return textures;
}
