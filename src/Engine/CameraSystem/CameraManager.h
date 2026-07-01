#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "Camera.h"
#include "SceneCamera.h"

class CameraManager
{
private:
    typedef std::vector<Camera*> CameraList;
    typedef std::unordered_map<std::string, Camera*> CameraTable;
    typedef std::vector<std::unique_ptr<SceneCamera>> SceneCameraList;

private:
    SceneCamera* selectedSceneCamera;
    Camera* mainCamera;
    CameraList cameraList;
    CameraTable cameraTable;
    SceneCameraList sceneCameraList;

public:
    Camera* getActiveCamera();
    std::vector<SceneCamera*> getSceneCameras();
    Camera* findCameraByName(std::string name);
    void setMainCamera(Camera* camera);
    void setSceneCameraProjection(int type);

    void updateSceneCamera(float deltaTime);
    void addCamera(Camera* camera);
    void addSceneCamera(std::unique_ptr<SceneCamera> camera);
    void removeSceneCamera(SceneCamera* camera);
    void removeCamera(Camera* camera);

private:
    static CameraManager* P_SHARED_INSTANCE;

private:
    CameraManager();
    ~CameraManager();
    CameraManager(const CameraManager&) {}
    CameraManager& operator = (const CameraManager&) {}

public:
    static CameraManager* getInstance();
    static void initialize();
    static void destroy();
};
