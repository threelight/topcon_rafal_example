#ifndef __TL_LOGGEDINCLIENTS__
#define __TL_LOGGEDINCLIENTS__


#include <stdint.h>
#include <list>


#include "TL_Pimpl.h"


namespace ThreeLight
{


/*! \brief Loged in clients
*/

class TL_LoggedInClients
{
public:
	typedef std::list<uint64_t> TL_ClientListType;


	void addClient(const uint64_t& client_);
	void addClients(const TL_ClientListType& clients_);

	void removeClient(uint64_t& client_);
	void removeClients(const TL_ClientListType& clients_);

	bool clientExists(uint64_t& client_id);

protected:
	TL_Pimpl<TL_LoggedInClients>::Type _p;

	TL_LoggedInClients();
	~TL_LoggedInClients();
};


/*! \brief representation of clients loged in on this server
*/

class TL_LocalClients:
	public TL_LoggedInClients
{
public:
	static void Destroy();

	static TL_LocalClients* Instance();

private:
	TL_LocalClients();
	~TL_LocalClients();

	static TL_LocalClients *_LocalClients;
};


/*! \brief representation of clients loged in on other servers
*/

class TL_RemoteClients:
	public TL_LoggedInClients
{
public:
	static void Destroy();

	static TL_RemoteClients* Instance();

private:
	TL_RemoteClients();
	~TL_RemoteClients();

	static TL_RemoteClients *_remoteClients;
};

} // end of threelight namespace


#endif
