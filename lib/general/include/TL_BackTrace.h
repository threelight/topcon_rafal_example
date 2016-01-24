#ifndef __TL_BACKTRACE__
#define __TL_BACKTRACE__


#include <execinfo.h>
#include <stdlib.h>


#include "TL_Log.h"


using namespace std;


namespace ThreeLight
{


class TL_BackTrace
{
public:
	static void Trace()
	{
		const int maxSize = 50;
		void * array[maxSize];
		int nSize = backtrace(array, maxSize);
		char **symbols = backtrace_symbols(array, nSize);

		LOG(NULL, "TL_BackTrace::Trace", TL_Log::NORMAL, "BackTrace: ");

		for (int i = 0; i < nSize; i++)
		{
			LOG_C(TL_Log::NORMAL, symbols[i]);
		}
	
		free(symbols);
	}
};


} //end of ThreeLight namespace


#endif
