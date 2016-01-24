#include "TL_Exception.h"


namespace ThreeLight
{

	
using namespace std;	
	
	
/******************************
TL_ExceptionBase IMPLEMENTATION
*******************************/

TL_ExceptionBase::TL_ExceptionBase(const char* who_, const char* what_) throw():
_who(who_), _what(what_)
{}

TL_ExceptionBase::TL_ExceptionBase(const string& who_, const string& what_) throw():
_who(who_), _what(what_)
{}


const char*
TL_ExceptionBase::who() const throw()
{
	return _who.c_str();
}


const char*
TL_ExceptionBase::what() const throw()
{
	return _what.c_str();
}


/*********************************
TL_ExceptionComply IMPLEMENTATION
**********************************/

TL_ExceptionComply::TL_ExceptionComply(const exception& exception_) throw():
_exception(exception_)
{}


const char*
TL_ExceptionComply::who() const throw()
{
	static const char* stdDefualtName = "STD OBJECT";
	const char* returnValue;
	const TL_ExceptionBase *pointer;

	pointer = dynamic_cast<const TL_ExceptionBase*>(&_exception);
	if(pointer==NULL) {
		returnValue = stdDefualtName;
	} else {
		returnValue = pointer->who();
	}

	return returnValue;
}


const char*
TL_ExceptionComply::what() const throw()
{
	return _exception.what();
}


} //end of ThreeLight namespace
