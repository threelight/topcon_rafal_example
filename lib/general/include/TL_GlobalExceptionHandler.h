#ifndef __TL_GLOBALEXCEPTIONHANDLER__
#define __TL_GLOBALEXCEPTIONHANDLER__


#include <exception>


#include "TL_Log.h"
#include "TL_Exception.h"
#include "TL_BackTrace.h"


using namespace std;


namespace ThreeLight
{


class TL_GlobalExceptionHandler
{
public:
	TL_GlobalExceptionHandler()
	{
		static TL_SingleTonHandler HandlerObj;
	}

private:
	class TL_SingleTonHandler
	{
	public:
		TL_SingleTonHandler()
		{
			set_terminate(Handler);
		}
	
		static void Handler()
		{
			// Exception from construction/destruction of global variables
			try
			{
				// re-throw
				throw;
			}
			catch(std::exception &e)
			{
				TL_ExceptionComply eC(e);
				LOG(NULL, "TL_GlobalExceptionHandler", TL_Log::NORMAL,
				"Global Exception: " << eC.who() << " " << eC.what() );

				TL_BackTrace::Trace();
			}
			catch (...)
			{
				LOG(NULL, "TL_GlobalExceptionHandler", TL_Log::NORMAL,
				"Global Exception: Unknown exception" );

				TL_BackTrace::Trace();
			}
		
			//if this is a thread performing some core activity
			abort();
			// else if this is a thread used to service requests
			// pthread_exit();
		}
	};
};


} // end of ThreeLight namespace


#endif

