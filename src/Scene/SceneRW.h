#pragma once
#include <string>

class SceneRW
{
public:
	SceneRW();
	~SceneRW();

	void exportScene();
	bool loadScene();
};

