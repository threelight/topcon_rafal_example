#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <err.h>
#include <stdlib.h>
#include <map>
#include <list>
#include <stdint.h>


#include "TL_Thread.h"
using namespace ThreeLight;
pthread_t __dummyThreatId = TL_Thread::MainThreatId();

#include "TL_MemoryLeakReport.h"
#include "TL_GlobalExceptionHandler.h"
#include "TL_GlobalStaticDestroyer.h"
#include "TL_ThreadManager.h"
#include "TL_SignalManager.h"
#include "TL_Mutex.h"
#include "TL_Time.h"
#include "TL_Log.h"
#include "TL_SharedPtr.h"
#include "TL_OSUtil.h"
#include "TL_Exception.h"
#include "TL_Pimpl.h"
#include "TL_SignalManager.h"
#include "TL_Semaphore.h"
#include "TL_TCPServer.h"
#include "TL_EventBuffer.h"
#include "TL_Db.h"
#include "TL_Login.h"
#include "TL_MessageDispatcher.h"
#include "TL_Central.h"
#include "TL_LoggedInClients.h"
#include "TL_ProtocolLogic.h"


TL_GlobalExceptionHandler __GlobalExceptionHandler;

int globalInit(void)
{
	TL_GlobalStaticDestroyer::Instance();
	TL_SignalManager::Instance();
	TL_EventBuffer::Instance();
	TL_ThreadManager::Instance();
	TL_Db::Instance();
	TL_Login::Instance();
	TL_MessageDispatcher::Instance();
	TL_LocalClients::Instance();
	TL_RemoteClients::Instance();
	TL_ProtocolLogic::Instance();
	TL_Log::Instance().setLogDir("/tmp/");
	TL_Log::Instance().setThreadLogFileName("test");
	return 0;
}

int __dummyGlobalInit = globalInit();



using namespace std;


int main()
{
	TL_Central central;
	central.start();

#if defined(DEBUG) && defined(MEMORYLEAKREPORT)
	showUnfreedMemory();
#endif
}
