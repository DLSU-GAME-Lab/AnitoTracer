#pragma once
#include "AUIScreen.h"

#include "Engine/Profiler/Profiler.h"

class UIManager;
class ProfilerScreen :    public AUIScreen
{
public:
	ProfilerScreen();
	~ProfilerScreen();

	void SetProfiler(GpuCpuProfiler* p) { profiler = p; }

private:
	virtual void drawUI() override;
	friend class UIManager;

	GpuCpuProfiler* profiler = nullptr;  // Add this in the ProfilerScreen class
};

