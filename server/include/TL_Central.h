#ifndef __TL_CENTRAL__
#define __TL_CENTRAL__


#include "TL_SignalManager.h"


namespace ThreeLight
{


class TL_Central:
	public TL_SignalManager_I
{
public:
	TL_Central();
	virtual ~TL_Central();

	virtual void signal(int signum_);


	void start();
};


}


#endif
