#include "ShaderBase.hpp"

ShaderBase::ShaderBase()
{
}

ShaderBase::~ShaderBase()
{
	Unload();
}

bool ShaderBase::Init(const std::string& folder, const std::string& vs, const std::string& fs)
{
	BuildVertexLayout(layout);
	program = ShaderLoader::LoadProgram(folder, vs, fs);

	if (!bgfx::isValid(program)) return false;

	OnInit();

	return true;
}

void ShaderBase::Submit(bgfx::ViewId viewID, uint32_t depth, bool preserveState)
{
	if (bgfx::isValid(program)) {
		bgfx::submit(viewID,
			program,
			depth,
			preserveState ? BGFX_DISCARD_ALL : BGFX_DISCARD_NONE
		);
	}
}

void ShaderBase::Unload()
{
	OnUnload();

	if (bgfx::isValid(program)) {
		bgfx::destroy(program);
		program = BGFX_INVALID_HANDLE;
	}
}
