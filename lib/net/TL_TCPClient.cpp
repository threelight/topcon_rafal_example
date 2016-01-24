#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>



#include "TL_TCPClient.h"
#include "TL_Exception.h"
#include "TL_OSUtil.h"
#include "TL_Log.h"
#include "TL_Time.h"


namespace ThreeLight
{


TL_TCPClient::TL_TCPClient()
{
}


TL_TCPClient::~TL_TCPClient()
{
}


int
TL_TCPClient::connect(const char* host_, const char* port_,
			const struct timespec* time_)
{
	int sockfd;
	int addrInfoError;
	int mask, count, error;
	socklen_t len;
	struct addrinfo hints, *res, *resSave;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrInfoError = getaddrinfo(host_, port_, &hints, &res);

	if( addrInfoError != 0 )
	{
		LOG(NULL, "TL_TCPClient::connect" , TL_Log::NORMAL,
		"getaddrinfo problem: " << gai_strerror(addrInfoError) );
		
		throw TL_Exception<TL_TCPClient>("TL_TCPClient::connect",
			_TLS("getaddrinfo problem: " << gai_strerror(addrInfoError) ) );
	}

	resSave = res;
	fd_set rset, wset;
	

	do
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(sockfd < 0) continue;

		mask = fcntl(sockfd, F_GETFD, 0);
		fcntl(sockfd, F_SETFD, mask | O_NONBLOCK );

		if(::connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break; //success

		if(errno == EINPROGRESS)
		{
			FD_ZERO(&rset);
			FD_ZERO(&wset);
			FD_SET(sockfd, &rset);
			FD_SET(sockfd, &wset);
			while( (count = pselect(sockfd+1, &rset, &wset, NULL, time_, NULL )) < 0)
			{
				if( errno != EINTR )
				{
					LOG(NULL, "TL_TCPClient::connect" , TL_Log::NORMAL,
					"pselect problem: " << TL_OSUtil::GetErrorString(errno) );
					break;
				} 
				FD_ZERO(&rset);
				FD_ZERO(&wset);
				FD_SET(sockfd, &rset);
				FD_SET(sockfd, &wset);
			}

			if( FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) )
			{
				len = sizeof(error);
				int result;
				result = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
				if(result<0 || error)
				{
					::close(sockfd);
					continue;
				}
				break; //success
			}
			else
			{
				::close(sockfd);
				continue;
			}
		}
		else
		{
			LOG(NULL, "TL_TCPClient::connect" , TL_Log::NORMAL,
			"connect: " << TL_OSUtil::GetErrorString(errno) );
			continue;
		}
		
	} while ( (res = res->ai_next) != NULL );

	if(res == NULL)
	{
		return -1;
	}
	
	return sockfd;
}


void
TL_TCPClient::connect(const char* host_, const char* port_,
			TL_TCPConnectionEstablished_I* callBack_,
			const struct timespec* time_)
{
}


}// end of ThreeLight namespace
