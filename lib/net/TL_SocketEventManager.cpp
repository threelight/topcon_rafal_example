#include <map>
#include <vector>
#include <algorithm>
#include <iostream>


#include "TL_SocketEventManager.h"
#include "TL_EventBuffer.h"
#include "TL_Log.h"
#include "TL_Mutex.h"
#include "TL_Semaphore.h"
#include "TL_ThreadGroupedClients.h"
#include "TL_LoggedInClients.h"
#include "TL_EPollServer.h"
#include "TL_Log.h"
#include "TL_Exception.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_SocketEventManager>
{
	void
	passNewClientsToGroups(TL_EPollServer::EventsListType* conns_)
	{
		static uint32_t vIndex=0;

		TL_EPollServer::EventsListType::const_iterator iter = conns_->begin();
					
		for(;iter!=conns_->end(); ++iter)
		{
			if(vIndex==_maxThreads) vIndex=0;
			if(_threads[vIndex]._list == NULL)	
			{
				// client thread is responsible for freeing the memory
				_threads[vIndex]._list = new TL_EPollServer::EventsListType;
			}
			_threads[vIndex]._list->push_back( (*iter) );
			vIndex++;
		}
		// I am done with the conns so...
		delete conns_;

		for(uint32_t i=0; i<_maxThreads; ++i)
		{
			if(_threads[i]._list != NULL)
				_threads[i]._group->addClients(_threads[i]._list);
		}

		// fill the local map
		TL_ScopedMutex sm(_mapProtect);
		TL_ThreadGroupedClients* tgcPtr;
		for(uint32_t i=0; i<_maxThreads; ++i)
		{
			if(_threads[i]._list != NULL)
			{
				tgcPtr = _threads[i]._group;
				iter = _threads[i]._list->begin();;

				for(;iter!=_threads[i]._list->end(); ++iter)
				{
					_clientMap.insert
					( 	pair<int, TL_ThreadGroupedClients*>
						(
							(*iter)->_socket, tgcPtr
						)
					);
				}
				_threads[i]._list = NULL;
			}
		}
	}

	void
	passEventsToGroups(TL_EPollServer::EventsListType* events_)
	{
		std::map<TL_ThreadGroupedClients*, TL_EPollServer::EventsListType*> groupedEvents;
		for(uint32_t i=0; i<_maxThreads; ++i)
		{
			groupedEvents.insert(
				pair<TL_ThreadGroupedClients*, TL_EPollServer::EventsListType*>
					(_threads[i]._group, NULL)
			);
		}
	
		TL_EPollServer::EventsListType::const_iterator iter = events_->begin();
		TL_ThreadGroupedClients* tgcPtr;
		_mapProtect.lock();
		for(; iter!=events_->end(); ++iter)
		{
			tgcPtr = _clientMap.find( (*iter)->_socket )->second;
			if(tgcPtr)
			{
				if(groupedEvents[tgcPtr] == NULL)
					groupedEvents[tgcPtr] = new TL_EPollServer::EventsListType;

				groupedEvents[tgcPtr]->push_back( (*iter) );
			}
		}
		_mapProtect.unlock();
		//done with events_
		delete events_;
		
		// pass the events to Grouped Clients
		std::map<TL_ThreadGroupedClients*, TL_EPollServer::EventsListType*>::iterator
			gIter = groupedEvents.begin();
		for(; gIter!=groupedEvents.end(); ++gIter)
		{
			if((*gIter).second)
				(*gIter).first->sendEvents( (*gIter).second );
		}
	}

	void
	initThreads(TL_SocketEventManager& eventManager_)
	{

		// initalize all the client threads
		TL_ThreadGroupedClients *threadPtr;
		for(uint32_t i=0; i<_maxThreads; ++i)
		{
			threadPtr = new TL_ThreadGroupedClients(eventManager_);
			threadPtr->start();
			_threads.push_back(
				newClientListType( threadPtr, NULL )
				);
		}
	}

	void
	destroyThreads()
	{
		// destroy all the threads
		std::vector<newClientListType>::iterator iter =
			_threads.begin();			
		for(; iter!=_threads.end(); ++iter)
		{
			iter->_group->destroy();
		}		
	}

	uint32_t _maxThreads;
	// socket - TL_ThreadGroupedClients;
	std::map<int, TL_ThreadGroupedClients*> _clientMap;

	typedef struct newClientList
	{
		newClientList(TL_ThreadGroupedClients* g_, TL_EPollServer::EventsListType* l_):
			_group(g_), _list(l_) {}

		TL_ThreadGroupedClients* _group;
		TL_EPollServer::EventsListType* _list;
	} newClientListType;

	std::vector<newClientListType> _threads;

	TL_Mutex _mapProtect;
	bool _initialized;
};


TL_SocketEventManager::TL_SocketEventManager(unsigned int maxThreads)
{
	_p->_maxThreads = maxThreads;
	_p->_initialized = true;
}


TL_SocketEventManager::~TL_SocketEventManager()
{
	if(_p->_initialized)
	{
		throw TL_Exception<TL_EventBuffer>("TL_EventBuffer::~TL_EventBuffer",
			"You forgot to call the destroy function");
	}
}

void
TL_SocketEventManager::destroy()
{
	if(_p->_initialized)
	{
		try
		{
			_p->_mapProtect.destroy();
			_p->destroyThreads();

		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_SocketEventManager>("TL_SocketEventManager::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}
	}
	_p->_initialized = false;
}


void
TL_SocketEventManager::addClientsToRemove(ClientListType& clientList_)
{
	std::map<int, TL_ThreadGroupedClients*> &clientMap = _p->_clientMap;

	ClientListType::const_iterator iter = clientList_.begin();
	TL_ScopedMutex(_p->_mapProtect);
	for(; iter!=clientList_.end(); ++iter)
	{
		clientMap.erase( (*iter) );
	}
}


void
TL_SocketEventManager::run_I() throw()
{
	TL_Log::Instance().setThreadLogFileName("EventManager");

	_p->initThreads(*this);


	TL_EPollServer::EventsListType *events=NULL;
	TL_EPollServer::EventsListType *conns=NULL;

	int loops = 0;
	for(;;)
	{
		if(testCancel())
		{
			break;
		}

		conns = TL_EventBuffer::getNewConnections();
		if(conns)
		{
			loops = 0;
			_p->passNewClientsToGroups(conns);
		}
		
		events = TL_EventBuffer::getEvents();
		if(events)
		{
			loops = 0;
			// pass events to threads
			_p->passEventsToGroups(events);
		}
		else
		{
			if(loops > 10)
			{
				usleep(1000); // 1 milisecond
			}
			else if(loops > 2)
			{
				usleep(10); // 10 microsecond
				loops++;
			} else {
				loops++;
			}
		}
	}
}


};
