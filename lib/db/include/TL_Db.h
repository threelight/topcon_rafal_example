#ifndef __TL_DB__
#define __TL_DB__

#include <stdint.h>
#include <mysql++.h>


#include "TL_NoCopy.h"
#include "TL_Pimpl.h"


namespace ThreeLight
{


class TL_Db:
	public TL_NoCopy
{
public:
	class TL_ScopedQuery
	{
	public:
		TL_ScopedQuery();
		~TL_ScopedQuery();

		mysqlpp::Query* get();
		void giveBack();

	private:
		mysqlpp::Query* _query;
	};

	static TL_Db* Instance();

	bool dbConnect(const char* db_,
			const char* host_ = "",
                        const char* user_ = "",
			const char* passwd_ = "",
			uint port_ = 0
			);

private:
	/*! brief count_ - amount of connections
	*/
	TL_Db(uint16_t count_);
	virtual ~TL_Db();

	mysqlpp::Query* get();
	void giveBack(mysqlpp::Query* query_);

	TL_Pimpl<TL_Db>::Type _p;	
};


} // end of namespace


#endif
