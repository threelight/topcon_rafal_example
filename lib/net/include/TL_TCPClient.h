#ifndef __TL_TCPCLIENT__
#define __TL_TCPCLIENT__


#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>




namespace ThreeLight
{


class TL_TCPConnectionEstablished_I
{
public:
	TL_TCPConnectionEstablished_I();
	virtual ~TL_TCPConnectionEstablished_I();

	virtual void result_I(int socket_);
};


class TL_TCPClient
{
public:

	TL_TCPClient();
	~TL_TCPClient();
	
	/*! \brief 75 seconds is default wait time
	*/
	static int connect(const char* host_, const char* port_,
			const struct timespec* time_=NULL);

	
	/*! \brief not implemented yet
	 *	it would be nice to have
	 */
	void connect(const char* host_, const char* port_,
			TL_TCPConnectionEstablished_I* callBack_,
			const struct timespec* time_=NULL);

	
private:


};


}// End of ThreeLight Namespace


#endif
