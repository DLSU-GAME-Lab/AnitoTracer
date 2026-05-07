#include "SceneCamera.h"

SceneCamera::SceneCamera(std::string name) : Camera(name)
{
}

SceneCamera::~SceneCamera()
{
}

GameObject::GameObjectPtr SceneCamera::Clone() const
{
	return std::make_unique<SceneCamera>(*this);
}

void SceneCamera::MoveForward(const float d)
{
	Camera::MoveForward(d);
}

void SceneCamera::MoveRight(const float d)
{
	Camera::MoveRight(d);
}

void SceneCamera::MoveUp(const float d)
{
	Camera::MoveUp(d);
}

void SceneCamera::Rotate(const float y, const float x)
{
	Camera::Rotate(y, x);
}
