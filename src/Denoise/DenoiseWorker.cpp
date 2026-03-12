#include "DenoiseWorker.hpp"
#include "Denoiser.hpp"

DenoiseWorker::DenoiseWorker(Denoiser* owner) : m_owner(owner)
{
}

void DenoiseWorker::run()
{
	m_owner->ExecuteDenoiseJob();
	m_owner->OnFinishedExecution();
}
