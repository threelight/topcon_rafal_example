#ifndef __TL_EXCEPTION__
#define __TL_EXCEPTION__


#include <exception>
#include <string>


namespace ThreeLight {


class TL_ExceptionBase:
	public std::exception
{
public:
	TL_ExceptionBase(const char* who_, const char* what_) throw();
	TL_ExceptionBase(const std::string& who_, const std::string& what_) throw();
	virtual ~TL_ExceptionBase() throw() {}

	virtual const char* who() const throw();
	virtual const char* what() const throw();

protected:
	std::string _who;
	std::string _what;
};


template <class T>
class TL_Exception: public TL_ExceptionBase
{
public:
	TL_Exception(const char* who_, const char* what_)  throw():
		TL_ExceptionBase(who_, what_) {};

	TL_Exception(const std::string& who_, const std::string& what_) throw():
		TL_ExceptionBase(who_, what_) {};
		
	virtual ~TL_Exception() throw() {}

	typedef T Type;
};


/*! \brief Wrapper class to be able to threat std exception
*          as TL_Exception
*/
class TL_ExceptionComply
{
public:
	TL_ExceptionComply(const std::exception& exception_) throw();
	virtual ~TL_ExceptionComply() throw() {};

	virtual const char* who() const throw();
	virtual const char* what() const throw();

protected:
	const std::exception& _exception;
};


} //end of ThreeLight namespace

#endif
