#pragma once
#include "From-GDGRAP2/IETThread.h"

class Denoiser;

class DenoiseWorker : public IETThread
{
public:
	DenoiseWorker(Denoiser* owner);
	~DenoiseWorker() = default;

	// Inherited via IETThread
	void run() override;

private:
	Denoiser* m_owner = nullptr;
};