#include <pthread.h>
#include <time.h>
#include <errno.h>


#include "TL_CondVariable.h"
#include "TL_Time.h"
#include "TL_Exception.h"
#include "TL_OSUtil.h"
#include "TL_Log.h"


namespace ThreeLight
{


TL_CondVariable::TL_CondVariable()
{
	int result=0;
	_initialized = false;
	// default settings
	result = pthread_cond_init(&_condition, NULL);
	
	if(result!=0)
	{
		switch(result)
		{
		case EAGAIN:
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::TL_CondVariable",
				"The system lacked the necessary "
				"resources (other than memory) to "
				"initialize another condition variable.");
		case ENOMEM:
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::TL_CondVariable",
				"Insufficient memory exists to "
				"initialize the condition variable.");
		case EBUSY:
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::TL_CondVariable",
				"The  implementation  has detected an attempt "
				"to reinitialize the object referenced "
				"by cond, a previously initialized, but "
				"not yet destroyed, condition variable.");
		case EINVAL:
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::TL_CondVariable",
				"The value specified by attr is invalid.");
		default:
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::TL_CondVariable",
				"Cannot initialaze condition variable "
				"Unknown error");
		}
	}
	_initialized = true;
}

TL_CondVariable::~TL_CondVariable()
{
	if(_initialized)
	{
		throw TL_Exception<TL_CondVariable>("TL_CondVariable::~TL_CondVariable",
			"You forgot to call the destroy function");
	}
}
	
void
TL_CondVariable::condWait(Predicate predicate_, void* data_)
{
	if(_locked==false)
		throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
			"Cannot wait on the condition if the mutex is not locked");
	
	int result=0;
	
	do
	{
		result = pthread_cond_wait(&_condition, &_mutex); 
		if(result!=0)
		{
			switch(result)
			{
			case EINVAL:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"The value specified by cond, mutex, or abstime is invalid."
					"or Different  mutexes  were  supplied  for concurrent "
					"pthread_cond_timedwait() or pthread_cond_wait() "
					"operations on the same condition variable.");
			case EPERM:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"The mutex was not owned by the current "
					"thread at the time of the call.");
			default:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"Cannot wait on the condition variable "
					"Unknown error.");
			}
		}
	// basause it is possible to be weaken up falsly 
	// one needs to check the condition again
	}
	while( (predicate_!=NULL) && (!predicate_(data_)) );
}


bool
TL_CondVariable::condTimedWait(const struct timespec& absTime_, Predicate predicate_, void* data_)
{
	if(_locked==false)
		throw TL_Exception<TL_CondVariable>("TL_CondVariable::condTimedWait",
			"Cannot wait on the condition if the mutex is not locked");
	
	int result=0;

	do
	{	
		result = pthread_cond_timedwait(&_condition, &_mutex, &absTime_); 
		if(result!=0)
		{
			switch(result)
			{
			case EINVAL:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"The value specified by cond, mutex, or abstime is invalid."
					"or Different  mutexes  were  supplied  for concurrent "
					"pthread_cond_timedwait() or pthread_cond_wait() "
					"operations on the same condition variable.");
			case EPERM:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"The mutex was not owned by the current "
					"thread at the time of the call.");
			case ETIMEDOUT:
				return false;
			default:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::condWait",
					"Cannot wait on the condition variable "
					"Unknown error.");
			}
		}
	// basause it is possible to be weaken up falsly 
	// one needs to check the condition again
	}
	while( (predicate_!=NULL) && (!predicate_(data_)) );
	return true;
}


void
TL_CondVariable::signal()
{
	pthread_cond_signal(&_condition);
}


void
TL_CondVariable::broadcast()
{
	pthread_cond_broadcast(&_condition);
}


void
TL_CondVariable::destroy()
{
	int result=0;

	if(_initialized)
	{
		// default settings
		result = pthread_cond_destroy(&_condition);
		
		if(result!=0)
		{
			switch(result)
			{
			case EBUSY:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::destroy",
					"The implementation has detected an attempt "
					"to destroy the object referenced  by  cond "
					"while it is referenced (for example, while "
					"being used in a pthread_cond_wait() or "
					"pthread_cond_timedwait() by another thread.");
			case EINVAL:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::destroy",
					"The value specified by cond is invalid.");
			default:
				throw TL_Exception<TL_CondVariable>("TL_CondVariable::destroy",
					"Cannot destroy condition variable "
					"Unknown error");
			}
		}
	
		try
		{
			TL_Mutex::destroy();
		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_CondVariable>("TL_CondVariable::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}
	
		_initialized = false;
	}
}


}
