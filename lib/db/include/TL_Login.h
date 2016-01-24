#ifndef __TL_LOGIN__
#define __TL_LOGIN__


#include <stdint.h>


#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_Login
{
public:
	static TL_Login* Instance();
 	
	bool check(const uint64_t& id_, const char* pass_);
private:
	
	TL_Login();
	~TL_Login();

	TL_Pimpl<TL_Login>::Type _p;
};


} // end of threelight namespace

#endif
