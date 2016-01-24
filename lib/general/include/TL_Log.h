#ifndef __TL_LOG__
#define __TL_LOG__


#include <ostream>


#include "TL_ThreadManager.h"
#include "TL_Pimpl.h"


namespace ThreeLight
{

// this formated data is meant to be used in exceptions.
#define _TLS(a) (\
	TL_ThreadManager::Instance().getThreadData()->_ss.str(""),\
	TL_ThreadManager::Instance().getThreadData()->_ss << a,   \
	TL_ThreadManager::Instance().getThreadData()->_ss.str() )


class TL_Log
{
public:
	enum LogLevel
	{
		NORMAL=0,
		DEBUG1=1,
		DEBUG2=2,
		DEBUG3=3
	};
	
	static TL_Log& Instance();

	static void Destroy();
	
	/*! \brief Path to a dir where logs are to be written
	*/
	int setLogDir(const char* path_);
	
	/*! \brief if set error output will go there
	*/
	int setStdErrFile(const char* file_);
	
	/*! \brief if set standard out will go there
	*/
	int setStdOutFile(const char* file_);
	
	/*! \brief each thread including main
	*          should set the log file name
	*          dafault is empty 
	*/
	int setThreadLogFileName(const char* name_);
	
	
	/*! \brief Sync all the log files
	*/
	void sync();

	ostream& Log(void* addr_, const char* who_, bool continue_=false);

	static LogLevel _LogLevel;
	
private:
	TL_Log();
	~TL_Log();
	
	TL_Pimpl<TL_Log>::Type _p;
};


#define LOG(addr, who, level, text) \
	if(TL_Log::_LogLevel>=level)\
	{\
		TL_Log::Instance().Log(addr, who, false) << text << endl;\
	}

#define LOG_C(level, text) \
	if(TL_Log::_LogLevel>=level)\
	{\
		TL_Log::Instance().Log(NULL, "NULL", true) << text << endl;\
	}


} // end of ThreeLight namespace


#endif
