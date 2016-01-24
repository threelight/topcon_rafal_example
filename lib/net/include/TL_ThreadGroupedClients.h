#ifndef __TL_THREADGROUPEDCLIENTS__
#define __TL_THREADGROUPEDCLIENTS__


#include <list>


#include "TL_Thread.h"
#include "TL_Pimpl.h"
#include "TL_EPollServer.h"
#include "TL_LoggedInClients.h"
#include "TL_SocketEventManager.h"
#include "TL_ProtocolLogic.h"


namespace ThreeLight
{


class TL_ThreadGroupedClients:
	public TL_Thread
{
public:
	TL_ThreadGroupedClients(TL_SocketEventManager& eventManager_,
		unsigned long maxClientsQueueSize_ = 1024,
		unsigned long maxEventsQueueSize_ = 1024);

	virtual ~TL_ThreadGroupedClients();

	void destroy();

	void addClients(TL_EPollServer::EventsListType *clients_);

	void sendEvents(TL_EPollServer::EventsListType *events_);

protected:
	virtual void run_I() throw();
	
private:
	TL_Pimpl<TL_ThreadGroupedClients>::Type _p;
};


};


#endif
