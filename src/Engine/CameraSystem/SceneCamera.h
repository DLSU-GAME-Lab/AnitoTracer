#pragma once
#include "Camera.h"
#include "HotkeySystem/HotkeyListener.hpp"

class SceneCamera : public Camera, public HotkeyListener
{
public:
	enum CameraMoveMode { NONE = -1, FPS = 0, PAN, FASTPAN, SLOWPAN, ZOOM, ORBIT };

	SceneCamera(std::string name);
	SceneCamera(const SceneCamera& other);
	~SceneCamera();

	GameObjectPtr Clone() const override;

	bool OnCursorPosition(double xpos, double ypos);
	bool Update(double speed, double timeDelta);

	void OnActionPressed(Hotkey::Action action) override;
	void OnActionReleased(Hotkey::Action action) override;

private:
	void HandleFPSRotation(float deltaX, float deltaY);
	void HandlePan(float dx, float dy);
	void HandleZoom(float dx, float dy);
	void HandleOrbit(float dx, float dy);

	// Movement flags
	bool cameraMovingForward_ = false;
	bool cameraMovingBackward_ = false;
	bool cameraMovingLeft_ = false;
	bool cameraMovingRight_ = false;
	bool cameraMovingUp_ = false;
	bool cameraMovingDown_ = false;

	// Speed modifiers
	bool camSpedUp = false;
	bool camSlowed = false;

	// Camera parameters
	float camSpeed_ = 5.0f;
	float orbitSpeed_ = 0.5f;
	float m_defaultPivotDistance = 1000.0f;

	// Orbit state
	float orbitYaw_ = 0.0f;
	float orbitPitch_ = 0.0f;
	glm::vec3 orbitPivot_ = glm::vec3(0.0f);

	// Mouse state
	double mousePosX_ = 0.0;
	double mousePosY_ = 0.0;

	CameraMoveMode m_currentMode = NONE;
};

