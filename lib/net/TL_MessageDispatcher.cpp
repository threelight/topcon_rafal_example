#include "TL_MessageDispatcher.h"
#include "TL_Semaphore.h"
#include "TL_Mutex.h"
#include "TL_Exception.h"
#include "TL_Log.h"


#include <map>


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_MessageDispatcher>
{
	ImplOf(uint32_t maxQueueSize_):
		_maxMessageQueueSize(maxQueueSize_),
		_currentMessageQueueSize(0)
	{}

	uint32_t _maxMessageQueueSize;
	uint32_t _currentMessageQueueSize;

	std::multimap<uint64_t, char*> _messageMap;
	TL_Mutex _queueProtect;

	TL_Semaphore _sleepSemaphore;

	static TL_MessageDispatcher* _Dispather;
};


TL_MessageDispatcher* ImplOf<TL_MessageDispatcher>::_Dispather = NULL;


TL_MessageDispatcher*
TL_MessageDispatcher::Instance()
{
	if(ImplOf<TL_MessageDispatcher>::_Dispather == NULL)
	{	
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);

		//should be taken from configuration
		uint32_t maxQueueSize = 1024;
		ImplOf<TL_MessageDispatcher>::_Dispather  =  new TL_MessageDispatcher(maxQueueSize);
	}
	return ImplOf<TL_MessageDispatcher>::_Dispather;	
}


TL_MessageDispatcher::TL_MessageDispatcher(uint32_t maxQueueSize_):
	_p( new ImplOf<TL_MessageDispatcher>(maxQueueSize_) )
{}


TL_MessageDispatcher::~TL_MessageDispatcher()
{
	try
	{
		_p->_queueProtect.destroy();
	}
	catch (std::exception &e)
	{
		TL_ExceptionComply eC(e);
		throw TL_Exception<TL_MessageDispatcher>("TL_MessageDispatcher::~TL_MessageDispatcher",
			_TLS( eC.who() << "->" << eC.what() ) );
	}
}


void
TL_MessageDispatcher::addMessage(uint64_t client_, char* message_)
{
	uint32_t &currentQueueSize = _p->_currentMessageQueueSize;
	uint32_t &maxQueueSize = _p->_maxMessageQueueSize;
	TL_Semaphore &sleepSemaphore = _p->_sleepSemaphore;

	if(currentQueueSize >= maxQueueSize) sleepSemaphore.wait();


	TL_ScopedMutex(_p->_queueProtect);

	_p->_messageMap.insert( pair<uint64_t, char*>(client_, message_) );
	currentQueueSize++;
}


void
TL_MessageDispatcher::addMessages(const TL_MessageListType& messages_)
{
	uint32_t &currentQueueSize = _p->_currentMessageQueueSize;
	uint32_t &maxQueueSize = _p->_maxMessageQueueSize;
	uint32_t size = messages_.size();
	std::multimap<uint64_t, char*> &messageMap = _p->_messageMap;
	TL_Semaphore &sleepSemaphore = _p->_sleepSemaphore;

	if(currentQueueSize + size > maxQueueSize) sleepSemaphore.wait();

	TL_MessageListType::const_iterator iter = messages_.begin();

	TL_ScopedMutex(_p->_queueProtect);


	for(; iter!=messages_.end(); ++iter)
	{
		messageMap.insert(
			pair<uint64_t, char*>( iter->_client, iter->_message )
		);
		currentQueueSize++;
	}
}


void
TL_MessageDispatcher::getMessages(std::list<uint64_t>& clients_, TL_MessageListType& messages_)
{
	multimap<uint64_t, char*>::iterator it;
	pair<multimap<uint64_t, char*>::iterator,multimap<uint64_t, char*>::iterator> ret;
	std::multimap<uint64_t, char*> &messageMap = _p->_messageMap;
	std::list<uint64_t>::const_iterator clientIter = clients_.begin();
	TL_Semaphore &sleepSemaphore = _p->_sleepSemaphore;

	messages_.clear();
	int32_t count=0;


	TL_ScopedMutex(_p->_queueProtect);
	for(; clientIter!=clients_.end(); ++clientIter)
	{
		ret = messageMap.equal_range( (*clientIter) );
		count = 0;
		for(it=ret.first; it!=ret.second; ++it)
		{
			messages_.push_back( TL_MessageType( it->second, it->first ) );
			count++;
		}
		sleepSemaphore.post(count);
		messageMap.erase(ret.first, ret.second);
	}
}


void
TL_MessageDispatcher::run_I() throw()
{
	TL_Log::Instance().setThreadLogFileName("MessageDispatcher");

	// go through all the clients
	//_p->process();
}

} // end of ThreeLight namespace
