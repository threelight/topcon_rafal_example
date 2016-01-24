#ifndef __TL_SEMAPHORE__
#define __TL_SEMAPHORE__

#include <time.h>


#include "TL_NoCopy.h"
#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_Semaphore:
	public TL_NoCopy
{
public:
	/*! \brief initializes the non shared semaphore
	*/
	TL_Semaphore(int initValue_=0);
	virtual ~TL_Semaphore(void);

	/*! \brief posts to the semaphore
	*
	* It can be safely called from within a signal handler
	*/
	void post();
	void post(unsigned int count_);

	/*! \brief waits indefinetly for resource to be available
	*/
	void wait();
	void wait(unsigned int count_);

	/*! \brief returns instantly
	*/
	bool tryWait();
	bool tryWait(unsigned int count_);

	/*! \brief waits only till absTime_
	*/
	bool timedWait(const struct timespec* absTime_);
	bool timedWait(const struct timespec* absTime_, unsigned int count_);

	/*! \brief gets a current value of the semaphore
	*/
	int getValue();


private:
	TL_Pimpl<TL_Semaphore>::Type _p;
};


} // end of ThreeLight namespace


#endif
