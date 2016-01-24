#ifndef __TL_PROTOCOLLOGIC__
#define __TL_PROTOCOLLOGIC__


#include <list>

#include "TL_Inet.h"
#include "TL_Pimpl.h"
#include "TL_Connection.h"
#include "TL_Thread.h"


using namespace std;


namespace ThreeLight
{


#define PUT_SIZE(size, command) *( (uint16_t*)(command+TL_ProtocolLogic::SRC_SIZE) ) = hton_16(size)
#define GET_SIZE(size, command) size = hton_16( *( (uint16_t*)(command+TL_ProtocolLogic::SRC_SIZE) ) )

#define PUT_WHAT(what, command) *(command_ + TL_ProtocolLogic::SRC_STATE) = what
#define GET_WHAT(what, command) what = *(command_ + TL_ProtocolLogic::SRC_STATE)

#define PUT_REP_DATA(data, command) *(command_ + TL_ProtocolLogic::REP_DATA) = data
#define GET_REP_DATA(data, command) data = *(command_ + TL_ProtocolLogic::REP_DATA)

#define GET_CLIENTS_COUNT(count, command) count = *(command_ + TL_ProtocolLogic::SRC_COUNT)

#define GET_ID(id, command) id =  ntoh_64( *( (uint64_t*)(command+TL_ProtocolLogic::SRC_ID) ) )


class TL_ProtocolLogic:
	public TL_Thread
{
public:
	enum LogicSizes
	{
		STATE_S		=1,
		COUNT_S		=1,
		SIZE_S		=2,
		CLIENT_S	=8
	};

	enum SrcLogic
	{
		SRC_SIZE	=0,
		SRC_STATE	=2,
		SRC_ID		=3,
		SRC_COUNT	=11,
		SRC_PASS	=11,
		SRC_DSTID	=12
	};

	enum ReplyLogic
	{
		REP_SIZE	=0,
		REP_STATE	=2,
		REP_DATA	=3
	};

	enum DestLogic
	{
		DST_SIZE	=0,
		DST_STATE	=2,
		DST_ID		=3,
		DST_DATA	=11
	};

	enum ClientState
	{
		INIT=0,
		TALK=1
	};

	enum ProtoWhat
	{
		LOGIN = 0,
		HISTORY,
		CHAT	
	};


	// if it is not NULL - reply is ready straight away
	char* addCommand(char* command_, TL_Connection* conn_,
				const uint64_t& groupKey_);
	

	typedef struct LogicStruct
	{
		LogicStruct(char* command_, TL_Connection *conn_):
			_command(command_), _conn(conn_)
		{
			_socket = _conn->getSocket();
		}

		~LogicStruct()
		{
			if(_command) delete _command;
		}

		char* _command;
		TL_Connection *_conn;
		int _socket;
	} TL_LogicStruct;

	typedef std::list<TL_LogicStruct*> TL_ListLogicStruct;
	typedef std::list<TL_LogicStruct*>::iterator TL_ListLogicStructIter;

	// returns entry from the list and removes it
	// recepient responsible for freeing the memory
	TL_ListLogicStruct* getReply(const uint64_t& groupKey_);

	static TL_ProtocolLogic* Instance();
	static void Destroy();

protected:
	/*!\brief Executes sequentally the queue of commands
	 */
	void run_I() throw();

private:
	TL_Pimpl<TL_ProtocolLogic>::Type _p;

	TL_ProtocolLogic(const unsigned long& maxCommands_);
	~TL_ProtocolLogic();
};


} // end of threelight namespace


#endif
