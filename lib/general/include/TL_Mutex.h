#ifndef __TL_MUTEX__
#define __TL_MUTEX__


#include <pthread.h>
#include <time.h>


#include "TL_NoCopy.h"


namespace ThreeLight {

class TL_Mutex:
	public TL_NoCopy
{
public:
	TL_Mutex(void);
	virtual ~TL_Mutex(void);

	void destroy();

	void lock(void);
	
	bool tryLock(void);

	bool timedLock(const struct timespec& time_);
	
	void unlock(void);
	
	bool isLocked();
		
protected:
	pthread_mutex_t	_mutex;
	bool _locked;
	bool _initialized;
};


class TL_ScopedMutex:
	public TL_NoCopy
{
public:
	TL_ScopedMutex(TL_Mutex& mutex_);
	~TL_ScopedMutex();
	
protected:
	TL_Mutex& _mutex;
};


} // end of ThreeLight namespace


#endif // __TL_MUTEX__
