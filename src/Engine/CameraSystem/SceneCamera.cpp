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

}

void SceneCamera::MoveRight(const float d)
{

}

void SceneCamera::MoveUp(const float d)
{

}

void SceneCamera::Rotate(const float y, const float x)
{

}
