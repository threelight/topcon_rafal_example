#ifndef __TL_TIME__
#define __TL_TIME__


#include <sys/time.h>
#include <time.h>


#include "TL_Pimpl.h"


namespace ThreeLight
{

	
class TL_Timespec
{
public:
	enum TimeType { NOW=1 };
	enum { MaxNanoseconds=1000000000 };
	enum
	{
		NANOSECOND	=1,
		MICROSECOND	=1000,
		MILISECOND	=1000000,
		SECOND		=1000000000 
	};
	
	TL_Timespec();
	TL_Timespec(TimeType type_);
	TL_Timespec(time_t sec_, long int nanoSec_);
	TL_Timespec(const struct timespec& timespec_);
	TL_Timespec(const TL_Timespec& TLTimespec_);
	~TL_Timespec();
	
	TL_Timespec& add(time_t sec_, long int nanoSec_);
	TL_Timespec& add(const struct timespec& timespec_);
	TL_Timespec& add(const TL_Timespec& TLTimespec_);
	
	operator const struct timespec&();
	operator const struct timespec*();

	time_t sec(); //returns
	time_t sec(time_t sec_); // sets and returns

	long int nsec(); // returns
	long int nsec(long int nsec_); // sets and returns

	/*! \brief function adds nanoseconds
	*
	* maximum naumber is limited by long int max
	* and for 32 bits it is 2 147 483 648
	* little above 2.1 seconds
	*/
	TL_Timespec& operator<<(long int reltime_);
		
private:
	struct timespec _timespec;
};


/*! \brief class is not thread safe
*          thread has to use it on a stack or protect
*          the content
*/

class TL_StrfTime
{
public:
	enum {BUFMAXSIZE=64};
	
	TL_StrfTime();
	TL_StrfTime(const char *format_);
	TL_StrfTime(const time_t& time_);
	TL_StrfTime(const time_t& time_, const char *format_);
	
	
	operator const char*();
	
private:
	TL_Pimpl<TL_StrfTime>::Type _p;
};


} // end of ThreeLight Namespace


#endif
