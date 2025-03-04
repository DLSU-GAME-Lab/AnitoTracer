#pragma once
#include "Camera.h"

class SceneCamera : public Camera
{
public:
	SceneCamera(std::string name);
	~SceneCamera();

protected:

	virtual void MoveForward(float d);
	virtual void MoveRight(float d);
	virtual void MoveUp(float d);
	virtual void Rotate(float y, float x);
};

