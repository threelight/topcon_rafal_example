#ifndef __TL_EVENTBUFFER__
#define __TL_EVENTBUFFER__


#include <list>


#include "TL_Pimpl.h"
#include "TL_EPollServer.h"


namespace ThreeLight
{


class TL_EventBuffer
{
public:
	static TL_EventBuffer&
	Instance();

	void setBufferSize(unsigned long maxSize_);
	void destroy();

	/*! \brief function called by EPollServer
	*/
	static void
	addEvents(TL_EPollServer::EventsListType *events_);

	static void
	addNewConnections(TL_EPollServer::EventsListType *events_);



	typedef std::list< TL_EPollServer::EventsListType* > EventsListOfListType;

	/*! \brief function called by Event Manager
	*/
	static TL_EPollServer::EventsListType*
	getEvents();

	static TL_EPollServer::EventsListType*
	getNewConnections();
	
private:
	TL_EventBuffer(unsigned long maxEventsSize_=1024);
	~TL_EventBuffer();
	
	TL_Pimpl<TL_EventBuffer>::Type _p;
	
};


} // end of theelight namespace


#endif
