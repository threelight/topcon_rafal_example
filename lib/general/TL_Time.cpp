#include <iostream>

#include "TL_Time.h"
#include "TL_Log.h"
#include "TL_Exception.h"


namespace ThreeLight
{


TL_Timespec::TL_Timespec()
{}


TL_Timespec::TL_Timespec(TimeType type_)
{
	if(type_ != NOW)
		throw TL_Exception<TL_Timespec>("TL_Timespec::TL_Timespec",
			"Type not supported");
	
	struct timeval tv;
	gettimeofday(&tv,NULL);

	_timespec.tv_sec = tv.tv_sec;
	_timespec.tv_nsec = tv.tv_usec*1000;
}


TL_Timespec::TL_Timespec(time_t sec_, long int nanoSec_)
{
	if(nanoSec_< 0 )
		throw TL_Exception<TL_Timespec>("TL_Timespec::TL_Timespec",
			"Nanosecond parameter less then 0 ");

	_timespec.tv_sec  = sec_;
	_timespec.tv_sec += nanoSec_ / MaxNanoseconds;
	_timespec.tv_nsec = nanoSec_ % MaxNanoseconds;
}


TL_Timespec::TL_Timespec(const struct timespec& timespec_)
{
	if(timespec_.tv_nsec < 0)
		throw TL_Exception<TL_Timespec>("TL_Timespec::TL_Timespec",
			"Nanosecond parameter less then 0 ");
			
	_timespec.tv_sec  = timespec_.tv_sec;
	_timespec.tv_sec += timespec_.tv_nsec / MaxNanoseconds;
	_timespec.tv_nsec = timespec_.tv_nsec % MaxNanoseconds;
}


TL_Timespec::TL_Timespec(const TL_Timespec& TLTimespec_)
{
	_timespec.tv_sec  = TLTimespec_._timespec.tv_sec;
	_timespec.tv_sec += TLTimespec_._timespec.tv_nsec / MaxNanoseconds;
	_timespec.tv_nsec = TLTimespec_._timespec.tv_nsec % MaxNanoseconds;
}


TL_Timespec::~TL_Timespec()
{}


TL_Timespec&	
TL_Timespec::add(time_t sec_, long int nanoSec_)
{
	
	_timespec.tv_sec += sec_;
	_timespec.tv_sec += nanoSec_ / MaxNanoseconds;
	_timespec.tv_nsec+= nanoSec_ % MaxNanoseconds;
	return *this;
}


TL_Timespec&
TL_Timespec::add(const struct timespec& timespec_)
{
	return add(timespec_.tv_sec, timespec_.tv_nsec);
}


TL_Timespec&
TL_Timespec::add(const TL_Timespec& TLTimespec_)
{
	return add(TLTimespec_._timespec.tv_sec, TLTimespec_._timespec.tv_nsec);
}

	
TL_Timespec::operator const struct timespec&()
{
	return _timespec;
}


TL_Timespec::operator const struct timespec*()
{
	return &_timespec;
}


time_t
TL_Timespec::sec()
{
	return _timespec.tv_sec;
}


time_t
TL_Timespec::sec(time_t sec_)
{
	return _timespec.tv_sec = sec_;
}


long int
TL_Timespec::nsec()
{
	return _timespec.tv_nsec;
}


long int
TL_Timespec::nsec(long int nsec_)
{
	return _timespec.tv_nsec = nsec_;
}


TL_Timespec&
TL_Timespec::operator<<(long int reltime_)
{
	return add(0, reltime_);
}


/********************************************************************
 ********************************************************************
 *******************************************************************/

template<>
struct ImplOf<TL_StrfTime>
{
	void
	initTime(const time_t& time_, const char* format_)
	{
		struct tm resultTime;
		
		char* format = const_cast<char*>(format_);
		
		if(format==NULL)
		{
			format = const_cast<char*>(_defaultFormat);
		}
			
		localtime_r(&time_, &resultTime);
		if(strftime(_buffor, TL_StrfTime::BUFMAXSIZE, format, &resultTime) == 0)
		{
			throw TL_Exception<TL_StrfTime>("ImplOf<TL_StrfTime>::initTime",
				_TLS("Size of the result string "
				"longer then " << TL_StrfTime::BUFMAXSIZE) );
		}
	}
	
	// DATA	
	char _buffor[TL_StrfTime::BUFMAXSIZE];
	static const char* _defaultFormat;
};

const char* ImplOf<TL_StrfTime>::_defaultFormat="%F.%T";

	
TL_StrfTime::TL_StrfTime()
{
	time_t theTime = time(NULL);
	_p->initTime(theTime, NULL);
}


TL_StrfTime::TL_StrfTime(const char *format_)
{
	time_t theTime = time(NULL);
	_p->initTime(theTime, format_);
}


TL_StrfTime::TL_StrfTime(const time_t& time_)
{
	_p->initTime(time_, NULL);
}
	

TL_StrfTime::TL_StrfTime(const time_t& time_, const char *format_)
{
	_p->initTime(time_, format_);
}


TL_StrfTime::operator const char*()
{
	return _p->_buffor;
}


} // end of ThreeLight namestace 
