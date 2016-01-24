#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <set>
#include <iostream>
#include <errno.h>


#include "TL_GlobalStaticDestroyer.h"
#include "TL_Exception.h"
#include "TL_OSUtil.h"
#include "TL_Log.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_GlobalStaticDestroyer>
{
	ImplOf()
	{
		if( atexit(AtExitDestroyerFun) < 0 )
		{
			throw TL_Exception<TL_GlobalStaticDestroyer>(
				"ImplOf<TL_GlobalStaticDestroyer>::ImplOf",
				TL_OSUtil::GetErrorString(errno) );
		}
	}
	
	static void
	AtExitDestroyerFun(void)
	{
		std::set<TL_StaticPtrBase_I*>::iterator iter =
			_objectSet.begin();
		try
		{
			for(; iter!=_objectSet.end(); ++iter)
			{
				(*iter)->destroy_I();
			}
			_Destroyer->Destroy();
		}
		catch (exception &e)
		{
			TL_ExceptionComply eC(e);
			LOG(NULL, "ImplOf<TL_GlobalStaticDestroyer>::AtExitDestroyerFun", TL_Log::NORMAL,
			"AtExit Exception: " << eC.who() << " " << eC.what() );
		}
	}

	static TL_GlobalStaticDestroyer *_Destroyer;
	static std::set<TL_StaticPtrBase_I*> _objectSet;
};


TL_GlobalStaticDestroyer* ImplOf<TL_GlobalStaticDestroyer>::_Destroyer = NULL;
std::set<TL_StaticPtrBase_I*> ImplOf<TL_GlobalStaticDestroyer>::_objectSet;


TL_GlobalStaticDestroyer::TL_GlobalStaticDestroyer()
{}


TL_GlobalStaticDestroyer::~TL_GlobalStaticDestroyer()
{}


TL_GlobalStaticDestroyer&
TL_GlobalStaticDestroyer::Instance()
{
	if(ImplOf<TL_GlobalStaticDestroyer>::_Destroyer == NULL)
	{
		// only main thread can instantiate the _Destroyer
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_GlobalStaticDestroyer>::_Destroyer =
			new TL_GlobalStaticDestroyer();
	}
	return *ImplOf<TL_GlobalStaticDestroyer>::_Destroyer;
}


void
TL_GlobalStaticDestroyer::Destroy()
{
	if(ImplOf<TL_GlobalStaticDestroyer>::_Destroyer != NULL)
	{
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		delete ImplOf<TL_GlobalStaticDestroyer>::_Destroyer;
		ImplOf<TL_GlobalStaticDestroyer>::_Destroyer = NULL;
	}
}


int
TL_GlobalStaticDestroyer::addObjToDesroy(TL_StaticPtrBase_I* object_)
{
	_p->_objectSet.insert(object_);
	return 1;
}

};
