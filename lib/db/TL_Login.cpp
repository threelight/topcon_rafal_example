#include "TL_Login.h"
#include "TL_Db.h"
#include "TL_Thread.h"
#include "TL_Log.h"


#include <mysql++.h>
#include <iostream>
#include <iomanip>


namespace ThreeLight
{


template<>
struct ImplOf<TL_Login>
{

	static TL_Db *_Db;
	static TL_Login *_Login;
};

TL_Db* ImplOf<TL_Login>::_Db = NULL;
TL_Login* ImplOf<TL_Login>::_Login = NULL;


TL_Login*
TL_Login::Instance()
{
	if(ImplOf<TL_Login>::_Login == NULL)
	{
		// only main thread can instantiate the login
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		ImplOf<TL_Login>::_Login =  new TL_Login();
	}
	return ImplOf<TL_Login>::_Login;
}


bool
TL_Login::check(const uint64_t& id_, const char* pass_)
{
	TL_Db::TL_ScopedQuery scopeQ;
	mysqlpp::Query *q = scopeQ.get();
	q->reset();

	bool ret=false;
	try
	{
		(*q) << "SELECT id FORM user WHERE id="
			<< id_ << " and passwd="
			<< mysqlpp::quote << pass_;

		mysqlpp::Result res = q->store();


		if(res.num_fields() > 0)
			ret = true;
	}
	catch (const mysqlpp::Exception& er)
	{
		LOG(NULL, "TL_Login::check" , TL_Log::NORMAL,
		"Login problem: " << er.what() );
	}

	return ret;
}


TL_Login::TL_Login()
{
	ImplOf<TL_Login>::_Db = TL_Db::Instance();
}


TL_Login::~TL_Login()
{
}


} // end of threelight namespace

