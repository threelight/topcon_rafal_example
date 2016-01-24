#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <cassert>
#include <iostream>

#include "TL_Thread.h"
#include "TL_CondVariable.h"
#include "TL_ThreadManager.h"
#include "TL_Exception.h"
#include "TL_Log.h"
#include "TL_SignalManager.h"
#include "TL_GlobalStaticDestroyer.h"


namespace ThreeLight
{


template<>
struct ImplOf<TL_Thread>
{
	ImplOf()
	{
		if( pthread_attr_init(&_attr) != 0 )
			throw TL_Exception<TL_Thread>("TL_Pimpl<TL_Thread>",
				"Insufficient memory to initialize pthread_attr_t");
				
		// By default threads are joinable
		// this is the default state from pthreads
		// Just to be sure we are joinable
		setJoinable();
		
		// schedule policy the default one SCHED_OTHER
		setPolicy(_DefaultSchedulePolicy);
		
		// Always explicit, we want to use our _attr structure
		pthread_attr_setinheritsched(&_attr, PTHREAD_EXPLICIT_SCHED);
		
		_running = false;
		_cancel = false;
		_threadId = 0;
		
		// by default we are part of the threadManager
		_controlled = true;
	}
	
	virtual ~ImplOf()
	{
		pthread_attr_destroy(&_attr);
	}
	
	void setJoinable()
	{
		// function dosn't return errors if second param is ok 
		pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_JOINABLE);
	}

	void setDetached()
	{
		// function dosn't return errors if second param is ok 
		pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_DETACHED);
	}
	
	int getType()
	{
		int type;
		pthread_attr_getdetachstate(&_attr, &type);
		return type;
	}
	
	void setPolicy(int policy_)
	{
		
		// function dosn't return errors if second param is ok
		if( pthread_attr_setschedpolicy(&_attr, policy_) != 0 )
			throw TL_Exception<TL_Thread>("TL_Pimpl<TL_Thread>::setPolicy",
				"Policy value not supperted");
	}
	
	int getPolicy()
	{
		int policy;
		pthread_attr_getschedpolicy(&_attr, &policy);
		return policy;
	}
	
	void setPriority(int priority_)
	{
		_priority.sched_priority = priority_;
		// function dosn't return errors if second param is ok
		pthread_attr_setschedparam(&_attr, &_priority);
	}

	static bool zeroPredicate(void *zero_)
	{
		assert(zero_!=NULL);
		unsigned long *zero = static_cast<unsigned long*>(zero_);
		return (*zero)==0;
	}

	
	// DATA
	pthread_attr_t _attr;
	
	static int _DefaultSchedulePolicy;
	
	static unsigned long _ThreadsCount;
	static TL_CondVariable  _CountProtect;
	
	struct sched_param _priority;
	
	pthread_t _threadId;
	
	bool _running;
	
	// cancel flag, logic in the run_I function
	// should finish if the flag is set to true
	bool _cancel;
	
	// indicates wheater or not
	// the thread belogs to threadManager
	bool _controlled;

	static pthread_t _MainThreadId;
};


int ImplOf<TL_Thread>::_DefaultSchedulePolicy = SCHED_OTHER;
unsigned long ImplOf<TL_Thread>::_ThreadsCount = 0;

TL_CondVariable ImplOf<TL_Thread>::_CountProtect;

int __nothing = TL_GlobalStaticDestroyer::Instance().addObjToDesroy(
	new TL_StaticPtr<TL_CondVariable>(
		ImplOf<TL_Thread>::_CountProtect
	)
);

// initialization of a main thread id
pthread_t ImplOf<TL_Thread>::_MainThreadId = 0;


/****************************************************************
*****************************************************************
****************************************************************/


void*
TL_Thread::threadFunction(TL_Thread *thread_)
{
	try
	{
		// block SIGPIPE for every thread
		TL_SignalManager::Instance().sigProcMask(SIG_BLOCK, SIGPIPE);

		// run the real function
		thread_->run_I();
		if(thread_->_p->_controlled)
		{
			ImplOf<TL_Thread>::_CountProtect.lock();
			ImplOf<TL_Thread>::_ThreadsCount --;
			if(ImplOf<TL_Thread>::_ThreadsCount==0)
			{
				ImplOf<TL_Thread>::_CountProtect.signal();
			}
			ImplOf<TL_Thread>::_CountProtect.unlock();
		
			TL_ThreadManager::Instance().removeThread(thread_);
		}
	}
	catch (std::exception &e)
	{
		TL_ExceptionComply eC(e);
		LOG(NULL, "TL_Thread::threadFunction" , TL_Log::NORMAL,
		"Big Problem: " << eC.who() << "\n" << eC.what() );
	}

	thread_->_p->_running = false;
	
	pthread_exit(NULL);
	return NULL;
}


TL_Thread::TL_Thread(bool controlled_)
{
	_p->_controlled = controlled_;
}


TL_Thread::~TL_Thread()
{
	assert(_p->_running == false);
}


void
TL_Thread::start()
{
	int result=0;
	
	result = pthread_create(&_p->_threadId,
				&_p->_attr,
				(void*(*)(void*)) TL_Thread::threadFunction,
				(void*)this );
	if(result !=0)
	{
		switch(result)
		{
		case EAGAIN:
			throw TL_Exception<TL_Thread>("TL_Thread::start",
				"Cannot create any more threads\n Lack of resources"
				"or PTHREAD_THREADS_MAX exceeded");
		case EINVAL:	
			throw TL_Exception<TL_Thread>("TL_Thread::start",
				"Cannot create the thread\n"
				"attr structute is not valid");
		case EPERM:
			throw TL_Exception<TL_Thread>("TL_Thread::start",
				"Cannot create the thread\n"
				"The caller does not have appropriate permission to set "
				"the required scheduling parameters or scheduling policy");
		default:
			throw TL_Exception<TL_Thread>("TL_Thread::start",
				"Cannot create the thread\n"
				"Shouldn't happen - unknown error");
		}
	}
	_p->_running = true;
	
	if(_p->_controlled)
	{	
		ImplOf<TL_Thread>::_CountProtect.lock();
		ImplOf<TL_Thread>::_ThreadsCount ++;
		ImplOf<TL_Thread>::_CountProtect.unlock();
		
		TL_ThreadManager::Instance().addThread(this);
	}
}


void
TL_Thread::setCancel()
{
	_p->_cancel = true;
}


const bool&
TL_Thread::testCancel()
{
	return _p->_cancel;
}


void
TL_Thread::join(void **value_ptr_)
{
	int result=0;
	result = pthread_join(_p->_threadId, value_ptr_);

	if(result != 0)
	{
		switch(result)
		{
		case EINVAL:
			throw TL_Exception<TL_Thread>("TL_Thread::join",
				"The  implementation  has detected that "
				"the value specified by threadId does "
				"not refer to a joinable thread.");
		case ESRCH:
			throw TL_Exception<TL_Thread>("TL_Thread::join",
				"No thread could be found corresponding to "
				"that specified by the given threadId.");
		case EDEADLK:
			throw TL_Exception<TL_Thread>("TL_Thread::join",
				"A deadlock was detected or the value of "
				"thread specifies the calling thread.");
			
		default:
			throw TL_Exception<TL_Thread>("TL_Thread::join",
				"Cannot join the thread\n"
				"Shouldn't happen - unknown error");
		}
	}
}


bool TL_Thread::isRunning()
{
	return _p->_running;	
}


void
TL_Thread::setJoinable()
{
	if(_p->_running)
		throw TL_Exception<TL_Thread>("TL_Thread::setJoinable",
			"Cannot change the type of thread to Joinable"
			" for runnig thread");
			
	_p->setJoinable();
}


void
TL_Thread::setDetached()
{
	if(_p->_running)
		throw TL_Exception<TL_Thread>("TL_Thread::setDetached",
			"Cannot change the type of thread to Detached"
			" for runnig thread");
	_p->setDetached();
}


int
TL_Thread::getType()
{
	return _p->getType();
}	


void
TL_Thread::setPolicy(int policy_)
{
	if(_p->_running)
		throw TL_Exception<TL_Thread>("TL_Thread::setPolicy",
			"Cannot change policy"
			" for running thread");
	_p->setPolicy(policy_);
}


int
TL_Thread::getPolicy()
{
	return _p->getPolicy();
}
	

void
TL_Thread::setPriority(int priority_)
{
	if(_p->_running)
		throw TL_Exception<TL_Thread>("TL_Thread::setPriority",
			"Cannot change priority"
			" for runnig thread");
			
	if(getPolicy() == SCHED_OTHER)
		throw TL_Exception<TL_Thread>("TL_Thread::setPriority",
			"Priority cannot be set for SCHED_OTHER policy");
			
	_p->setPriority(priority_);
}


int
TL_Thread::getPriority()
{
	return _p->_priority.sched_priority;
}


pthread_t
TL_Thread::GetThreadId()
{
	return pthread_self();
}


bool
TL_Thread::equal(const pthread_t& threadId_)
{
	return pthread_equal(_p->_threadId, threadId_) == 0;
}


bool
TL_Thread::equal(const TL_Thread& thread_)
{
	return pthread_equal(_p->_threadId, thread_._p->_threadId) == 0;
}


pthread_t
TL_Thread::MainThreatId()
{
	if(ImplOf<TL_Thread>::_MainThreadId == 0)
	{
		ImplOf<TL_Thread>::_MainThreadId = pthread_self();
	}
	return ImplOf<TL_Thread>::_MainThreadId;
}


unsigned long
TL_Thread::getThreadsCount()
{
	return ImplOf<TL_Thread>::_ThreadsCount;
}


bool
TL_Thread::waitForEnd(const struct timespec& absTime_)
{
	bool result = ImplOf<TL_Thread>::_CountProtect.timedLock(absTime_);
	if(!result) return false;

	// release the mutex _CountProtect and waits for 
	// condition variable
	result = ImplOf<TL_Thread>::_CountProtect.condTimedWait(absTime_,
		ImplOf<TL_Thread>::zeroPredicate,
		(void*)&ImplOf<TL_Thread>::_ThreadsCount);
	
	ImplOf<TL_Thread>::_CountProtect.unlock();

	return result==true;
}


} // end of ThreeLight namespace
