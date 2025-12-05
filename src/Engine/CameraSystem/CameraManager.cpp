#include "CameraManager.h"
#include "SceneCamera.h"

CameraManager* CameraManager::P_SHARED_INSTANCE = NULL;

Camera* CameraManager::getActiveCamera()
{
	//TODO : if (EngineState != PLAY)
	if (this->mainCamera != nullptr)
		return this->mainCamera;
	else
		return this->selectedSceneCamera;
}

Camera* CameraManager::findCameraByName(std::string name)
{
	if (this->cameraTable[name] != nullptr) {
		return this->cameraTable[name];
	}
	else {
		return nullptr;
	}
}

std::vector<SceneCamera*> CameraManager::getSceneCameras()
{
	std::vector<SceneCamera*> result;

	for (const auto& camera : this->sceneCameraList)
	{
		result.push_back(camera.get());
	}

	return result;
}

void CameraManager::setMainCamera(Camera* camera)
{
	mainCamera = camera;
}

void CameraManager::setSceneCameraProjection(int type)
{
	this->selectedSceneCamera->SetProjectionType((Camera::ProjectionMode)type);
}

void CameraManager::updateSceneCamera(float deltaTime)
{
	this->selectedSceneCamera->Update(1, deltaTime);
}

void CameraManager::addCamera(Camera* camera)
{
	this->cameraList.push_back(camera);
	this->cameraTable[camera->GetName()] = camera;
}

void CameraManager::addSceneCamera(std::unique_ptr<SceneCamera> camera)
{
	if (this->selectedSceneCamera == NULL)
		this->selectedSceneCamera = camera.get();

	this->cameraTable[camera->GetName()] = camera.get();
	this->sceneCameraList.push_back(std::move(camera));

}

void CameraManager::removeSceneCamera(SceneCamera* camera)
{
	int index = -1;

	for (int i = 0; i < this->sceneCameraList.size() && index == -1; i++)
	{
		if (this->sceneCameraList[i].get() == camera)
			index = i;
	}

	if (index != -1)
	{
		this->sceneCameraList.erase(this->sceneCameraList.begin() + index);
	}
}

void CameraManager::removeCamera(Camera* camera)
{
	int index = -1;

	for (int i = 0; i < this->cameraList.size() && index == -1; i++)
	{
		if (this->cameraList[i] == camera)
			index = i;
	}

	if (index != -1)
	{
		this->cameraList.erase(this->cameraList.begin() + index);
	}
}

CameraManager::CameraManager()
{
	auto sceneCam = std::make_unique<SceneCamera>("Scene Camera");
	this->selectedSceneCamera = sceneCam.get();
	this->mainCamera = nullptr;
	this->addSceneCamera(std::move(sceneCam));
}

CameraManager::~CameraManager()
{
	P_SHARED_INSTANCE = nullptr;
}

CameraManager* CameraManager::getInstance() {
	return P_SHARED_INSTANCE;
}

void CameraManager::initialize()
{
	if (P_SHARED_INSTANCE) {}
	P_SHARED_INSTANCE = new CameraManager();
}

void CameraManager::destroy()
{
	if (P_SHARED_INSTANCE != NULL)
	{
		delete P_SHARED_INSTANCE;
	}
}

void CameraManager::onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters)
{
	if (eventName == EventNames::ON_CAMERA_ADDED)
	{
		Camera* camera = reinterpret_cast<Camera*>(parameters->getIntData("CAMERA_PTR", 0));
		this->addCamera(camera);
	}
	else if (eventName == EventNames::ON_CAMERA_REMOVED)
	{
		Camera* camera = reinterpret_cast<Camera*>(parameters->getIntData("CAMERA_PTR", 0));
		this->removeCamera(camera);
	}
}
