#include <signal.h>
#include <iostream>


#include "TL_Central.h"
#include "TL_EPollServer.h"
#include "TL_Exception.h"
#include "TL_EventBuffer.h"
#include "TL_SocketEventManager.h"
#include "TL_Time.h"
#include "TL_SignalManager.h"
#include "TL_ThreadManager.h"
#include "TL_Log.h"


namespace ThreeLight {


TL_Central::TL_Central()
{
}


TL_Central::~TL_Central()
{

}


void
TL_Central::signal(int signum_)
{
}


void
TL_Central::start()
{
	// TO DO - take info from configuration

	// create the pool server
	TL_EPollServer server(100, 1000);
		
	TL_SocketEventManager eventManager(1);

	TL_EventBuffer::Instance().setBufferSize(1000);

	try
	{
		// start event manager before server to be ready to accept events
		eventManager.start();
		
		server.listen("*", "3333", TL_EventBuffer::addEvents, TL_EventBuffer::addNewConnections);
	
		server.start();

		TL_SignalManager::Instance().addSignalReceiver(this, SIGHUP);
		// loop waiting for signals
		for(;;)
		{
			TL_Timespec theWait(TL_Timespec::NOW);
			theWait.add(1, 0); // 1 sec
			TL_SignalManager::Instance().process(theWait);
		}
	}
	catch (std::exception& e)
	{
		TL_ExceptionComply eC(e);

		LOG(this, "TL_Central::start" , TL_Log::NORMAL,
			eC.what() << " " << eC.who() );
	}

	TL_Timespec theWait(TL_Timespec::NOW);
	theWait.add(10,0);
	TL_ThreadManager::Instance().cancelAllThreads(theWait);
	server.destroy();
	eventManager.destroy();
}


} // end of threelight namespace
