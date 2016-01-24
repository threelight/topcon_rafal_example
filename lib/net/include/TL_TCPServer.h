#ifndef __TL_TCPSERVER__
#define __TL_TCPSERVER__


#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_TCPServer
{
public:

	TL_TCPServer();
	~TL_TCPServer();

	/*! \brief It changes only the size of completed connections waiting
	 * 	to be accepted
	 * Call has to be made before the listen, to has an effect
	 * default is set to 1024
	 * 
	 * The size of the queue of not completed connections can be made by changing
	 * tcp_max_backlog_size option, see man tcp(7)
	 */
	void setBacklog(int backlog_);

	int listen(const char* host_, const char* port_);
	
	/*! \brief Call this only after the listen
	 */
	void setBlocking(bool blocking_);

	/*! \brief Waits time_ for the connection to be made
	 *	If signal is caught it may wait from scratch
	 */
	int accept(const struct timespec* time_,
			struct ::sockaddr_storage *client_=NULL,
			socklen_t *len_=NULL);

	void destroy();

private:
	TL_Pimpl<TL_TCPServer>::Type _p;

};


}// End of ThreeLight Namespace


#endif
