#include <sys/epoll.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>


#include "TL_EPollServer.h"
#include "TL_TCPServer.h"
#include "TL_Exception.h"
#include "TL_OSUtil.h"
#include "TL_Log.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_EPollServer>
{
	TL_TCPServer _tcpServer;
	int _eventsSize;
	TL_EPollServer::eventCollbackFun _eventCallBackFun;
	TL_EPollServer::newConnectionCollbackFun _newConnectionCallBackFun;
	int _epollFd;
	int _listenFd;
};


TL_EPollServer::TL_EPollServer(int epollSize_, int eventsSize_)
{
	_p->_epollFd = 0;
	_p->_eventsSize = eventsSize_;

	if( (_p->_epollFd = epoll_create(epollSize_)) < 0)
	{
		throw TL_Exception<TL_EPollServer>("TL_EPollServer::TL_EPollServer",
			TL_OSUtil::GetErrorString(errno));
	}
}


TL_EPollServer::~TL_EPollServer()
{
	if(_p->_epollFd)
	{
		throw TL_Exception<TL_EPollServer>("TL_EPollServer::~TL_EPollServer",
			"You forgot to call the destroy function");
	}
}


void
TL_EPollServer::setBacklog(int backlog_)
{
	_p->_tcpServer.setBacklog(backlog_);
}


void
TL_EPollServer::listen(const char* host_, const char* port_,
			eventCollbackFun eventCallback_,
			newConnectionCollbackFun newConnectionCollback_)
{

	_p->_eventCallBackFun = eventCallback_;
	_p->_newConnectionCallBackFun = newConnectionCollback_;

	try
	{
		_p->_listenFd = _p->_tcpServer.listen(host_, port_);
		_p->_tcpServer.setBlocking(false);
	}
	catch (TL_ExceptionBase& e)
	{
		throw TL_Exception<TL_EPollServer>("TL_EPollServer::setup",
			_TLS( e.who() << "->" << e.what() ) );
	}
}


void
TL_EPollServer::run_I() throw()
{
	TL_Log::Instance().setThreadLogFileName("EPollServer");

	struct epoll_event ev, events[_p->_eventsSize];
	//TL_EventType *eventsToPass[_p->_eventsSize];
	EventsListType *eventsToPass=NULL;
	EventsListType *newConnectionsToPass=NULL;
	int nfds = 0;
	int maxevents = _p->_eventsSize;
	int client;
	struct ::sockaddr_storage local;
	socklen_t addrlen;
	int mask;
	int n=0;
	
	// only to make it faster
	eventCollbackFun &eventCallBack = _p->_eventCallBackFun;
	newConnectionCollbackFun &newConnectionCallBack = _p->_newConnectionCallBackFun;
	int &epollFd = _p->_epollFd;
	int &listenFd = _p->_listenFd;

	// add listening socket to epoll
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listenFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev) < 0)
	{
		::close(listenFd);
		LOG(this, "TL_EPollServer::run_I",
			TL_Log::NORMAL,
			"Cannot add listening socket to epoll: " <<
			TL_OSUtil::GetErrorString(errno) );
		// finish the thread
		return;	
	}	

	for(;;) {
		// test if thread should finish
		if( this->testCancel() )
		{
			break;
		}
		//wait indefinetly
		nfds = epoll_wait(epollFd, events, maxevents, 1000); // 1 second
		eventsToPass = NULL;
		newConnectionsToPass = NULL;

		for(n = 0; n < nfds; ++n) {

			if(events[n].data.fd == listenFd)
			{
				while ( (client = accept(listenFd,
						(struct sockaddr *) &local,
						&addrlen)) < 0)
				{
					if(errno != EAGAIN || errno != EINTR )
					{
						break;
					}
				}

				if( client < 0 )
				{
					LOG(this, "TL_EPollServer::run_I",
						TL_Log::NORMAL,
						"accept problem: " <<
						TL_OSUtil::GetErrorString(errno) );
					continue;
				}

				mask = fcntl(client, F_GETFD, 0);
				fcntl(client, F_SETFD, mask | O_NONBLOCK );

				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = client;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, client, &ev) < 0)
				{
					::close(client);
					LOG(this, "TL_EPollServer::run_I",
						TL_Log::NORMAL,
						"epoll_ctl add to poll problem: "
						<< TL_OSUtil::GetErrorString(errno) );
				}
				else
				{
					cout << "jest new client" << endl;

					if(newConnectionsToPass == NULL)
						newConnectionsToPass = new EventsListType;

					newConnectionsToPass->push_back(new TL_EventType(
						events[n].data.fd,
						NEW,
						new struct sockaddr_storage(local),
						addrlen)
						);
				}
			}
			else
			{
				Event e=NEW;
				if(events[n].data.fd & EPOLLIN)  e = (Event) (e | IN);
				if(events[n].data.fd & EPOLLOUT) e = (Event) (e | OUT);
				if(events[n].data.fd & EPOLLMSG) e = (Event) (e | MSG);
				if(events[n].data.fd & EPOLLPRI) e = (Event) (e | PRI);
				if(events[n].data.fd & EPOLLERR) e = (Event) (e | ERR);
				if(events[n].data.fd & EPOLLHUP) e = (Event) (e | HUP);
				
				cout << "jest new event " << (e & OUT) << endl;

				if(eventsToPass == NULL)
					eventsToPass = new EventsListType;

				eventsToPass->push_back(new TL_EventType(
					events[n].data.fd,
					e)
					);
			}
		}

		if(eventsToPass)
			eventCallBack(eventsToPass);

		if(newConnectionsToPass)
			newConnectionCallBack(newConnectionsToPass);
       }
}


void
TL_EPollServer::destroy()
{
	if(_p->_epollFd && (::close(_p->_epollFd) < 0) )
	{
		throw TL_Exception<TL_EPollServer>("TL_EPollServer::destroy",
			TL_OSUtil::GetErrorString(errno));
	}
	_p->_epollFd = 0;
	_p->_tcpServer.destroy();
	
}


} // end of ThreeLight namespace
