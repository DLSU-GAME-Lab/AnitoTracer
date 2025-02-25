#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include "Camera.h"
#include "SceneCamera.h"

class CameraManager
{
private:
    typedef std::vector<std::shared_ptr<Camera>> CameraList;
    typedef std::unordered_map<std::string, std::shared_ptr<Camera>> CameraTable;
    typedef std::vector<std::shared_ptr<SceneCamera>> SceneCameraList;

private:
    std::shared_ptr<SceneCamera> selectedSceneCamera;
    std::shared_ptr<Camera> mainCamera;
    CameraList cameraList;
    CameraTable cameraTable;
    SceneCameraList sceneCameraList;

public:
    std::shared_ptr<Camera> getActiveCamera();
    std::vector<std::shared_ptr<SceneCamera>> getSceneCameras();
    std::shared_ptr<Camera> findCameraByName(std::string name);
    void setMainCamera(std::shared_ptr<Camera> camera);
    void setSceneCameraProjection(int type);

    void updateSceneCamera(float deltaTime);
    void addCamera(std::shared_ptr<Camera> camera);
    void addSceneCamera(std::shared_ptr<SceneCamera> camera);
    void removeSceneCamera(std::shared_ptr<SceneCamera> camera);
    void removeCamera(std::shared_ptr<Camera> camera);

private:
    static CameraManager* P_SHARED_INSTANCE;

private:
    CameraManager();
    ~CameraManager();
    CameraManager(const CameraManager&) {}
    CameraManager& operator = (const CameraManager&);

public:
    static CameraManager* getInstance();
    static void initialize();
    static void destroy();
};
