#ifndef __TL_SHAREDPTR__
#define __TL_SHAREDPTR__


#include <boost/shared_ptr.hpp>


namespace ThreeLight
{

/*! \brief Template allowing swithing between normal and shared pointer.
*
* By default Ordinary pointer.
*/
template<class T, class Pointer=T>
struct TL_SharedPtr
{
	typedef Pointer* Ptr;
};


/*! \brief Patrial Specialization for shared Pointer.
*/
template<class T>
struct TL_SharedPtr<T, boost::shared_ptr<T> >
{
	typedef boost::shared_ptr<T> Ptr;
};


} // end of ThreeLight namespace


#endif
