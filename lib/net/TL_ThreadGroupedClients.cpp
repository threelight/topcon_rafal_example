#include <map>
#include <iostream>


#include "TL_Mutex.h"
#include "TL_EventBuffer.h"
#include "TL_ThreadGroupedClients.h"
#include "TL_Connection.h"
#include "TL_Log.h"
#include "TL_Semaphore.h"
#include "TL_EPollServer.h"
#include "TL_Exception.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_ThreadGroupedClients>
{
	ImplOf(TL_SocketEventManager& eventManager_,
		unsigned long& maxClientsQueueSize_,
		unsigned long& maxEventsQueueSize_):
	_localClients( TL_LocalClients::Instance() ),
	_eventManager(eventManager_),
	_protocolLogic( TL_ProtocolLogic::Instance() ),
	_currentClientsSize(0),
	_maxClientsSize(maxClientsQueueSize_),
	_currentEventsSize(0),
	_maxEventsSize(maxEventsQueueSize_),
	_initialized(true)
	{}

	TL_EPollServer::EventsListType*
	getClients()
	{
		TL_ScopedMutex sm(_clientsListProtect);
		TL_EPollServer::EventsListType* newClients=NULL;
		if(_newClientsList.size())
		{	
			newClients = _newClientsList.front();
			if(newClients)
			{
				_currentClientsSize -= newClients->size();
				_newClientsList.pop_front();
			}
		}
		return newClients;
	}

	TL_EPollServer::EventsListType*
	getEvents()
	{
		TL_ScopedMutex sm(_eventsListProtect);
		TL_EPollServer::EventsListType* events=NULL;
		if(_eventsList.size())
		{
			events = _eventsList.front();
			if(events)
			{
				_currentEventsSize -= events->size();
				_eventsList.pop_front();
			}
		}
		return events;
	}

	void
	process(const bool& cancel_)
	{
		TL_EPollServer::EventsListType *newClients=NULL, *events=NULL;
		TL_EPollServer::EventsListType::iterator iter;
		std::map<int, TL_Connection*>::iterator connIter;
		TL_Connection *conn=NULL;

		TL_ProtocolLogic::TL_ListLogicStruct *listLogicStruct=NULL;
		TL_ProtocolLogic::TL_ListLogicStructIter listIter;

		uint32_t loops=0;
		uint64_t clientId;
		TL_SocketEventManager::ClientListType clientsToRemove;
		char* command;

		for(;;)
		{
			clientsToRemove.clear();
			if( _mainLoop.tryWait() )
			{
				loops=0;
				newClients = getClients();
				if(newClients)
				{
					for(iter = newClients->begin();
						iter != newClients->end();
						++iter)
					{
						_connections.insert(
							pair<int, TL_Connection*>
							(
								(*iter)->_socket,
								new TL_Connection( (*iter)->_socket)
							)
						);
						// I should use it before deleting it
						// to get usefull info about the client
						delete (*iter);
					}
	
					delete newClients;
				}
	
				TL_EPollServer::TL_EventType *event=NULL;
				events = getEvents();
				bool initState=false;
				if(events)
				{
					for(iter = events->begin();
						iter != events->end();
						++iter)
					{
						initState=false;
						event = (*iter);
						switch(event->_events)
						{
						case TL_EPollServer::IN:
						case TL_EPollServer::MSG:
						case TL_EPollServer::PRI:
						case TL_EPollServer::ERR:
						case TL_EPollServer::HUP:
							connIter = _connections.find( event->_socket );
							conn = connIter->second;
							if(conn)
							{
								if(conn->getState() == TL_Connection::INIT)
									initState = true;

								conn->processRead();

								if(initState && conn->clientId())
									_localClients->addClient(clientId);
								
								if(conn->finish())
								{
									_connections.erase(connIter);
									clientsToRemove.push_back(conn->getSocket());

									clientId = conn->clientId();
									if(clientId) {
										_localClients->removeClient(clientId);
										_clients.erase(clientId);
									}

									conn->destroy();
								}
								else
								{
									while(NULL != (command=conn->getReadMessage()) )
										_protocolLogic->addCommand(command, conn, (uint64_t)this );
								}
							}
							break;
						default:
							break;
						}
						
						delete event;
					}
					delete events;
				} // events
			} // try wait
			
			// if more then 10 loops, sleep for 1 milisecond
			if(loops++ > 10)
			{
				usleep(10000); // 1 milisecond
			}
			
			// get responce from the logic
			// they are responces to the same client
			listLogicStruct = _protocolLogic->getReply((uint64_t)this);
			if(listLogicStruct)
			{
				listIter = listLogicStruct->begin();
				for(; listIter!=listLogicStruct->end(); ++listIter)
				{
					connIter = _connections.find((*listIter)->_socket);
					if(connIter != _connections.end())
					{
						if(connIter->second == (*listIter)->_conn)
						{
							(*listIter)->_conn->addMessageToWrite(
								(*listIter)->_command
							);
						}
					}
					delete (*listIter);
				}
				delete listLogicStruct;
			}

			//TODO take the messages from message dispatcher
			// and pass them to our clients
			// TL_MessageDispatcher::getMessages(std::list<uint64_t>& clients_, TL_MessageListType& messages_)

			for(connIter = _connections.begin();
				connIter != _connections.end(); )
			{
				conn = connIter->second;
				if(conn->processWrite())
				{
					loops=0;
				}

				if(conn->finish())
				{
					_connections.erase(connIter++);
					clientsToRemove.push_back(conn->getSocket());
					clientId = conn->clientId();
					if(clientId)
					{
						_localClients->removeClient(clientId);
						_clients.erase(clientId);
					}

					conn->destroy();
				}
				else
				{
					++connIter;
				}
			}

			if(clientsToRemove.size())
			{
				_eventManager.addClientsToRemove(clientsToRemove);
			}


			// cancel thread in progress
			// free all the alocated memory
			if(cancel_)
			{
				// remove whats left in the queues
				while( NULL != (newClients = getClients()) )
				{
					for(iter = newClients->begin();
						iter != newClients->end();
						++iter)
					{
						delete (*iter);
					}
					delete events;
				}

				while( NULL != (events = getEvents()) )
				{
					for(iter = events->begin();
						iter != events->end();
						++iter)
					{
						delete (*iter);
					}
					delete events;
				}
				//leave the thread 
				break;
			}
		}// for(;;)
	}


	TL_LocalClients *_localClients;
	TL_SocketEventManager& _eventManager;
	TL_ProtocolLogic* _protocolLogic;


	TL_Mutex _clientsListProtect;
	unsigned long _currentClientsSize;
	unsigned long _maxClientsSize;
	TL_EventBuffer::EventsListOfListType _newClientsList;

	TL_Mutex _eventsListProtect;
	unsigned long _currentEventsSize;
	unsigned long _maxEventsSize;
	TL_EventBuffer::EventsListOfListType _eventsList;

	std::map<int, TL_Connection*> _connections;
	std::map<uint64_t, TL_Connection*> _clients;
	TL_Semaphore _mainLoop;


	bool _initialized;
};


TL_ThreadGroupedClients::TL_ThreadGroupedClients(TL_SocketEventManager& eventManager_,
	unsigned long maxClientsQueueSize_,
	unsigned long maxEventsQueueSize_
	)
:_p( new ImplOf<TL_ThreadGroupedClients>(eventManager_,
					maxClientsQueueSize_,
					maxEventsQueueSize_) )
{}


TL_ThreadGroupedClients::~TL_ThreadGroupedClients()
{
	if(_p->_initialized)
	{
		throw TL_Exception<TL_ThreadGroupedClients>(
			"TL_ThreadGroupedClients::~TL_ThreadGroupedClients",
			"You forgot to call the destroy function");
	}
}



void
TL_ThreadGroupedClients::destroy()
{
	if(_p->_initialized)
	{
		try
		{
			_p->_clientsListProtect.destroy();
			_p->_eventsListProtect.destroy();
		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_ThreadGroupedClients>("TL_ThreadGroupedClients::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}
	}
	_p->_initialized = false;
}


void
TL_ThreadGroupedClients::addClients(TL_EPollServer::EventsListType *clients_)
{
	unsigned long &currClientsSizeRef = _p->_currentClientsSize;
	unsigned long &maxClientsSizeRef = _p->_maxClientsSize;
	unsigned long size = clients_->size();

	if(currClientsSizeRef + size > maxClientsSizeRef)
		usleep(100); // sleep 0.1 milisecond

	TL_ScopedMutex(_p->_clientsListProtect);
	currClientsSizeRef += size;
	_p->_newClientsList.push_back(clients_);
	_p->_mainLoop.post();
}


void
TL_ThreadGroupedClients::sendEvents(TL_EPollServer::EventsListType *events_)
{
	unsigned long &currEventsSizeRef = _p->_currentEventsSize;
	unsigned long &maxEventsSizeRef = _p->_maxEventsSize;
	unsigned long size = events_->size();

	if(currEventsSizeRef + size > maxEventsSizeRef)
		usleep(100); // sleep 0.1 milisecond

	TL_ScopedMutex(_p->_eventsListProtect);
	currEventsSizeRef += size;
	_p->_newClientsList.push_back(events_);
	_p->_mainLoop.post();
}


void
TL_ThreadGroupedClients::run_I() throw()
{
	TL_Log::Instance().setThreadLogFileName("ThreadGroupedClients");

	// go through all the clients
	_p->process( testCancel() );
}


};
