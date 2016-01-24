#include "TL_EventBuffer.h"
#include "TL_Mutex.h"
#include "TL_EPollServer.h"
#include "TL_Thread.h"
#include "TL_Exception.h"
#include "TL_Semaphore.h"
#include "TL_Log.h"


using namespace std;


namespace ThreeLight
{

	
template<>
struct ImplOf<TL_EventBuffer>
{
	
	ImplOf()
	{
		_initialized = true;
		_actEventQueueSize = 0;
	}

	~ImplOf()
	{}

	void
	init(unsigned long maxEventsSize_)
	{
		_maxEventsSize = maxEventsSize_;
	}
	
	static TL_EventBuffer* _Buffer;

	TL_Mutex _eventListProtect;
	TL_EventBuffer::EventsListOfListType _eventList;
	TL_Semaphore _eventSleepSemaphore;


	TL_EventBuffer::EventsListOfListType _newConnectionList;

	bool _initialized;
	
	unsigned long _maxEventsSize;
	unsigned long _actEventQueueSize;
};


TL_EventBuffer* ImplOf<TL_EventBuffer>::_Buffer = 0;


TL_EventBuffer&
TL_EventBuffer::Instance()
{
	if(ImplOf<TL_EventBuffer>::_Buffer == NULL)
	{
		// only main thread can instantiate the buffer
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_EventBuffer>::_Buffer = new TL_EventBuffer();
	}
	return *ImplOf<TL_EventBuffer>::_Buffer;
}


void
TL_EventBuffer::setBufferSize(unsigned long maxSize_)
{
	_p->_maxEventsSize = maxSize_;
}


TL_EventBuffer::TL_EventBuffer(unsigned long maxEventsSize_)
{
	_p->init(maxEventsSize_);
}


TL_EventBuffer::~TL_EventBuffer()
{
	if(_p->_initialized)
	{
		throw TL_Exception<TL_EventBuffer>("TL_EventBuffer::~TL_EventBuffer",
			"You forgot to call the destroy function");
	}
}


void
TL_EventBuffer::destroy()
{
	if(_p->_initialized)
	{
		try
		{
 			_p->_eventListProtect.destroy();
		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_EventBuffer>("TL_EventBuffer::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}	
		_p->_initialized = false;
	}
}


void
TL_EventBuffer::addEvents(TL_EPollServer::EventsListType *events_)
{
	static TL_EventBuffer* Buffer = &Instance();

	unsigned long &queueSize = Buffer->_p->_actEventQueueSize;
	unsigned long &maxSize = Buffer->_p->_maxEventsSize;
	unsigned long count = events_->size();
	
	TL_Semaphore &sleepSem = Buffer->_p->_eventSleepSemaphore;

	while(queueSize + count > maxSize)
	{
		sleepSem.wait();
	}
	
	TL_ScopedMutex(Buffer->_p->_eventListProtect);
	queueSize += count;
	Buffer->_p->_eventList.push_back(events_);
}


void
TL_EventBuffer::addNewConnections(TL_EPollServer::EventsListType *events_)
{
	static TL_EventBuffer* Buffer = &Instance();

	unsigned long &queueSize = Buffer->_p->_actEventQueueSize;
	unsigned long &maxSize = Buffer->_p->_maxEventsSize;
	unsigned long count = events_->size();
	TL_Semaphore &sleepSem = Buffer->_p->_eventSleepSemaphore;

	while(queueSize + count > maxSize) sleepSem.wait();
	
	TL_ScopedMutex(Buffer->_p->_eventListProtect);
	queueSize += count;
	Buffer->_p->_newConnectionList.push_back(events_);
}


TL_EPollServer::EventsListType*
TL_EventBuffer::getEvents()
{
	static TL_EventBuffer* Buffer = &Instance();

	TL_ScopedMutex(Buffer->_p->_eventListProtect);
	TL_EPollServer::EventsListType *eList=NULL;
	if(Buffer->_p->_eventList.size())
	{
 		eList = Buffer->_p->_eventList.front();
		if(eList)
		{
			Buffer->_p->_actEventQueueSize -= eList->size(); 
			Buffer->_p->_eventList.pop_front();
			Buffer->_p->_eventSleepSemaphore.post();
		}
	}
	return eList;
}


TL_EPollServer::EventsListType*
TL_EventBuffer::getNewConnections()
{
	static TL_EventBuffer* Buffer = &Instance();

	TL_ScopedMutex(Buffer->_p->_eventListProtect);

	TL_EPollServer::EventsListType *eList=NULL;

	if(Buffer->_p->_newConnectionList.size())
	{
		eList = Buffer->_p->_newConnectionList.front();
		if(eList)
		{
			Buffer->_p->_actEventQueueSize -= eList->size(); 
			Buffer->_p->_eventSleepSemaphore.post();
		}
		Buffer->_p->_newConnectionList.pop_front();
	}

	return eList;
}


} // end of threelight namespace
