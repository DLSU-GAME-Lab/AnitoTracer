#pragma once

#include <bgfx/bgfx.h>
#include <string>
#include "ShaderLoader.hpp"

class ShaderBase {

public:
	ShaderBase();
	virtual ~ShaderBase();

	ShaderBase(const ShaderBase&) = delete;
	ShaderBase& operator=(const ShaderBase&) = delete;

	bool Init(const std::string& folder, const std::string& vs, const std::string& fs);
	void Submit(bgfx::ViewId viewID, uint32_t dept = 0, bool preserveState = false);
	void Unload();

	bgfx::ProgramHandle GetProgram() const { return program; }
	const bgfx::VertexLayout& GetLayout() const { return layout; }

protected:
	virtual void OnCreate() {};
	virtual void BuildVertexLayout(bgfx::VertexLayout& _layout) = 0;
	virtual void OnInit() {}
	virtual void OnUnload() {}

	bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
	bgfx::VertexLayout layout;
};