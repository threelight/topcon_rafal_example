#include <errno.h>


#include "TL_Mutex.h"
#include "TL_Exception.h"
#include "TL_BackTrace.h"


namespace ThreeLight
{


TL_Mutex::TL_Mutex(void)
{
	int result=0;
	result = pthread_mutex_init(&_mutex, NULL);
	if(result != 0)
	{
		switch(result)
		{
		case EAGAIN:
			throw TL_Exception<TL_Mutex>("TL_Mutex::TL_Mutex",
				"The system lacked the necessary resources "
				"(other than memory) to initialize another mutex.");
		case ENOMEM:
			throw TL_Exception<TL_Mutex>("TL_Mutex::TL_Mutex",
				"Insufficient memory exists to initialize the mutex.");
		case EPERM:	
			throw TL_Exception<TL_Mutex>("TL_Mutex::TL_Mutex",
				"The caller does not have the "
				"privilege to perform the operation.");
		default:
			throw TL_Exception<TL_Mutex>("TL_Mutex::TL_Mutex",
				"Cannot initialaze the mutex\n"
				"Shouldn't happen - unknown error");		
		}
	}
	_locked = false;
	_initialized = true;
}


TL_Mutex::~TL_Mutex(void)
{
	if(_initialized)
	{
		throw TL_Exception<TL_Mutex>("TL_Mutex::~TL_Mutex",
			"You forgot to call the destroy function");
	}
}


void
TL_Mutex::destroy(void)
{
	int result=0;

	if(_initialized)
	{
		result = pthread_mutex_destroy(&_mutex);
		if( result != 0)
		{
			switch(result)
			{
			case EBUSY:
				throw TL_Exception<TL_Mutex>("TL_Mutex::destroy",
					"The  implementation  has  detected  an  attempt "
					" to destroy the object referenced by mutex while "
					"it  is  locked  or  referenced by another thread");
			case EINVAL:
				throw TL_Exception<TL_Mutex>("TL_Mutex::destroy",
					"The value specified by mutex is invalid.");
			default:
				throw TL_Exception<TL_Mutex>("TL_Mutex::destroy",
					"Cannot destroy the mutex "
					"Shouldn't happen - unknown error"); 
			}
		}
		_initialized = false;
	}
}


void
TL_Mutex::lock(void)
{
	int result=0;
	result = pthread_mutex_lock(&_mutex);

	if(result != 0)
	{
		switch(result)
		{
		case EINVAL:
			TL_BackTrace::Trace();
			throw TL_Exception<TL_Mutex>("TL_Mutex::lock",
				"The value specified by mutex does not "
				"refer to an initialized mutex object.");
		case EAGAIN:
			throw TL_Exception<TL_Mutex>("TL_Mutex::lock",
				"The  mutex  could not be acquired because "
				"the maximum number of recursive locks for mutex has "
				"been exceeded.");
		case EDEADLK:
			throw TL_Exception<TL_Mutex>("TL_Mutex::lock",
				"The current thread already owns the mutex.");
		default:
			throw TL_Exception<TL_Mutex>("TL_Mutex::lock",
				"Cannot lock the mutex "
				"Shouldn't happen - unknown error");
		}
	}
	_locked = true;
}


bool
TL_Mutex::tryLock(void)
{
	int result=0;
	result = pthread_mutex_trylock(&_mutex);

	if(result != 0)
	{
		switch(result)
		{
		case EINVAL:
			throw TL_Exception<TL_Mutex>("TL_Mutex::tryLock",
				"The value specified by mutex does not "
				"refer to an initialized mutex object.");
		case EAGAIN:
			throw TL_Exception<TL_Mutex>("TL_Mutex::tryLock",
				"The  mutex  could not be acquired because "
				"the maximum number of recursive locks for mutex has "
				"been exceeded.");
		case EBUSY:
			// mutex is busy 
			return false;
		default:
			throw TL_Exception<TL_Mutex>("TL_Mutex::tryLock",
				"Cannot lock the mutex "
				"Shouldn't happen - unknown error");
		}
	}
	_locked = true;
	// managed to lock the mutex
	return true;
}


bool
TL_Mutex::timedLock(const struct timespec& time_)
{
	int result=0;
	result = pthread_mutex_timedlock(&_mutex, &time_);
	
	if(result != 0)
	{
		switch(result)
		{
		case EAGAIN:
			throw TL_Exception<TL_Mutex>("TL_Mutex::timedLock",
				"The  mutex  could not be acquired because "
				"the maximum number of recursive locks for mutex has "
				"been exceeded.");
		case EINVAL:
			throw TL_Exception<TL_Mutex>("TL_Mutex::timedLock",
				"The  process or thread would have blocked, "
				"and the abs_timeout parameter specified "
				"a nanosec-onds field value less than zero "
				"or greater than or equal to 1000 million.");
		case EDEADLK:
			throw TL_Exception<TL_Mutex>("TL_Mutex::timedLock",
				"The current thread already owns the mutex.");
		case ETIMEDOUT:
			// timeout expired 
			return false;
		default:
			throw TL_Exception<TL_Mutex>("TL_Mutex::timedLock",
				"Cannot timedlock the mutex "
				"Shouldn't happen - unknown error");
		}
	}
	_locked = true;
	return true;
}

	
void
TL_Mutex::unlock(void)
{
	
	int result=0;
	result = pthread_mutex_unlock(&_mutex);
	
	if(result != 0)
	{
		switch(result)
		{
		case EINVAL:
			throw TL_Exception<TL_Mutex>("TL_Mutex::unlock",
				"The value specified by mutex does not "
				"refer to an initialized mutex object.");
		case EPERM:
			throw TL_Exception<TL_Mutex>("TL_Mutex::unlock",
				"The value specified by mutex does not "
				"refer to an initialized mutex object.");
		default:
			throw TL_Exception<TL_Mutex>("TL_Mutex::unlock",
				"Cannot unlock the mutex "
				"Shouldn't happen - unknown error");
		}
	}
	_locked = false;
	
}


bool
TL_Mutex::isLocked()
{
	return _locked;
}

/*********************************************************************
 *********************************************************************
 ********************************************************************/

TL_ScopedMutex::TL_ScopedMutex(TL_Mutex& mutex_):_mutex(mutex_)
{
	_mutex.lock(); 
}
	

TL_ScopedMutex::~TL_ScopedMutex()
{
	if(_mutex.isLocked())
		_mutex.unlock();
}


} // end of ThreeLight namespace
