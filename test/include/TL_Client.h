#ifndef __TL_CLIENT__
#define __TL_CLIENT__


#include "TL_Pimpl.h"


namespace ThreeLight
{





class TL_Client
{
public:
	TL_Client(const char* host_, const char* port_);
	void connect();
	
	void login();

private:

	TL_Pimpl<TL_Client>::Type _p;
	
};


}


#endif
