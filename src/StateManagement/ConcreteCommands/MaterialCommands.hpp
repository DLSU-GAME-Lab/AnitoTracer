#pragma once
#include "StateManagement/ICommand.hpp"
#include "Assets/Material.hpp"

#include <string>
#include <glm/glm.hpp>

class GameObject;

class ModifyColorCommand : public ICommand
{
public:
	ModifyColorCommand(Assets::Material* material, glm::vec4 color);
	~ModifyColorCommand() = default;

	void execute() override;
	void undo() override;

private:
	Assets::Material* material;
	glm::vec4 newColor;
	glm::vec4 oldColor;
};

class ChangeMapCommand : public ICommand
{
public:
	ChangeMapCommand(Assets::Material* material, int textureId);
	~ChangeMapCommand() = default;

	void execute() override;
	void undo() override;

private:
	Assets::Material* material;
	int newTextureId;
	int oldTextureId;
};

class ModifyFuzzinessCommand : public ICommand
{
public:
	ModifyFuzzinessCommand(Assets::Material* material, float value);
	~ModifyFuzzinessCommand() = default;

	void execute() override;
	void undo() override;

private:
	Assets::Material* material;
	float newValue;
	float oldValue;
};

class ModifyRefractionIndexCommand : public ICommand
{
public:
	ModifyRefractionIndexCommand(Assets::Material* material, float value);
	~ModifyRefractionIndexCommand() = default;

	void execute() override;
	void undo() override;

private:
	Assets::Material* material;
	float newValue;
	float oldValue;
};
