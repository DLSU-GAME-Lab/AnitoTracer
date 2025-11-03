#pragma once

#include <string>

#include "UserSettings.hpp"
#include "From-GDGRAP2/GameObject.h"
#include "Utilities/Glm.hpp"
#include "Utilities/HotkeyListener.hpp"

class Camera : public GameObject, public HotkeyListener
{
public:
	enum ProjectionMode { orthographic = 0, perspective };
	enum CameraMode { NONE = -1, FPS = 0, PAN, FASTPAN, SLOWPAN, ZOOM, ORBIT };

	Camera(std::string name, ProjectionMode proj = perspective);
	~Camera();

	void Reset(const glm::mat4& modelView);

	glm::mat4 ModelView();

	bool OnKey(int key, int scancode, int action, int mods);
	bool OnCursorPosition(double xpos, double ypos);
	bool OnMouseButton(int button, int action, int mods);
	void OBBRaycast(const int button, const int action);
	bool UpdateCamera(double speed, double timeDelta);
	void OnActionPressed(Hotkey::Action action) override;
	void OnActionReleased(Hotkey::Action action) override;

	glm::mat4 GetProjection(UserSettings settings, const VkExtent2D extent);
	glm::mat4 GetProjection();
	void SetProjectionType(ProjectionMode type);

	glm::mat4 GetView();

	void setLocalPosition(float x, float y, float z) override;
	void setLocalPosition(glm::vec3 pos) override;
	

protected:

	virtual void MoveForward(float d);
	virtual void MoveRight(float d);
	virtual void MoveUp(float d);
	virtual void Rotate(float y, float x);
	void UpdateVectors();


	std::string name;
	ProjectionMode projMode;

	// Matrices and vectors.
	glm::mat4 orientation_ = glm::mat4(1);
	glm::mat4 projection_{};
	glm::mat4 view_ = glm::mat4(1.0f);;

	glm::vec4 position_{ 0, 0, 0, 0 };
	glm::vec4 right_{ 1, 0, 0, 0 };
	glm::vec4 up_{ 0, 1, 0, 0 };
	glm::vec4 forward_{ 0, 0, -1, 0 };

	// Control states.
	bool cameraMovingLeft_{};
	bool cameraMovingRight_{};
	bool cameraMovingBackward_{};
	bool cameraMovingForward_{};
	bool cameraMovingDown_{};
	bool cameraMovingUp_{};

	float cameraRotX_{};
	float cameraRotY_{};
	float modelRotX_{};
	float modelRotY_{};

	double mousePosX_{};
	double mousePosY_{};

	CameraMode m_currentMode = NONE;

	float windowWidth_{};
	float windowHeight_{};

	float camSpeed_ = 1.0f;
	bool camSlowed = false;
	bool camSpedUp = false;
	float camNormalSpeed = 1.0f;
	float camSlowSpeed = 0.2f;
	float camFastSpeed = 1.5f;

	float m_defaultPivotDistance = 1000.0f;
};