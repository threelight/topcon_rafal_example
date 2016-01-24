// to have timedwait function available
#define _XOPEN_SOURCE 600
#include<semaphore.h>
#include<bits/local_lim.h>
#include<errno.h>
#include<iostream>


#include "TL_Semaphore.h"
#include "TL_Exception.h"


namespace ThreeLight
{


template<>
struct ImplOf<TL_Semaphore>
{
	void init(int initValue_)
	{
		if(initValue_>SEM_VALUE_MAX)
			initValue_ = SEM_VALUE_MAX;
		if(initValue_<0)
			initValue_ = 0;

		// this is a semaphore which cannot be
		// shared beetween processes
		sem_init(&_semaphore, 0, initValue_);
	}

	~ImplOf()
	{
		sem_destroy(&_semaphore);
	}

	sem_t _semaphore;
};


TL_Semaphore::TL_Semaphore(int initValue_)
{
	_p->init(initValue_);
}


TL_Semaphore::~TL_Semaphore(void)
{}


void
TL_Semaphore::post()
{
	sem_post(&_p->_semaphore);
}

void
TL_Semaphore::post(unsigned int count_=1)
{
	for(unsigned int i=0; i<count_; ++i)
	{
		sem_post(&_p->_semaphore);
	}
}


void
TL_Semaphore::wait()
{
	int result=0;
	do {
		result = sem_wait(&_p->_semaphore);
	}
	while(result==-1 && errno==EINTR);
}


void
TL_Semaphore::wait(unsigned int count_)
{
	int result=0;
	for(unsigned int i=0; i<count_; ++i)
	{
		do {
			result = sem_wait(&_p->_semaphore);
		}
		while(result==-1 && errno==EINTR);
	}
}


bool
TL_Semaphore::tryWait(void)
{
	int result=0;
	
	do {
		result = sem_trywait(&_p->_semaphore);
		if(result==-1)
		{
			if(errno==EAGAIN)
				return false;
		}

	}
	while(result==-1 && errno==EINTR);

	return true;
}


bool
TL_Semaphore::tryWait(unsigned int count_)
{
	int result=0;

	for(unsigned int i=0; i<count_; ++i)
	{
		do {
			result = sem_trywait(&_p->_semaphore);
			if(result==-1)
			{
				if(errno==EAGAIN)
					return false;
			}
		}
		while(result==-1 && errno==EINTR);
	}

	return true;
}


bool
TL_Semaphore::timedWait(const struct timespec* absTime_)
{
	int result=0;

	do {
		result = sem_timedwait(&_p->_semaphore, absTime_);

		if(result==-1)
		{
			if(errno==EAGAIN || errno==ETIMEDOUT)
				return false;
		}
	}
	while(result==-1 && errno==EINTR);

	return true;
}


bool
TL_Semaphore::timedWait(const struct timespec* absTime_, unsigned int count_)
{
	int result=0;

	for(unsigned int i=0; i<count_; ++i)
	{
		do {
			result = sem_timedwait(&_p->_semaphore, absTime_);
			if(result==-1)
			{
				if(errno==EAGAIN || errno==ETIMEDOUT)
					return false;
			}
		}
		while(result==-1 && errno==EINTR);
	}

	return true;
}


int
TL_Semaphore::getValue()
{
	int result=0;
	sem_getvalue(&_p->_semaphore, &result);

	if(result<0) result = 0;

	return result;
}


} // end of ThreeLight namespace
