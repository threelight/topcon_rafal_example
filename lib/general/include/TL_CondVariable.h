#ifndef __TL_CONDVARIABLE__
#define __TL_CONDVARIABLE__


#include "TL_Mutex.h"


namespace ThreeLight
{

class TL_CondVariable:
	public TL_Mutex
{
public:
	typedef bool (*Predicate)(void*);
	
	
	/*! \brief creates standard condition variable
	*          default settings
	*/
	TL_CondVariable();
	
	~TL_CondVariable();
	
	/*! \brief waits indefinetly for a signal on a variable
	*          mutex has to be locked first
	*/
	void condWait(Predicate predicate_, void* data_);
	
	/*! \brief waits only till absolute time for a signal
	*          the variable
	*/
	bool condTimedWait(const struct timespec& absTime_, Predicate predicate, void* data_);
	
	/*! \brief send a signal to the variable
	*          mutex has to be locked first 
	*/	
	void signal();
	
	/*! \brief send signal on the variable
	*          all the waiting threads will be weaken up
	*/	
	void broadcast();

	
	void destroy();
		
protected:
	pthread_cond_t _condition;
	bool _initialized;
};


}

#endif
