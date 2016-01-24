#ifndef __TL_CREATE_REQUEST__
#define __TL_CREATE_REQUEST__

namespace ThreeLight
{



class TL_CreateRequest
{
public:
	TL_CreateRequest()
	{}

	// allocates memory
	char* loginRequest(const char* user_, const char* pass_);
};


}

#endif
