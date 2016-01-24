#include <unistd.h>
#include <string.h>
#include <list>
#include <iostream>
#include <errno.h>


#include "TL_Connection.h"
#include "TL_Log.h"
#include "TL_Mutex.h"
#include "TL_Inet.h"
#include "TL_OSUtil.h"
#include "TL_Exception.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_Connection>
{
	ImplOf(const int& socket_):
		_socket(socket_),
		_clientId(0),
		_finish(false),
		_bytesToRead(0),
		_messageSize(0),
		_bytesToWrite(0),
		_state(TL_Connection::INIT),
		_initialized(true)
	{}


	void
	processRead()
	{
		ssize_t bytesRead=0;

		if( (_messageSize==0) && (_bytesToRead==0) )
		{
			// reading the size of the message
			do
			{
				bytesRead = ::read(_socket, (char*)&_messageSize, 2);

				if(bytesRead==0)
				{
					_finish=true;
					return;
				}
			}
			while( (bytesRead == -1) && (errno == EINTR) );

			if( bytesRead == -1 )
			{
				if(errno == EAGAIN)
				{
					// should never happen as we are notified
					// with data available for reading
					LOG(NULL, "ImplOf<TL_Connection>::processRead" , TL_Log::NORMAL,
					"(1) It should never happen !" );
				}
				else
				{
					LOG(NULL, "ImplOf<TL_Connection>::processRead" , TL_Log::NORMAL,
					TL_OSUtil::GetErrorString(errno) );
					_finish = true;
				}
				return;
			}

			_messageSize = ntoh_16(_messageSize);

			if( _messageSize == 0 )
			{
				LOG(NULL, "ImplOf<TL_Connection>::processRead" , TL_Log::NORMAL,
				"Message size 0, closing the connection" );
				_finish = true;
				return;
			}
		}

		if( _messageSize )
		{
			_message = new char[_messageSize + 2];
			memset(_message, 0, _messageSize + 2);
			_mark = _message;
			_bytesToRead = _messageSize + 2;
		}

		// reading the message
		// the 2 is for the size of the next message, if any
		// and for exosting the socket
		do
		{
			bytesRead = ::read(_socket, _mark, _bytesToRead);

			if( bytesRead==0 )
			{
				_finish=true;
				return;
			}

			if( bytesRead )
			{
				// the only case where the socket is not exosted yet
				if( _bytesToRead == bytesRead )
				{
					// I assume it is always 2
					_mark += (bytesRead-2);
					uint16_t *mSize = (uint16_t*)_mark;
					_messageSize = ntoh_16( *mSize );
					*_mark = 0;

					_outMessageList.push_back(_message);

					_bytesToRead = 0;
					processRead();
				}
				// socket exosted - can leave the function
				else
				{
					_bytesToRead -= bytesRead;
					_mark += bytesRead;
					_messageSize = 0;

					if(_bytesToRead == 2)
					{
						_bytesToRead = 0;
						_outMessageList.push_back(_message);
					}
				}
				return;
			}
			
		}	// repeat reading if EINTR
		while( (bytesRead == -1) && (errno == EINTR) );
		
		if( (bytesRead == -1) )
		{
			if(errno == EAGAIN)
			{
				// should never happen as we are notified
				// with data available for reading
				LOG(NULL, "ImplOf<TL_Connection>::processRead" , TL_Log::NORMAL,
				"(2) It should never happen !" );
			}
			else
			{
				LOG(NULL, "ImplOf<TL_Connection>::processRead" , TL_Log::NORMAL,
				TL_OSUtil::GetErrorString(errno) );
				_finish = true;
			}
			return;
		}
	}

	bool
	processWrite()
	{
		ssize_t bytesWritten = 0;
		bool ret=false;

		do
		{
			if(_bytesToWrite==0)
			{
				_listProtect.lock();
				if(_outMessageList.size())
				{
					_writePtr = _outMessageList.front();
					_outMessageList.pop_front();
				}
				_listProtect.unlock();

				if(_writePtr)
				{
					_bytesToWrite = strlen(_writePtr);
					_initPtr = _writePtr;
				}
			}

			if(_writePtr)
			{
				ret = true;
				bytesWritten = ::write(_socket, _writePtr, _bytesToWrite);
				if(bytesWritten == -1)
				{
					if(errno != EAGAIN)
					{
						LOG(NULL, "ImplOf<TL_Connection>::processWrite" , TL_Log::NORMAL,
						TL_OSUtil::GetErrorString(errno) );
						_finish = true;
						break;
					}
				} else {
					_bytesToWrite -= bytesWritten;
					_writePtr += bytesWritten;

					// release the memeory
					if(_bytesToWrite==0)
					{
						delete _initPtr;
					}
				}
			}
			else
			{
				return ret;
			}
			
		}
		while( !((bytesWritten==-1) && (errno==EAGAIN)) );
		return ret;
	}


	int _socket;

	uint64_t _clientId;
	bool _finish;
	uint16_t _bytesToRead;
	uint16_t _messageSize;
	char* _message;
	char* _mark;

	std::list<char*> _inMessageList;
	TL_Mutex _listProtect;


	std::list<char*> _outMessageList;

	uint16_t _bytesToWrite;
	char* _writePtr;
	char* _initPtr;

	uint16_t _state;

	bool _initialized;
};


TL_Connection::TL_Connection(int fd_):
	_p( new ImplOf<TL_Connection>(fd_) )
{
	
}


TL_Connection::~TL_Connection()
{
	if(_p->_initialized)
	{
		throw TL_Exception<TL_Connection>("TL_Connection::~TL_Connection",
			"You forgot to call the destroy function");
	}
}


void
TL_Connection::processRead()
{
	_p->processRead();
}


char*
TL_Connection::getReadMessage()
{
	if(_p->_outMessageList.size())
	{
		char* ret = _p->_outMessageList.back();
		_p->_outMessageList.pop_back();
		return ret;
	}
	return NULL;
}


void
TL_Connection::addMessageToWrite(char* message_)
{
	TL_ScopedMutex(_p->_listProtect);
	_p->_inMessageList.push_back(message_);
}


bool
TL_Connection::processWrite()
{
	return _p->processWrite();
}


bool
TL_Connection::finish()
{
	return _p->_finish;
}


void
TL_Connection::destroy()
{
	if(_p->_initialized)
	{
		try
		{
			_p->_listProtect.destroy();
		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_Connection>("TL_Connection::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}
	}
	_p->_initialized = false;

	::close(_p->_socket);

	// do something with message queue
	// wait some time before storing it into database

	//delete itself
	delete this;
}


void
TL_Connection::setClientId(uint64_t clientId_)
{
	_p->_clientId = clientId_;
}


uint64_t
TL_Connection::clientId()
{
	return 0;
}


int
TL_Connection::getSocket()
{
	return _p->_socket;
}


uint16_t
TL_Connection::getState()
{
	return _p->_state;
}


void
TL_Connection::setState(const uint16_t& state_)
{
	_p->_state = state_;
}


} //end of threelight namespace


