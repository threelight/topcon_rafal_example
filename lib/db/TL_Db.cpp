#include "TL_Db.h"
#include "TL_Thread.h"
#include "TL_Semaphore.h"
#include "TL_Mutex.h"
#include "TL_Exception.h"


using namespace std;


namespace ThreeLight
{


template<>
struct ImplOf<TL_Db>
{
	~ImplOf()
	{
		removeExising();
	}

	void
	removeExising()
	{
		if(_connections.size() > 0 )
		{
			std::list<mysqlpp::Query*>::iterator iterQ;
			iterQ  = _queries.begin();
			for(; iterQ != _queries.end(); ++iterQ)
			{
				delete (*iterQ);
			}
			_queries.clear();

			std::list<mysqlpp::Connection*>::iterator iter;
			iter  = _connections.begin();
			for(; iter != _connections.end(); ++iter)
			{
				(*iter)->close();
				delete (*iter);
			}
			_connections.clear();
		}
	}

	bool
	initConnection(mysqlpp::Connection* conn_)
	{

		removeExising();

		bool ret=true;;
		_connections.push_back(conn_);
		mysqlpp::Query *q = new mysqlpp::Query(conn_);
		_queries.push_back(q);

		mysqlpp::Connection *conn;
		for(uint16_t i=1; i<_count; ++i)
		{
			conn = new mysqlpp::Connection(conn_);

			if(!conn->connected())
			{
				delete conn;
				ret = false;
				break;
			}
			_connections.push_back(conn);
			q = new mysqlpp::Query(conn);
			_queries.push_back(q);
		}

		// all the connections should succed if not
		// disconect and free evertything
		if(ret == false)
		{
			removeExising();

		}

		return ret;
	}

	mysqlpp::Query*
	get()
	{
		_resource.wait();
		_protect.lock();
		mysqlpp::Query* q = _queries.back();
		_queries.pop_back();
		_protect.unlock();

		if(q == NULL)
		{
			throw TL_Exception<TL_Db>("ImplOf<TL_Db>::get()",
				"Serious problem with the amount of available resources!");
		}

		return q;
	}

	void
	giveBack(mysqlpp::Query* query_)
	{
		_protect.lock();
		_queries.push_back(query_);
		_protect.unlock();

		_resource.post();
	}

	std::list<mysqlpp::Connection*> _connections;
	std::list<mysqlpp::Query*> _queries;

	TL_Semaphore _resource;
	TL_Mutex _protect;
	uint16_t _count;

	static TL_Db *_Db;
};

TL_Db* ImplOf<TL_Db>::_Db = NULL;



TL_Db::TL_ScopedQuery::TL_ScopedQuery()
{
	_query = TL_Db::Instance()->get();
}


TL_Db::TL_ScopedQuery::~TL_ScopedQuery()
{
	if(_query != NULL)
		TL_Db::Instance()->giveBack(_query);
}


mysqlpp::Query*
TL_Db::TL_ScopedQuery::get()
{
	return _query;
}


void
TL_Db::TL_ScopedQuery::giveBack()
{
	if(_query != NULL)
	{
		TL_Db::Instance()->giveBack(_query);
		_query = NULL;
	}
}


TL_Db*
TL_Db::Instance()
{
	if(ImplOf<TL_Db>::_Db == NULL)
	{
		// only main thread can instantiate the manager
		assert(pthread_equal(TL_Thread::MainThreatId(), pthread_self()) != 0);
		// TODO get that info from configuration
		uint16_t count = 2;
		ImplOf<TL_Db>::_Db =  new TL_Db(count);
	}
	return ImplOf<TL_Db>::_Db;
}


bool
TL_Db::dbConnect(const char* db_,
		const char* host_,
		const char* user_,
		const char* passwd_,
		uint port_)
{
	mysqlpp::Connection *conn = new mysqlpp::Connection(db_,
		host_,
		user_,
		passwd_,
		port_);

	if( conn->connected() )
	{
		return _p->initConnection(conn);

	}
	else
	{
		return false;
	}
}


mysqlpp::Query*
TL_Db::get()
{
	return _p->get();
}


void
TL_Db::giveBack(mysqlpp::Query* query_)
{
	_p->giveBack(query_);
}


TL_Db::TL_Db(uint16_t count_)
{
	_p->_count = count_;

	for(int i=0; i<count_; ++i)
		_p->_resource.post();
}


TL_Db::~TL_Db()
{

}


} // end of namespace

