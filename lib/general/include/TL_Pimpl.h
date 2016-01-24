#ifndef __TL_PIMPL__
#define __TL_PIMPL__


#include <cassert>


namespace ThreeLight
{


template <class T>
struct Pimpl
{
	Pimpl():_impl(new T)
	{}

	Pimpl(T* obj_):_impl(obj_)
	{}

	~Pimpl()
	{
		delete _impl;
	}

	T*
	operator->()
	{
		return _impl;
	}

	const T*
	operator->() const
	{
		return _impl;
	}

	T&
	operator*()
	{
		return *_impl;
	}

	const T&
	operator*() const
	{
		return *_impl;
	}

private:
	T *_impl;
};


template<class T>
struct ImplOf
{
	ImplOf()
	{
		assert(false);
	}
};


template<class T>
struct TL_Pimpl
{
	typedef Pimpl<ImplOf<T> > Type;
};


} // end of ThreeLight namespace

#endif
