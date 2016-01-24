#ifndef TL_SIGNALMANAGER
#define TL_SIGNALMANAGER


#include <time.h>
#include "TL_Pimpl.h"


namespace ThreeLight
{



class TL_SignalManager_I
{
public:
	virtual void signal(int signum_) = 0;
	virtual ~TL_SignalManager_I() {}
};


class TL_SignalManager
{
public:
	static TL_SignalManager& Instance();
	static void Destroy();

	void addSignalReceiver(TL_SignalManager_I *receiver_, int signum_);
	void removeSignalReceiver(TL_SignalManager_I *receiver_, int signum_);

	void sigProcMask(int how_, int signum_);
	
	void process(const struct timespec* absTime_);
private:
	
	TL_SignalManager();
	~TL_SignalManager();
	
	TL_Pimpl<TL_SignalManager>::Type _p;
};


} // end of ThreeLight namespace


#endif
