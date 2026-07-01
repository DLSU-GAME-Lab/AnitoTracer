#include "SceneList.hpp"
#include "Assets/Material.hpp"
#include "Assets/Model.hpp"
#include "Assets/Texture.hpp"
#include <functional>
#include <iostream>
#include <random>
#include <memory>

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

#include "Assets/GameObjectFactory.hpp"

using namespace glm;
using Assets::Material;
using Assets::Model;
using Assets::Texture;

namespace
{
	void UpdateCameraObject(const glm::vec3& cameraPos, const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0))
	{
		auto cameraObject = CameraManager::getInstance()->getActiveCamera();
		if (!cameraObject)	return;

		cameraObject->setLocalPosition(cameraPos);

		glm::vec3 direction = glm::normalize(target - cameraPos);

		float yaw = glm::degrees(atan2(direction.x, direction.z));
		float pitch = glm::degrees(asin(-direction.y));
		float roll = 0.0f;

		glm::vec3 rotation(pitch, yaw, roll);
		cameraObject->setLocalRotationEuler(rotation);
	}

	void AddRayTracingInOneWeekendCommonScene(const bool& isProc, std::function<float()>& random)
	{
		// Common models from the final scene from Ray Tracing In One Weekend book. Only the three central spheres are missing.
		// Calls to random() are always explicit and non-inlined to avoid C++ undefined evaluation order of function arguments,
		// this guarantees consistent and reproducible behaviour across different platforms and compilers.

		{
			Model m = Model::CreateSphere(vec3(0, -1000, 0), 1000, *Material::Lambertian(vec3(0.5f, 0.5f, 0.5f)), isProc);
			auto go = std::make_unique<GameObject>("GroundSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
			ModelManager::getInstance()->addObject(std::move(go));
		}

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

						Model m = Model::CreateSphere(center, 0.2f, *Material::Lambertian(vec3(r, g, b)), isProc);
						auto go = std::make_unique<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
						ModelManager::getInstance()->addObject(std::move(go));
					}
					else if (chooseMat < 0.95f) // Metal
					{
						const float fuzziness = 0.5f * random();
						const float b = 0.5f * (1 + random());
						const float g = 0.5f * (1 + random());
						const float r = 0.5f * (1 + random());

						Model m = Model::CreateSphere(center, 0.2f, *Material::Metallic(vec3(r, g, b), fuzziness), isProc);
						auto go = std::make_unique<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
						ModelManager::getInstance()->addObject(std::move(go));
					}
					else // Glass
					{
						Model m = Model::CreateSphere(center, 0.2f, *Material::Dielectric(1.5f), isProc);
						auto go = std::make_unique<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
						ModelManager::getInstance()->addObject(std::move(go));
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
	{"Vokselia", Vokselia}, // USED
	{"Breakfast Room", BreakfastRoom}, // USED
	{"Salle De Bain", SalleDeBain}, 
	{"Gallery", Gallery}, // USED// USED
	{"Bistro_EXT", BistroEXT}, // USED// USED
	{"Bistro_INT", BistroINT}, // USED// USED
	{"balay_anito", BalayAnito}, // USED// USED
	{"Empty", Empty}, // USED
};

SceneAssets SceneList::CubeAndSpheres(CameraInitialState& camera)
{
	// Basic test scene.
	glm::vec3 cameraPos = { 0.0f, 0.0f, -2 };
	glm::vec3 target = { 0.0f, 0.0f, 0.0f };
	glm::vec3 up = { 0.0f, 1.0f, 0.0f };

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 90;
	camera.Aperture = 0.05f;
	camera.FocusDistance = 2.0f;
	camera.ControlSpeed = 2.0f;
	camera.GammaCorrection = false;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::vector<GameObject*> gameObjects;
	std::vector<Texture> textures;

	auto model = std::make_shared<Model>(Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/cube_multi.obj"));
	auto cube = GameObjectFactory::CreateEmpty();
	cube->setModel(std::move(model));
	ModelManager::getInstance()->addObject(std::move(cube));

	model = std::make_shared<Model>(Model::CreateSphere(vec3(1, 0, 0), 0.5, *Material::Metallic(vec3(0.7f, 0.5f, 0.8f), 0.2f), true));
	auto metalSphere = GameObjectFactory::CreateEmpty();
	metalSphere->setModel(std::move(model));
	ModelManager::getInstance()->addObject(std::move(metalSphere));

	model = std::make_shared<Model>(Model::CreateSphere(vec3(-1, 0, 0), 0.5, *Material::Dielectric(1.5f), true));
	auto dielecSphere = GameObjectFactory::CreateEmpty();
	dielecSphere->setModel(std::move(model));
	ModelManager::getInstance()->addObject(std::move(dielecSphere));

	model = std::make_shared<Model>(Model::CreateSphere(vec3(0, 1, 0), 0.5, *Material::Lambertian(vec3(1.0f), 0), true));
	auto lambSphere = GameObjectFactory::CreateEmpty();
	lambSphere->setModel(std::move(model));
	ModelManager::getInstance()->addObject(std::move(lambSphere));

	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/land_ocean_ice_cloud_2048.png", Vulkan::SamplerConfig()));

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::vector<Texture>(), std::move(lights));
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

	AddRayTracingInOneWeekendCommonScene(isProc, random);

	// add three central spheres
	{
		Model m = Model::CreateSphere(vec3(0, 1, 0), 1.0f, *Material::Dielectric(1.5f), isProc);
		auto go = std::make_unique<GameObject>("CenterDielectric", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		Model m = Model::CreateSphere(vec3(-4, 1, 0), 1.0f, *Material::Lambertian(vec3(0.4f, 0.2f, 0.1f)), isProc);
		auto go = std::make_unique<GameObject>("LeftLambert", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		Model m = Model::CreateSphere(vec3(4, 1, 0), 1.0f, *Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.0f), isProc);
		auto go = std::make_unique<GameObject>("RightMetal", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::vector<Texture>(), std::move(lights));
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

	AddRayTracingInOneWeekendCommonScene(isProc, random);

	{
		Model m = Model::CreateSphere(vec3(0, 1, 0), 1.0f, *Material::Metallic(vec3(1.0f), 0.1f, 2), isProc);
		auto go = std::make_unique<GameObject>("CenterMetallic", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		Model m = Model::CreateSphere(vec3(-4, 1, 0), 1.0f, *Material::Lambertian(vec3(1.0f), 0), isProc);
		auto go = std::make_unique<GameObject>("LeftLambert", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		Model m = Model::CreateSphere(vec3(4, 1, 0), 1.0f, *Material::Metallic(vec3(1.0f), 0.0f, 1), isProc);
		auto go = std::make_unique<GameObject>("RightMetallic", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(m));
		ModelManager::getInstance()->addObject(std::move(go));
	}

	std::vector<Texture> textures;
	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_mars.jpg", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_moon.jpg", Vulkan::SamplerConfig()));
	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/land_ocean_ice_cloud_2048.png", Vulkan::SamplerConfig()));

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	auto gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
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

	AddRayTracingInOneWeekendCommonScene(isProc, random);

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

	{
		auto go = std::make_unique<GameObject>("Lucy_Diel", GameObject::PrimitiveType::MESH, std::make_shared<Model>(std::move(lucy0)));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		auto go = std::make_unique<GameObject>("Lucy_Lambert", GameObject::PrimitiveType::MESH, std::make_shared<Model>(std::move(lucy1)));
		ModelManager::getInstance()->addObject(std::move(go));
	}
	{
		auto go = std::make_unique<GameObject>("Lucy_Metal", GameObject::PrimitiveType::MESH, std::make_shared<Model>(std::move(lucy2)));
		ModelManager::getInstance()->addObject(std::move(go));
	}

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::CornellBox(CameraInitialState& camera)
{
	glm::vec3 cameraPos(278, 278, 800);
	glm::vec3 target(278, 278, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");

	auto box0Model = Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *white);
	auto box1Model = Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *white);

	box0Model.Transform(rotate(translate(i, vec3(555 - 130 - 165, 0, -65)), radians(-18.0f), vec3(0, 1, 0)));
	box1Model.Transform(rotate(translate(i, vec3(555 - 265 - 165, 0, -295)), radians(15.0f), vec3(0, 1, 0)));

	auto cornellModel = Model::CreateCornellBox(555);

	auto goCornell = std::make_unique<GameObject>("CornellBox", GameObject::PrimitiveType::CORNELL_BOX, std::make_shared<Model>(cornellModel));
	auto goBox0 = std::make_unique<GameObject>("Box0", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box0Model));
	auto goBox1 = std::make_unique<GameObject>("Box1", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box1Model));

	ModelManager::getInstance()->addObject(std::move(goCornell));
	ModelManager::getInstance()->addObject(std::move(goBox0));
	ModelManager::getInstance()->addObject(std::move(goBox1));

	// Add light objects
	auto pl = std::make_unique<Light>("Center Light", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::vector<Texture>(), std::move(lights));
}

SceneAssets SceneList::CornellBoxLucy(CameraInitialState& camera)
{
	glm::vec3 cameraPos(278, 278, 800);
	glm::vec3 target(278, 278, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	const auto i = mat4(1);
	const auto sphereModel = Model::CreateSphere(vec3(555 - 130, 165.0f, -165.0f / 2 - 65), 80.0f, *Material::Dielectric(1.5f), true);
	auto lucyModel = Model::LoadModel(FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj");

	lucyModel.Transform(
		rotate(
			scale(
				translate(i, vec3(555 - 300 - 165 / 2, -9, -295 - 165 / 2)),
				vec3(0.6f)),
			radians(75.0f), vec3(0, 1, 0)));

	auto goCornell = std::make_unique<GameObject>("CornellBox", GameObject::PrimitiveType::CORNELL_BOX, std::make_shared<Model>(Model::CreateCornellBox(555)));
	auto goSphere = std::make_unique<GameObject>("LargeSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphereModel));
	auto goLucy = std::make_unique<GameObject>("Lucy", GameObject::PrimitiveType::MESH, std::make_shared<Model>(lucyModel));

	ModelManager::getInstance()->addObject(std::move(goCornell));
	ModelManager::getInstance()->addObject(std::move(goSphere));
	ModelManager::getInstance()->addObject(std::move(goLucy));

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::vector<Texture>(), std::move(lights));
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
	auto sphere1 = std::make_unique<GameObject>("GroundSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere1Model));
	ModelManager::getInstance()->addObject(std::move(sphere1));

	pos = vec3(0, 1, 0); center = 1.0f;
	Model sphere2Model = Model::CreateSphere(pos, 1.0f, *Material::Dielectric(1.5f), isProcedural);
	auto sphere2 = std::make_unique<GameObject>("CenterSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere2Model));
	ModelManager::getInstance()->addObject(std::move(sphere2));

	pos = vec3(-8, 2.5f, 1); center = 2.5f;
	Model sphere3Model = Model::CreateSphere(pos, 2.5f, *Material::Metallic(vec3(0.4f, 0.2f, 0.1f), MathUtils::randomFloat(0.0f, 0.2f)), isProcedural);
	auto sphere3 = std::make_unique<GameObject>("LeftSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere3Model));
	ModelManager::getInstance()->addObject(std::move(sphere3));

	pos = vec3(4, 1, 0); center = 1.0f;
	Model sphere4Model = Model::CreateSphere(pos, 1.0f, *Material::Metallic(vec3(0.7f, 0.6f, 0.5f), 0.0f), isProcedural);
	auto sphere4 = std::make_unique<GameObject>("RightSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
	ModelManager::getInstance()->addObject(std::move(sphere4));

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
					auto objectInstance = std::make_unique<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(modelInstance));
					ModelManager::getInstance()->addObject(std::move(objectInstance));
				}
			}
		}

		for (int a = -5; a < 5; a++)
		{
			for (int b = -5; b < 5; b++)
			{
				vec3 center(a + 0.9f * MathUtils::randomFloat(), 0.2 + (5 * MathUtils::randomFloat()), b + 0.9 * MathUtils::randomFloat());

				std::shared_ptr<Material> materialInstance = Material::Dielectric(1.5f);
				Model modelInstance = Model::CreateSphere(center, MathUtils::randomFloat(0.1f, 0.2f), *materialInstance, isProcedural);
				auto objectInstance = std::make_unique<GameObject>("SmallSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(modelInstance));
				ModelManager::getInstance()->addObject(std::move(objectInstance));
			}
		}
	}

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();

	// Add light objects
	auto pl = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::GDGRAP2_CornellBox(CameraInitialState& camera)
{
	glm::vec3 cameraPos(0, 0, 1800);
	glm::vec3 target(0, 0, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	auto box0 = GameObjectFactory::CreateCube("Right Box");
	box0->setLocalPosition(vec3(125, -194, 100));
	box0->setLocalRotationEuler(vec3(0, 50, 0));
	box0->setLocalScale(glm::vec3(3, 3.3f, 3));
	ModelManager::getInstance()->addObject(std::move(box0));

	auto box1 = GameObjectFactory::CreateCube("Tall Box");
	box1->setLocalPosition(vec3(-100, -112, -100));
	box1->setLocalRotationEuler(vec3(0, -60, 0));
	box1->setLocalScale(glm::vec3(3, 6.6f, 3));
	ModelManager::getInstance()->addObject(std::move(box1));

	auto cornellBoxObject = GameObjectFactory::CreateCornellBox();
	ModelManager::getInstance()->addObject(std::move(cornellBoxObject));

	// Add light objects
	auto pl = GameObjectFactory::CreateLight(Light::LightType::PointLight, "Sample Point Light");
	ModelManager::getInstance()->addLightObject(std::move(pl));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
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
	auto boxGround = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	boxGround->setLocalPosition(-250.0f, 600.0f, -500.0f);
	ModelManager::getInstance()->addObject(std::move(boxGround));

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

			std::shared_ptr<Material> groundMat = Material::Metallic(VectorUtils::randomFloatVec3(), MathUtils::randomFloat(0.0f, 0.25f));
			Model box = Model::CreateBox(vec3(x0, y0, z0), vec3(x1, y1, z1), *groundMat);
			auto boxGround = std::make_unique<GameObject>("GroundBox", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box));

			boxGround->setLocalPosition(x0, y0, z0);

			ModelManager::getInstance()->addObject(std::move(boxGround));

			if (j % 8 == 0)
			{
				vec3 randomPt = vec3(x0 + MathUtils::randomFloat(-x0, x0), y0 + MathUtils::randomFloat(250.0f, 350.0f), z0);
				std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);
				std::shared_ptr<Material> groundMetalMat = Material::Metallic(VectorUtils::randomFloatVec3(), MathUtils::randomFloat(0.0f, 0.4f));

				if (i % 2 == 0)
				{
					Model sphere4Model = Model::CreateSphere(randomPt, 20.0f, *groundMetalMat, false);
					auto sphere = std::make_unique<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
					ModelManager::getInstance()->addObject(std::move(sphere));
				}
				else
				{
					Model sphere4Model = Model::CreateSphere(randomPt, 45.0f, *groundReflectMat, false);
					auto sphere = std::make_unique<GameObject>("ReflectedSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphere4Model));
					ModelManager::getInstance()->addObject(std::move(sphere));
				}

			}
		}
	}

	std::shared_ptr<Material> diffuseCheckerMat = Material::Lambertian(vec3(1), 3);
	Model textureSphere = Model::CreateSphere(vec3(-280, 280, 300), 160, *diffuseCheckerMat, false);
	auto sphere = std::make_unique<GameObject>("CheckerSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(textureSphere));
	ModelManager::getInstance()->addObject(std::move(sphere));

	std::shared_ptr<Material> earthMat = Material::Lambertian(vec3(1), 4);
	Model earthModel = Model::CreateSphere(vec3(400, 400, 400), 200, *earthMat, false);
	auto earthObj = std::make_unique<GameObject>("EarthSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(earthModel));
	ModelManager::getInstance()->addObject(std::move(earthObj));

	std::shared_ptr<Material> metalMat = Material::Metallic(VectorUtils::randomFloatVec3(), 0.15f);
	Model metalModel = Model::CreateSphere(vec3(0, 450, 145), 50, *metalMat, false);
	auto metalObj = std::make_unique<GameObject>("MetalSphere", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(metalModel));
	ModelManager::getInstance()->addObject(std::move(metalObj));

	auto sphereGroup = GameObjectFactory::CreateEmpty("SphereGroup");

	for (int i = 0; i < 1000; i++)
	{
		auto metalMat = Material::Metallic(vec3(0.73, 0.73, 0.73), MathUtils::randomFloat(0.0f, 0.5f));
		Model sphereInstance = Model::CreateSphere(VectorUtils::randomFloatVec3(0, 165), 10.0f, *metalMat, false);
		sphereGroup->addChild(std::make_unique<GameObject>("SmallSphereInstance", GameObject::PrimitiveType::SPHERE, std::make_shared<Model>(sphereInstance)));
	}

	sphereGroup->setLocalRotationEuler(0, -45, 0);
	sphereGroup->setLocalPosition(vec3(-200, 300, 450));
	ModelManager::getInstance()->addObject(std::move(sphereGroup));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();

	// Add light objects
	auto pl1 = std::make_unique<Light>("Point Light 1", Light::LightType::PointLight);
	ModelManager::getInstance()->addLightObject(std::move(pl1));

	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::AnitoTracer_DemoScene(CameraInitialState& camera)
{
	glm::vec3 cameraPos(0, 0, 1000);
	glm::vec3 target(0, 0, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.73, 0.73, 0.73) * 7.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(1000, 10, 1000), *areaLight);
	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1000.0f, 500.0f);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	auto box0 = GameObjectFactory::CreateCube("Right Box");
	box0->setLocalPosition(vec3(125, -194, -400));
	box0->setLocalRotationEuler(vec3(0, 50, 0));
	box0->setLocalScale(glm::vec3(3, 3.3f, 3));
	ModelManager::getInstance()->addObject(std::move(box0));

	auto box1 = GameObjectFactory::CreateCube("Tall Box");
	box1->setLocalPosition(vec3(-100, -112, -500));
	box1->setLocalRotationEuler(vec3(0, -60, 0));
	box1->setLocalScale(glm::vec3(3, 6.6f, 3));
	ModelManager::getInstance()->addObject(std::move(box1));

	auto cornellBoxObject = GameObjectFactory::CreateCornellBox();
	cornellBoxObject->setLocalPosition(0, 0, -500);
	ModelManager::getInstance()->addObject(std::move(cornellBoxObject));

	auto lucy = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/lucy.obj", "Lucy");
	lucy->setLocalPosition(vec3(-100, -170, -350));
	lucy->setLocalRotationEuler(vec3(0, 90, 0));
	lucy->setLocalScale(vec3(0.25));

	ModelManager::getInstance()->addObject(std::move(lucy));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Model_Showcase(CameraInitialState& camera)
{
	glm::vec3 cameraPos(0, 0, 800);
	glm::vec3 target(0, 0, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.73, 0.73, 0.73) * 7.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(1000, 10, 1000), *areaLight);
	std::unique_ptr<GameObject> areaLightObject = GameObjectFactory::CreateEmpty("AreaLight");
	areaLightObject->setLocalPosition(0, 1000.0f, 500.0f);
	areaLightObject->setModel(std::make_shared<Model>(areaLightModel));
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = MaterialLibrary::getInstance()->getMaterial(L"Mirror");

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Sponza(CameraInitialState& camera)
{
	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = GameObjectFactory::CreateEmpty("AreaLight");
	areaLightObject->setModel(std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/Sponza/sponza.obj", "sponza");
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	textures.push_back(Texture::LoadTexture(FileUtils::getAssetsFolderPath().generic_string() + "/textures/2k_moon.jpg", Vulkan::SamplerConfig()));

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::SanMiguel(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	std::unique_ptr<Camera> cameraObj = std::make_unique<Camera>("Camera");
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj.get());
	ModelManager::getInstance()->addObject(std::move(cameraObj));

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/San_Miguel/san-miguel.obj", "san-miguel");
	gameObject->setLocalScale(20.0f, 20.0f, 20.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Empty(CameraInitialState& camera)
{
	glm::vec3 cameraPos(278, 278, 800);
	glm::vec3 target(278, 278, 0);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Vokselia(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	std::unique_ptr<Camera> cameraObj = std::make_unique<Camera>("Camera");
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj.get());
	ModelManager::getInstance()->addObject(std::move(cameraObj));

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/vokselia_spawn/vokselia_spawn.obj", "vokselia");
	gameObject->setLocalScale(10.0f, 10.0f, 10.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::BistroEXT(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/Bistro/obj/exterior.obj", "Bistro");
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::BistroINT(CameraInitialState& camera)
{
	glm::vec3 cameraPos(-179.904, 240.910, -112.142);
	glm::vec3 target(1089, 196.492, -68.968);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/Bistro/obj/interior.obj", "Bistro");
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::BalayAnito(CameraInitialState& camera)
{
	glm::vec3 cameraPos(-185.5, 81.2, -155.5);
	glm::vec3 target(-0.678, 1.282, 6.784);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::mt19937 engine(1);
	std::function<float()> random = std::bind(std::uniform_real_distribution<float>(), engine);

	bool isProcedural = false;

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);

	auto path = FileUtils::getAssetsFolderPath().generic_string() + "/models/balay_anito/ANITO_Archvis_Scene_New.obj";

	auto gameObject = GameObjectFactory::CreateFromModelFile(path, "balay_anito");
	gameObject->setLocalScale(50.0f, 50.0f, 50.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::BreakfastRoom(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	std::unique_ptr<Camera> cameraObj = std::make_unique<Camera>("Camera");
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj.get());
	ModelManager::getInstance()->addObject(std::move(cameraObj));

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/breakfast_room/breakfast_room.obj", "breakfast_room");
	gameObject->setLocalScale(10.0f, 10.0f, 10.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::SalleDeBain(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	std::unique_ptr<Camera> cameraObj = std::make_unique<Camera>("Camera");
	ModelManager::getInstance()->addObject(std::move(cameraObj));
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj.get());

	const auto i = mat4(1);
	const auto white = MaterialLibrary::getInstance()->getMaterial(L"White");
	const auto mirror = Material::Metallic(vec3(0.1f, 0.1f, 0.1f), 0.0f);
	const auto clear_mirror = Material::Metallic(vec3(1.0f, 1.0f, 1.0f), 0.0f);
	std::shared_ptr<Material> groundReflectMat = Material::Dielectric(1.5f);


	Model box0 = Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *clear_mirror);
	Model box1 = Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *clear_mirror);
	Model box2 = Model::CreateBox(vec3(0, 0, -50), vec3(50, 50, 0), *mirror);

	auto box0_Object = std::make_unique<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box0));
	ModelManager::getInstance()->addObject(std::move(box0_Object));
	box0_Object = nullptr;

	auto box1_Object = std::make_unique<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box1));
	ModelManager::getInstance()->addObject(std::move(box1_Object));
	box1_Object = nullptr;

	auto box2_Object = std::make_unique<GameObject>("Box", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(box2));
	ModelManager::getInstance()->addObject(std::move(box2_Object));
	box2_Object = nullptr;

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/salle_de_bain/salle_de_bain.obj", "SalleDeBain");
	gameObject->setLocalScale(10.0f, 10.0f, 10.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
}

SceneAssets SceneList::Gallery(CameraInitialState& camera)
{
	glm::vec3 cameraPos(800, 400, -230);
	glm::vec3 target(-350, 200, 65);
	glm::vec3 up(0, 1, 0);

	camera.ModelView = lookAt(cameraPos, target, up);
	camera.FieldOfView = 40;
	camera.Aperture = 0.0f;
	camera.FocusDistance = 10.0f;
	camera.ControlSpeed = 500.0f;
	camera.GammaCorrection = true;
	camera.HasSky = true;

	UpdateCameraObject(cameraPos, target, up);

	std::shared_ptr<Material> areaLight = Material::DiffuseLight(vec3(0.7, 0.7, 0.7) * 10.0f);
	Model areaLightModel = Model::CreateBox(vec3(0, 0, 0), vec3(2000, 10, 2000), *areaLight);

	auto areaLightObject = std::make_unique<GameObject>("AreaLight", GameObject::PrimitiveType::CUBE, std::make_shared<Model>(areaLightModel));
	areaLightObject->setLocalPosition(0, 1500, -500);
	areaLightObject->setLocalRotationEuler(0, 0, 0);
	ModelManager::getInstance()->addObject(std::move(areaLightObject));

	std::unique_ptr<Camera> cameraObj = std::make_unique<Camera>("Camera");
	cameraObj->setLocalPosition(0, 10.0f, 0);
	CameraManager::getInstance()->addCamera(cameraObj.get());
	ModelManager::getInstance()->addObject(std::move(cameraObj));

	auto gameObject = GameObjectFactory::CreateFromModelFile(FileUtils::getAssetsFolderPath().generic_string() + "/models/gallery/gallery.obj", "Gallery");
	gameObject->setLocalScale(10.0f, 10.0f, 10.0f);
	ModelManager::getInstance()->addObject(std::move(gameObject));

	std::vector<GameObject*> gameobjects = ModelManager::getInstance()->getObjectList();
	std::vector<Texture> textures = TextureLibrary::getInstance()->getTextureLibraryList();
	std::vector<Assets::LightProperties> lights = ModelManager::getInstance()->getAllLightProperties();

	return std::forward_as_tuple(std::move(gameobjects), std::move(textures), std::move(lights));
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
