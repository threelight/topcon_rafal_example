#ifndef __TL_EPOLLSERVER__
#define __TL_EPOLLSERVER__


#include <sys/types.h>
#include <sys/socket.h>
#include <list>


#include "TL_Pimpl.h"
#include "TL_Thread.h"


namespace ThreeLight
{

/*! \brief Server should be runnig in a separate thread
 */

class TL_EPollServer:
	public TL_Thread
{
public:
	TL_EPollServer(int epollSize_, int eventsSize_=100);
	virtual ~TL_EPollServer();


	enum Event
	{
		// New connection	
		NEW = 0x00000000,

		// The associated file is available for read(2) operations.
		IN = 0x00000001,

		// The associated file is available for write(2) operations.
		OUT = 0x00000002,

		// Stream socket peer closed connection, or shut down writing half of connection.
		// (This flag is especially useful for writing simple code to detect peer shutdown
                // when using Edge Triggered monitoring.)
		MSG = 0x00000004,

		// There is urgent data available for read(2) operations.
		PRI = 0x00000008,

		// Error condition happened on the associated file descriptor.
		// epoll_wait(2) will always wait for this event; it is not necessary to set it in events.
		ERR = 0x00000010,

		// Hang up happened on the associated file descriptor.
		// epoll_wait(2) will always wait for this event; it is not necessary to set it in events.
		HUP = 0x00000020
	};

	/*! \brief It changes only the size of completed connections waiting
	 * 	to be accepted
	 * to has an effect call has to be made before the listen
	 * Default is set to 1024 (system default)
	 * 
	 * The size of the queue of not completed connections can be modified by changing
	 * tcp_max_backlog_size option, see man tcp(7)
	 */
	void setBacklog(int backlog_);

	typedef struct EventType
	{
		EventType(const int &s_,
			const Event &e_,
			struct ::sockaddr_storage *a_=NULL,
			const socklen_t &l_=0):
		_socket(s_),
		_events(e_),
		_addr(a_),
		_len(l_) {}

		EventType(const EventType& e_):
		_socket(e_._socket),
		_events(e_._events),
		_addr(e_._addr),
		_len(e_._len) {}

		~EventType()
		{
			if(_addr) {
				delete _addr;
				_addr= NULL;
			}
		}
		
		void operator=(const EventType& e_)
		{
			_socket = e_._socket;
			_events = e_._events;
			_addr = e_._addr;
			_len = e_._len;
		}
		
		int _socket;
		Event _events;
		struct ::sockaddr_storage *_addr;
		socklen_t _len;
	} TL_EventType;

	typedef std::list< TL_EventType*> EventsListType;

	/*!\brief These functions are responsible for freeing the memory of events
	 */
	typedef void (*eventCollbackFun)(EventsListType *eList_);
	typedef void (*newConnectionCollbackFun)(EventsListType *eList_);

	void listen(const char* host_, const char* port_,
			eventCollbackFun eventCollback_,
			newConnectionCollbackFun newConnectionCollback_);

	void destroy();

protected:
	virtual void run_I() throw();

private:
	TL_Pimpl<TL_EPollServer>::Type _p;
};


}// End of ThreeLight Namespace


#endif
