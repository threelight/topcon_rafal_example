#include <map>
#include <string.h>

#include "TL_ProtocolLogic.h"
#include "TL_LoggedInClients.h"
#include "TL_Mutex.h"
#include "TL_MessageDispatcher.h"
#include "TL_Log.h"
#include "TL_Connection.h"
#include "TL_Login.h"
#include "TL_Exception.h"


namespace ThreeLight
{


template<>
struct ImplOf<TL_ProtocolLogic>
{
	ImplOf(const unsigned long& maxQueueSize_):
		_maxQueueSize( _maxQueueSize ),
		_localClients(TL_LocalClients::Instance()),
		_dispatcher( TL_MessageDispatcher::Instance() ),
		_initialized(true)
	{
		_entryQueue.clear();
		_exitQueue.clear();
	}

	// reply for the same client
	TL_Mutex _entryProtect;
	std::map<uint64_t, TL_ProtocolLogic::TL_ListLogicStruct*> _entryQueue;

	TL_Mutex _exitProtect;
	// this is only for replies which are for the same connection
	std::map<uint64_t, TL_ProtocolLogic::TL_ListLogicStruct*> _exitQueue;

	unsigned long _maxQueueSize;
	TL_LocalClients *_localClients;
	unsigned long _currentQueueSize;
	TL_MessageDispatcher *_dispatcher;
	bool _initialized;

	static TL_ProtocolLogic* _Logic;
};


TL_ProtocolLogic* ImplOf<TL_ProtocolLogic>::_Logic = NULL;


TL_ProtocolLogic*
TL_ProtocolLogic::Instance()
{
	if(ImplOf<TL_ProtocolLogic>::_Logic == NULL)
	{
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		// should get that from config
		unsigned long maxCommands = 1024;
		ImplOf<TL_ProtocolLogic>::_Logic =  new TL_ProtocolLogic(maxCommands);
	}
	return ImplOf<TL_ProtocolLogic>::_Logic;
}


TL_ProtocolLogic::TL_ProtocolLogic(const unsigned long& maxCommands_):
	_p( new ImplOf<TL_ProtocolLogic>(maxCommands_) )
{}


TL_ProtocolLogic::~TL_ProtocolLogic()
{
	if(_p->_initialized)
	{
		throw TL_Exception<TL_ProtocolLogic>("TL_ProtocolLogic::~TL_ProtocolLogic",
			"You forgot to call the destroy function");
	}
}


// if it is not NULL - reply is ready straight away
char*
TL_ProtocolLogic::addCommand(
	char* command_,
	TL_Connection* conn_,
	const uint64_t& groupKey_)
{
	if( (command_ == NULL) || (conn_ == NULL) )
	{
		if(command_ == NULL)
		{
			LOG(this, "TL_ProtocolLogic::addCommand" , TL_Log::NORMAL,
			"Serious problem: command_==NULL" );
		}
		if(conn_ == NULL)
		{
			LOG(this, "TL_ProtocolLogic::addCommand" , TL_Log::NORMAL,
			"Serious problem: conn_==NULL" );
		}
		return NULL;
	}
	
	uint8_t what;
	uint16_t srcSize = 0;
	uint8_t clientsCount = 0;
	uint16_t dataSize = 0;
	uint16_t destSize = 0;
	uint64_t *dstClientId;
	char *destCommand;
	char *pass;
	uint64_t srcClientId;


	GET_WHAT(what, command_);
	
	switch (what)
	{
	case LOGIN:
		// it may couse problems !!
		if(conn_->getState() != TL_Connection::INIT)
		{
			// TO DO: disconnect the bustard
		}

		GET_ID(srcClientId, command_);
		pass = ( command_ + SRC_PASS );
		
		PUT_SIZE(1, command_);

		if(TL_Login::Instance()->check(srcClientId, pass))
		{
			*(command_ + REP_DATA) = 0; //ok
			conn_->setState(TL_Connection::TALK);
			conn_->setClientId(srcClientId);

			_p->_localClients->addClient(srcClientId);
		}
		else
		{
			*(command_ + REP_DATA) = 1; //login failed
		}

		// enter data into exit Queue
		_p->_exitProtect.lock();
		if(_p->_exitQueue.insert( pair<uint64_t,
				TL_ProtocolLogic::TL_ListLogicStruct*>
			(groupKey_, NULL) ).second)
		{
			_p->_exitQueue[groupKey_] = new TL_ListLogicStruct;
		}
		_p->_exitQueue[groupKey_]->push_back( new TL_LogicStruct(command_, conn_) );
		_p->_exitProtect.unlock();
		
		break;
	case CHAT:
		// check if you are in legitimate talk state
		if(conn_->getState() != TL_Connection::TALK)
		{
			// TO DO: disconnect the bustard
		}
		
		GET_ID(srcClientId, command_);
		if( conn_->clientId() != srcClientId )
		{
			// TO DO: disconnect the bustard
		}

		GET_CLIENTS_COUNT(clientsCount, command_);
		GET_SIZE(srcSize, command_);

		dataSize = srcSize - ( STATE_S + CLIENT_S + COUNT_S) -
				(clientsCount * CLIENT_S);
		// calculate the dest size
		//	size + state + id + data
		destSize = SIZE_S + STATE_S + CLIENT_S + dataSize;

		
		// send the same to all clients
		for(int i=0; i<clientsCount; ++i)
		{
			destCommand = new char[destSize];
			// set up size
			PUT_SIZE(destSize-2, destCommand);
			// set up state
			PUT_WHAT(TALK, destCommand);
			// set up src id
			memcpy(command_ + SRC_DSTID, destCommand + DST_ID, CLIENT_S);
			// set up data
			memcpy(command_ + DST_ID + ( CLIENT_S * clientsCount),
				destCommand + DST_DATA, dataSize);
			// set up dest client
			dstClientId = (uint64_t*) (command_ + DST_ID + ( CLIENT_S * i ) );
			*dstClientId = ntoh_64(*dstClientId);
			// put the responce to the Dispather
			_p->_dispatcher->addMessage(*dstClientId, destCommand);
		}
		// done with command
		delete command_;

		break;

	default:
		break;
		// problem
	}

	return NULL;
}


TL_ProtocolLogic::TL_ListLogicStruct*
TL_ProtocolLogic::getReply(const uint64_t& groupKey_)
{
	TL_ListLogicStruct *ret=NULL;

	_p->_exitProtect.lock();
	if(_p->_exitQueue.size())
	{	
		ret = _p->_exitQueue[groupKey_];
		_p->_exitQueue.erase(groupKey_);
	}
	_p->_exitProtect.unlock();

	return ret;
}


void
TL_ProtocolLogic::Destroy()
{
	
	if(ImplOf<TL_ProtocolLogic>::_Logic->_p->_initialized)
	{
		try
		{
			ImplOf<TL_ProtocolLogic>::_Logic->_p->_entryProtect.destroy();
			ImplOf<TL_ProtocolLogic>::_Logic->_p->_exitProtect.destroy();
		}
		catch (std::exception &e)
		{
			TL_ExceptionComply eC(e);
			throw TL_Exception<TL_ProtocolLogic>("TL_ProtocolLogic::destroy",
				_TLS( eC.who() << "->" << eC.what() ) );
		}
	}
	ImplOf<TL_ProtocolLogic>::_Logic->_p->_initialized = false;
}


void
TL_ProtocolLogic::run_I() throw()
{
	TL_Log::Instance().setThreadLogFileName("ProtocolLogic");
	// Go throu entry queue and
	// recognize the command and put the result to exitLoop
	// or messageDispather
	
}


} // end of threelight namespace
