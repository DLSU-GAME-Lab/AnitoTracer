#include "CameraManager.h"

#include <iostream>

#include "SceneCamera.h"

CameraManager* CameraManager::P_SHARED_INSTANCE = NULL;

std::shared_ptr<Camera> CameraManager::getActiveCamera()
{
	//TODO : if (EngineState != PLAY)
	if (this->mainCamera != nullptr)
		return this->mainCamera;
	else
		return this->selectedSceneCamera;
}

std::shared_ptr<Camera> CameraManager::findCameraByName(std::string name)
{
	if (this->cameraTable[name] != nullptr) {
		return this->cameraTable[name];
	}
	else {
		return nullptr;
	}
}

std::vector<std::shared_ptr<SceneCamera>> CameraManager::getSceneCameras()
{
	return this->sceneCameraList;
}

void CameraManager::setMainCamera(std::shared_ptr<Camera> camera)
{
	mainCamera = camera;
}

void CameraManager::setSceneCameraProjection(int type)
{
	this->selectedSceneCamera->SetProjectionType((Camera::ProjectionMode)type);
}

void CameraManager::updateSceneCamera(float deltaTime)
{
	this->selectedSceneCamera->UpdateCamera(1, deltaTime);
}

void CameraManager::addCamera(std::shared_ptr<Camera> camera)
{
	this->cameraList.push_back(camera);
	this->cameraTable[camera->getName()] = camera;
}

void CameraManager::addSceneCamera(std::shared_ptr<SceneCamera> camera)
{
	if (this->selectedSceneCamera == NULL)
		this->selectedSceneCamera = camera;

	this->sceneCameraList.push_back(camera);
	this->cameraTable[camera->getName()] = camera;
}

void CameraManager::removeSceneCamera(std::shared_ptr<SceneCamera> camera)
{
	int index = -1;

	for (int i = 0; i < this->sceneCameraList.size() && index == -1; i++)
	{
		if (this->sceneCameraList[i] == camera)
			index = i;
	}

	if (index != -1)
	{
		this->sceneCameraList.erase(this->sceneCameraList.begin() + index);
	}
}

void CameraManager::removeCamera(std::shared_ptr<Camera> camera)
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
	this->selectedSceneCamera = std::make_shared<SceneCamera>("Scene Camera");
	this->mainCamera = nullptr;
	this->addCamera(this->selectedSceneCamera);
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
