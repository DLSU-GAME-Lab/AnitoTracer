#pragma once
#include <semaphore>
#include <mutex>

/// <summary>
/// Representation of a semaphore class using C++ 20 semaphores
/// </summary>
class IETSemaphore
{
public:
	IETSemaphore(int available);
	~IETSemaphore();

	void acquire() const;
	void acquire(int permits) const;
	void release() const;
	void release(int permits) const;

private:
	const static int GLOBAL_MAX_PERMIT = 50;
	typedef std::counting_semaphore<GLOBAL_MAX_PERMIT> Semaphore; //C++ 20 requires permits to be known during compile-time. Global max here is 50.
	Semaphore* semaphore;
};