#ifndef __TL_GLOBALSTATICTESTROYER__
#define __TL_GLOBALSTATICTESTROYER__


#include "TL_Pimpl.h"

namespace ThreeLight
{


class TL_StaticPtrBase_I
{
public:
	virtual ~TL_StaticPtrBase_I(){};
	virtual void destroy_I()=0;
};


template<class T>
class TL_StaticPtr:
	public TL_StaticPtrBase_I
{
public:
	TL_StaticPtr(T& ptr_):_ptr(&ptr_){};

	virtual ~TL_StaticPtr(){}

	virtual void destroy_I()
	{
		_ptr->destroy();
	}

private:
	T* _ptr;
};



class TL_GlobalStaticDestroyer
{
public:
	static TL_GlobalStaticDestroyer&
	Instance();

	static void
	Destroy();

	int
	addObjToDesroy(TL_StaticPtrBase_I* object_);

private:
	TL_GlobalStaticDestroyer();
	~TL_GlobalStaticDestroyer();

	TL_Pimpl<TL_GlobalStaticDestroyer>::Type _p;
};


} // end of ThreeLight namespace

#endif
