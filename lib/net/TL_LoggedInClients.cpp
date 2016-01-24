#include "TL_LoggedInClients.h"
#include "TL_Mutex.h"
#include "TL_Exception.h"
#include "TL_Log.h"


#include<set>


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_LoggedInClients>
{
	std::set<uint64_t> _clients;
	TL_Mutex _setProtect;
};


TL_LoggedInClients::TL_LoggedInClients()
{
}


TL_LoggedInClients::~TL_LoggedInClients()
{
	try
	{
		_p->_setProtect.destroy();
	}
	catch (std::exception &e)
	{
		TL_ExceptionComply eC(e);
		throw TL_Exception<TL_LoggedInClients>("TL_LoggedInClients::~TL_LoggedInClients",
			_TLS( eC.who() << "->" << eC.what() ) );
	}
}
	

void
TL_LoggedInClients::addClient(const uint64_t& client_)
{
	TL_ScopedMutex(_p->_setProtect);
	_p->_clients.insert(client_);
}


void
TL_LoggedInClients::addClients(const TL_ClientListType& clients_)
{
	std::set<uint64_t>& clients = _p->_clients;
	TL_ClientListType::const_iterator iter = clients_.begin();

	TL_ScopedMutex(_p->_setProtect);
	for(; iter!=clients_.end(); ++iter)
	{
		clients.insert( (*iter) );
	}	
}


void
TL_LoggedInClients::removeClient(uint64_t& client_)
{
	TL_ScopedMutex(_p->_setProtect);
	_p->_clients.erase(client_);
}


void
TL_LoggedInClients::removeClients(const TL_ClientListType& clients_)
{
	std::set<uint64_t>& clients = _p->_clients;
	TL_ClientListType::const_iterator iter = clients_.begin();

	TL_ScopedMutex(_p->_setProtect);
	for(; iter!=clients_.end(); ++iter)
	{
		clients.erase( (*iter) );
	}
}


bool
TL_LoggedInClients::clientExists(uint64_t& client_)
{
	TL_ScopedMutex(_p->_setProtect);
	bool ret (
		_p->_clients.find(client_) != _p->_clients.end() 
		);

	return ret;
}



TL_LocalClients* TL_LocalClients::_LocalClients = NULL;


TL_LocalClients::TL_LocalClients():TL_LoggedInClients()
{}


TL_LocalClients::~TL_LocalClients()
{}


TL_LocalClients*
TL_LocalClients::Instance()
{
	if(TL_LocalClients::_LocalClients == NULL)
	{	
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		TL_LocalClients::_LocalClients =  new TL_LocalClients();
	}
	return TL_LocalClients::_LocalClients;	
}


void
TL_LocalClients::Destroy()
{
	assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
	delete TL_LocalClients::_LocalClients;
}



TL_RemoteClients* TL_RemoteClients::_remoteClients = NULL;


TL_RemoteClients::TL_RemoteClients():TL_LoggedInClients()
{}


TL_RemoteClients::~TL_RemoteClients()
{}


TL_RemoteClients*
TL_RemoteClients::Instance()
{
	if(TL_RemoteClients::_remoteClients == NULL)
	{	
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		TL_RemoteClients::_remoteClients =  new TL_RemoteClients();
	}
	return TL_RemoteClients::_remoteClients;	
}


void
TL_RemoteClients::Destroy()
{
	assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
	delete TL_RemoteClients::_remoteClients;
}


} // end of ThreeLight namespace
