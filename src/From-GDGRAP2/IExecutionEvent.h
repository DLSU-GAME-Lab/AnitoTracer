#pragma once
// This is an interface for the execution event callback, which will be called when the execution of a thread is finished.
class IExecutionEvent
{
public:
	virtual void onFinishedExecution() = 0;
};