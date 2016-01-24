#ifndef __TL_CONNECTION__
#define __TL_CONNECTION__


#include <stdint.h>


#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_ProtocolLogic;


class TL_Connection
{
public:
	enum States
	{
		INIT = 0,
		TALK = 1
	};
	
	TL_Connection(int fd_);
	~TL_Connection();
	

	void processRead();
	char* getReadMessage();

	void addMessageToWrite(char* message_);
	bool processWrite();

	// indicates if the connection should be deleted
	bool finish();

	void destroy();
	
	void setClientId(uint64_t clientId_);
	// return client Id or 0 if not set yet
	uint64_t clientId();

	int getSocket();

	//state of connection
	// eg initail, loged in
	// talking
	uint16_t getState();
	void setState(const uint16_t& state_);

private:
	TL_Pimpl<TL_Connection>::Type _p;
};


} // end of threelight namespace


#endif
