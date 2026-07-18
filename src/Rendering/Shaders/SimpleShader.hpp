#pragma once

#include "ShaderBase.hpp"

class SimpleShader : public ShaderBase {
public:
	SimpleShader() {
		Init("simple", "v_simple", "f_simple");
	}

protected:
	void BuildVertexLayout(bgfx::VertexLayout& _layout) override {
		_layout.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
			.end();
	}
};