#include <pthread.h>
#include <errno.h>

#include <cassert>
#include <iostream>
#include <set>

#include "TL_ThreadManager.h"
#include "TL_Thread.h"
#include "TL_Mutex.h"
#include "TL_Time.h"
#include "TL_Exception.h"

using namespace std;

namespace ThreeLight
{

template<>
struct ImplOf<TL_ThreadManager>
{
	ImplOf()
	{
		int result=0;
		result = pthread_key_create(&_threadDataPointer, ThreadDataDestructor);
		if(result!=0)
		{
			switch(result)
			{
			case EAGAIN:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"The  system  lacked  the necessary resources "
					"to create another thread-specific data key, "
					"or the system-imposed limit on the total number "
					"of keys per process {PTHREAD_KEYS_MAX} has been exceeded.");
			case ENOMEM:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"Insufficient memory exists to create the key.");
			default:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"Cannot create key - unknown error");
			}
		}
	}
	
	~ImplOf()
	{
		pthread_key_delete(_threadDataPointer);
		_threadListProtect.destroy();
	}
	
	static void ThreadDataDestructor(void *dataPtr_)
	{
		TL_Thread* ptr = static_cast<TL_Thread*>(dataPtr_);
		delete ptr;
	}
	
	
	pthread_key_t _threadDataPointer;
	
	TL_Mutex _threadListProtect;
	
	set<TL_Thread*> _threadList;
		
	static TL_ThreadManager *_Manager;
	
	
};


// making sure that manager is instatiated by main thread only
// in this case sequancial operator is perfect
TL_ThreadManager *ImplOf<TL_ThreadManager>::_Manager = NULL;


TL_ThreadManager::TL_ThreadManager()
{}
	

TL_ThreadManager::~TL_ThreadManager()
{}


TL_ThreadManager&
TL_ThreadManager::Instance()
{
	if(ImplOf<TL_ThreadManager>::_Manager == NULL)
	{	
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_ThreadManager>::_Manager =  new TL_ThreadManager();
	}
	return *ImplOf<TL_ThreadManager>::_Manager;
}


void
TL_ThreadManager::Destroy()
{
	assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
	// neet to be sure all threads are gone
	TL_Timespec absTime(TL_Timespec::NOW);
	absTime << TL_Timespec::SECOND;
	
	ImplOf<TL_ThreadManager>::_Manager->cancelAllThreads( absTime );
	
	delete ImplOf<TL_ThreadManager>::_Manager;
	ImplOf<TL_ThreadManager>::_Manager = NULL;
}


TL_ThreadData*
TL_ThreadManager::getThreadData()
{
	void *ptrToData;
	
	if ((ptrToData = pthread_getspecific(_p->_threadDataPointer)) == NULL) {
		ptrToData = new TL_ThreadData();
		int result=0;
		result = pthread_setspecific(_p->_threadDataPointer, ptrToData);
		
		if(result != 0)
		{
			switch(result)
			{
			case ENOMEM:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"Insufficient memory exists to associate "
					"the value with the key.");
			case EINVAL:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"The key value is invalid.");
			default:
				throw TL_Exception<TL_ThreadManager>("TL_ThreadManager",
					"Cannot set key - unknown error.");
			}
		}
	}
	return static_cast<TL_ThreadData*>(ptrToData);
}


void
TL_ThreadManager::addThread(TL_Thread* thread_)
{
	assert(thread_!=NULL);
		
	TL_ScopedMutex(_p->_threadListProtect);
	pair<set<TL_Thread*>::iterator, bool> ret;
	ret = _p->_threadList.insert(thread_);
	
	if(ret.second==false)
	{
		throw TL_Exception<TL_ThreadManager>("TL_ThreadManager::addThread",
			"Shouldn't happen at all - serious problem.");
	}	
}


void
TL_ThreadManager::removeThread(TL_Thread* thread_)
{
	assert(thread_!=NULL);
	// releases the mutex when function ends
	// stack variable
	TL_ScopedMutex(_p->_threadListProtect);
	set<TL_Thread*>::iterator iter;
	iter = _p->_threadList.find(thread_);
	
	if(iter == _p->_threadList.end())
	{
		throw TL_Exception<TL_ThreadManager>("TL_ThreadManager::removeThread",
			"Shouldn't happen at all - serious problem.");
	}
	
	if( (*iter)->getType() == PTHREAD_CREATE_DETACHED)
	{
		_p->_threadList.erase(iter);
	}
}


void
TL_ThreadManager::cancelAllThreads(const struct timespec absTime_)
{
	set<TL_Thread*>::iterator iter;

	_p->_threadListProtect.lock();
	iter = _p->_threadList.begin();
	for(; iter!=_p->_threadList.end(); ++iter)
	{
		(*iter)->setCancel();
	}
	_p->_threadListProtect.unlock();

	bool result = TL_Thread::waitForEnd(absTime_);
	
	// all the threads finished
	// and all the rest threads are Joinable
	if(result)
	{
		iter = _p->_threadList.begin();
		for(; iter!=_p->_threadList.end(); ++iter)
		{
			(*iter)->join(NULL);
		}
		_p->_threadListProtect.unlock();
		_p->_threadList.clear();
	}	
}
	
		
} // end of ThreeLight namespace
