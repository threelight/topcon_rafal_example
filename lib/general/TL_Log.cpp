#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<iostream>


#include "TL_Log.h"
#include "TL_Thread.h"
#include "TL_Time.h"
#include "TL_OSUtil.h"
#include "TL_ThreadManager.h"
#include "TL_Exception.h"


namespace ThreeLight
{


template<>
struct ImplOf<TL_Log>
{
	ImplOf():
	_sdtErrFile(-1),
	_sdtOutFile(-1),
	_manager( TL_ThreadManager::Instance() )
	{}
	
	~ImplOf()
	{
		if(_sdtErrFile > 0)
		{
			::fsync(_sdtErrFile);
			::close(_sdtErrFile);
		}
		if(_sdtOutFile > 0)
		{
			::fsync(_sdtOutFile);
			::close(_sdtOutFile);
		}
	}

	ostream&
	getLog()
	{
		static const char ext[] = "log";
		TL_ThreadData *ptr =  _manager.getThreadData();
		if(ptr->_file == NULL)
		{
			stringstream logFile;
			logFile << _logDir << "/";
			if(ptr->_logFileName.empty())
			{
				logFile << TL_Thread::GetThreadId() << "." << ext;
			}
			else
			{
				logFile << ptr->_logFileName << "-" <<
				TL_Thread::GetThreadId() << "." << ext;
			}
			
			ptr->_file = new ofstream( logFile.str().c_str() );
			if( ! ptr->_file->is_open())
			{
				throw TL_Exception<TL_Log>("TL_Log::setLog",
					_TLS("Cannot create log file: " << logFile.str() ) );
			}
		}
		return *ptr->_file;
	}
	

	static TL_Log* _TLLog;
	int _sdtErrFile;	
	int _sdtOutFile;
	string _logDir;
	TL_ThreadManager &_manager;
};

TL_Log* ImplOf<TL_Log>::_TLLog = NULL;


TL_Log&
TL_Log::Instance()
{
	if(ImplOf<TL_Log>::_TLLog==NULL)
	{
		// only main thread can instantiate the log
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_Log>::_TLLog =  new TL_Log();
	}
	return *ImplOf<TL_Log>::_TLLog;
		
}


void
TL_Log::Destroy()
{
	if(ImplOf<TL_Log>::_TLLog!=NULL)
	{
		// only main thread can destroy the log
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		delete ImplOf<TL_Log>::_TLLog;
		ImplOf<TL_Log>::_TLLog = NULL;
	}
}


TL_Log::LogLevel TL_Log::_LogLevel = TL_Log::NORMAL;


TL_Log::TL_Log()
{
	char* var = getenv("TL_DEBUG");

	if(var != NULL)
	{
		int test = atoi(var);
		if(test<-1)
			test = -1;
		if(test>DEBUG3)
			test = DEBUG3;

		_LogLevel = (LogLevel)test;
	}	
}


TL_Log::~TL_Log()
{
	TL_ThreadData *ptr =  _p->_manager.getThreadData();
	delete ptr;
}


int
TL_Log::setLogDir(const char* path_)
{
	_p->_logDir = path_;
	try
	{
		TL_StrfTime timeDir;

		_p->_logDir.append("/");
		_p->_logDir.append(timeDir);
		TL_OSUtil::CreateDir(_p->_logDir.c_str());
	}
	catch (TL_Exception<TL_OSUtil>& e)
	{
		throw TL_Exception<TL_Log>("TL_Log::setLogDir",
			_TLS( e.who() << "->" << e.what()) );
	}
	return 0;
}


int
TL_Log::setStdErrFile(const char* file_)
{
	assert(_p->_sdtErrFile == -1);
	assert(!_p->_logDir.empty());

	string path(_p->_logDir);
	path.append("/");
	path.append(file_);
	int result = _p->_sdtErrFile = ::open(path.c_str(),
			O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR);
	if(result == -1)
	{
		throw TL_Exception<TL_Log>("TL_Log::setStdErrFile",
			TL_OSUtil::GetErrorString(errno) + " " + path);
	}
	::close(stderr->_fileno); // close std error

	result = ::fcntl(_p->_sdtErrFile ,F_DUPFD, 0);
	if(result == -1)
	{
		throw TL_Exception<TL_Log>("TL_Log::setStdErrFile",
			TL_OSUtil::GetErrorString(errno) );
	}
	return 0;
}


int
TL_Log::setStdOutFile(const char* file_)
{
	assert(_p->_sdtOutFile == -1);
	assert(!_p->_logDir.empty());

	string path(_p->_logDir);
	path.append("/");
	path.append(file_);
	int result = _p->_sdtOutFile = ::open(path.c_str(),
			O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR);
	if(result == -1)
	{
		throw TL_Exception<TL_Log>("TL_Log::setStdOutFile",
			TL_OSUtil::GetErrorString(errno) );
	}
	::close(stdout->_fileno); // close std output

	result = ::fcntl(_p->_sdtOutFile ,F_DUPFD, 0);
	if(result == -1)
	{
		throw TL_Exception<TL_Log>("TL_Log::setStdOutFile",
			TL_OSUtil::GetErrorString(errno) );
	}
	return 0;
}


int
TL_Log::setThreadLogFileName(const char* name_)
{
	if(name_==NULL)
	{
		throw TL_Exception<TL_Log>("TL_Log::setThreadLogFileName",
			"NULL pointer passed.");
	}

	TL_ThreadData *ptr =  _p->_manager.getThreadData();
	if(ptr->_file != NULL)
	{
		throw TL_Exception<TL_Log>("TL_Log::setThreadLogFileName",
			"Log file already opened.");
	}
	ptr->_logFileName = name_;

	return 0;
}


void
TL_Log::sync()
{
	if(_p->_sdtOutFile != -1)
		::fsync(_p->_sdtOutFile);
	if(_p->_sdtErrFile != -1)
		::fsync(_p->_sdtErrFile);

	// TODO sync rest of the log files
}


ostream&
TL_Log::Log(void* addr_, const char* who_, bool continue_)
{
	assert(!_p->_logDir.empty());

	ostream &out = _p->getLog();

	if(!continue_)
	{
		out << "Address: " << addr_ << " Who: " << who_ << endl;
	}

	return out;
}


} // end of ThreeLight namespace
