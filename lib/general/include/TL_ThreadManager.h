#ifndef __TL_THREADMANAGER__
#define __TL_THREADMANAGER__


#include <time.h>


#include "TL_ThreadData.h"
#include "TL_Thread.h"


namespace ThreeLight
{


class TL_ThreadManager
{
public:
	static TL_ThreadManager& Instance();
	static void Destroy();
	
	/*! \brief threads can get their private data
	*          this is an alternative for privte data
	*          in a class
	*/
	TL_ThreadData* getThreadData();
	
	
	/*! \brief Cancel all the threads 
	*          wait at the most absTime_ time
	*/
	void cancelAllThreads(const struct timespec absTime_);
	

private:
	TL_ThreadManager();
	~TL_ThreadManager();
	
	friend class TL_Thread;
	/*! \brief Each thread adds itselt to the 
	*         list to be commonly handled
	*/
	void addThread(TL_Thread* thread_);
	void removeThread(TL_Thread* thread_);

	TL_Pimpl<TL_ThreadManager>::Type _p;
};


}


#endif

