#include "IETSemaphore.h"

IETSemaphore::IETSemaphore(int available)
{
	this->semaphore = new Semaphore(available);
}

IETSemaphore::~IETSemaphore()
{
	delete this->semaphore;
}

void IETSemaphore::acquire() const
{
	this->semaphore->acquire();
}

void IETSemaphore::acquire(int permits) const
{
	for (int i = 0; i < permits; i++)
	{
		this->semaphore->acquire();
	}
}

void IETSemaphore::release(int permits) const
{
	this->semaphore->release(permits);
}

void IETSemaphore::release() const
{
	this->semaphore->release();
}