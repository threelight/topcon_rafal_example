#ifndef __TL_THREAD__
#define __TL_THREAD__

#include <pthread.h>
#include <time.h>

#include "TL_NoCopy.h"
#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_Thread:
	public TL_NoCopy
{
public:
	/*! \brief defaults are:
	*          threads are Joinable
	*          schedule poilcy SCHED_OTHER
	*          explicit parameters from attr
	*/
	TL_Thread(bool controlled_= true);
	
	virtual ~TL_Thread();
	
	void start();
	
	/*! \brief sets the flag which indicates
	*          the thread should finish
	*/
	void setCancel();
	
	/*! \brief this function should be checked
	*         inside run_I to test against
	*         flag indicating exit request 
	*/	
	const bool& testCancel();
	
	/*! \brief works only for joinable threads
	*          throws exception in other case
	*/
	void join(void **value_ptr_);
	
	bool isRunning();
	
	/*! \brief cannot set anything after the thread has been started
	*          in such case exception will be thrown
	*/
	void setJoinable();
	void setDetached();

	/*! \brief return the type of the thread
	*          Joinable or Detached
	*/
	int getType();
	
	/*! \brief look at man for pthread_attr_setschedpolicy
	*/
	void setPolicy(int policy_);
	int getPolicy();
	
	/*! \brief look at man for pthread_attr_setschedpolicy
	*/
	void setPriority(int priority_);
	int getPriority();
	
	
	static pthread_t GetThreadId();
	
	static unsigned long getThreadsCount();
	
	bool equal(const pthread_t& threadId_);
	bool equal(const TL_Thread& thread_);
	
	/*! \brief function returns thread id
	*          of a main process 
	*/
	static pthread_t MainThreatId();

	/*! \brief waits for all the threads to end
	*          absolute time as a deadline
	*          cancel request has to be sent to the threads 
	*          before using this function 
	*/
	static bool waitForEnd(const struct timespec& absTime_);	
	
protected:	
	/*! \brief this function will be executed in the thread
	*/
	virtual void run_I() throw() = 0;
	
private:
	TL_Pimpl<TL_Thread>::Type _p;
	
	static void* threadFunction(TL_Thread *);
};


} // end of ThreeLight namespace

#endif
