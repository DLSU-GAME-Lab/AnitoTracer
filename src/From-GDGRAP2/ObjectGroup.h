#pragma once
#include <memory>
#include <string>
#include <glm/vec3.hpp>
#include "GameObject.h"

class ObjectGroup final: public GameObject
{
public:
	typedef glm::vec3 vec3;
	typedef std::string String;

	ObjectGroup(String name);

	void addModel(std::shared_ptr<Assets::Model> model);
	void clearModelGroup();

	int getSize() const;
	std::shared_ptr<Assets::Model> getModelAt(int index);

private:
	typedef std::vector<std::shared_ptr<Assets::Model>> ModelList;
	ModelList modelList;

	void performModelTransform() override;
	void performModelRotate() override;
	void performModelScale() override;
};

