#ifndef __TL_SOCKETEVENTMANGER__
#define __TL_SOCKETEVENTMANGER__


#include <list>


#include "TL_Thread.h"
#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_SocketEventManager:
	public TL_Thread
{
public:
	TL_SocketEventManager(unsigned int maxThreads);
	virtual ~TL_SocketEventManager();
	void destroy();

	typedef std::list<int> ClientListType;

	void addClientsToRemove(ClientListType &clientList_);

protected:
	virtual void run_I() throw();

private:
	TL_Pimpl<TL_SocketEventManager>::Type _p;
	
};


}

#endif
