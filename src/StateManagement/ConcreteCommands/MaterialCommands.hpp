#pragma once
#include "StateManagement/ICommand.hpp"
#include "Assets/Material.hpp"

#include <string>
#include <glm/glm.hpp>
#include <variant>

class GameObject;


class ModifyMaterialPropertyCommand : public ICommand
{
public:
	using Variant = std::variant<glm::vec4, int, float>;
	using Setter = std::function<void(Assets::Material*, const Variant&)>;

	ModifyMaterialPropertyCommand(Assets::Material* material, Setter setter, Variant oldValue, Variant newValue);
	~ModifyMaterialPropertyCommand() = default;

	void execute() override;
	void undo() override;

private:
	Assets::Material* material;
	Setter apply;
	Variant oldValue;
	Variant newValue;

};
