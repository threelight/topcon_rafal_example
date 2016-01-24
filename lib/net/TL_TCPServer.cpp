#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>

#include "TL_TCPServer.h"
#include "TL_Exception.h"
#include "TL_OSUtil.h"
#include "TL_Log.h"


using namespace std;


namespace ThreeLight
{
	
	
template<>
struct ImplOf<TL_TCPServer>
{
	void init()
	{
		_socket = 0;
		_backlog = 1024;
	}
	
	// listening socket
	int _socket;
	// size of the queue - established
	// connection waiting to be accepted
	int _backlog;
	// for select function
	fd_set _set;

};


TL_TCPServer::TL_TCPServer()
{
	_p->init();
}


TL_TCPServer::~TL_TCPServer()
{
	if(_p->_socket)
	{
		throw TL_Exception<TL_TCPServer>("TL_TCPServer::~TL_TCPServer",
			"You forgot to call the destroy function");
	}
}


void
TL_TCPServer::setBacklog(int backlog_)
{
	_p->_backlog = backlog_;
}


int
TL_TCPServer::listen(const char* host_, const char* port_)
{
	const int on = 1;
	int addrInfoError;
	struct addrinfo hints, *res, *resSave;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	addrInfoError = getaddrinfo(host_, port_, &hints, &res);
	
	if( addrInfoError != 0 )
	{
		LOG(this, "TL_TCPServer::listen" , TL_Log::NORMAL,
		"getaddrinfo problem: " << gai_strerror(addrInfoError) );
		
		throw TL_Exception<TL_TCPServer>("TL_TCPServer::listen",
			_TLS("getaddrinfo problem: " << gai_strerror(addrInfoError) ) );
	}
	
	resSave = res;
	do
	{
		_p->_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(_p->_socket < 0 ) continue;
		
		setsockopt(_p->_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
		
		if( bind(_p->_socket, res->ai_addr, res->ai_addrlen) == 0 )
			break; // success
		
		close(_p->_socket);
		
	} while ( (res = res->ai_next) != NULL );
	
	
	if(res == NULL)
	{
		throw TL_Exception<TL_TCPServer>("TL_TCPServer::listen",
			"Cannot socket or bind");
	}
	
	
	if( ::listen(_p->_socket, _p->_backlog) < 0 )
	{
		throw TL_Exception<TL_TCPServer>("TL_TCPServer::listen",
			_TLS("Listen problem: " << TL_OSUtil::GetErrorString(errno)) );
	}
	
	return _p->_socket;
}


void
TL_TCPServer::setBlocking(bool blocking_)
{
	int mask = fcntl(_p->_socket, F_GETFD, 0);
	
	if(blocking_)
	{
		fcntl(_p->_socket, F_SETFD, mask & (~O_NONBLOCK) );
	}
	else
	{
		fcntl(_p->_socket, F_SETFD, mask | O_NONBLOCK );
	}
}


int
TL_TCPServer::accept(const struct timespec* time_,
	struct ::sockaddr_storage *client_,
	socklen_t *len_)
{
	int count;

	FD_ZERO(&_p->_set);
	FD_SET(_p->_socket, &_p->_set);
	
	while( (count = pselect(_p->_socket+1, &_p->_set, NULL, NULL, time_, NULL)) < 0 )
	{
		if( errno != EINTR )
		{
			throw TL_Exception<TL_TCPServer>("TL_TCPServer::accept",
				TL_OSUtil::GetErrorString(errno));
		} 
		FD_ZERO(&_p->_set);
		FD_SET(_p->_socket, &_p->_set);
	}

	if(count>0)
	{
		int conn;
		if( (conn = ::accept(_p->_socket, (struct sockaddr*)client_, len_)) < 0)
		{
			if(errno == EAGAIN ||
				errno == EWOULDBLOCK ||
				errno == EINTR ||
				errno == ECONNABORTED ||
				errno == EPROTO )
			{
				return -1;
			}

			throw TL_Exception<TL_TCPServer>("TL_TCPServer::accept",
				TL_OSUtil::GetErrorString(errno));
		}
		return conn;
	}
	
	return -1;
}


void
TL_TCPServer::destroy()
{
	if(_p->_socket && (::close(_p->_socket) < 0) )
	{
		throw TL_Exception<TL_TCPServer>("TL_TCPServer::destroy",
			TL_OSUtil::GetErrorString(errno));
	}
	_p->_socket = 0;
}


} // end of ThreeLight namespace
