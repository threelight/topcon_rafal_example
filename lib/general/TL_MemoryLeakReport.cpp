#include <pthread.h>
#include <string.h>
#include <iostream>
#include <list>
#include <set>


#include "TL_MemoryLeakReport.h"


#if defined(DEBUG) && defined(MEMORYLEAKREPORT)

using namespace std;

static const int __fileSize = 256;

struct TL_NewMemory
{
	void*	_addr;
	size_t	_size;
	char	_file[__fileSize];
	int	_line;
};

typedef std::set<TL_NewMemory*> BadMemList;

BadMemList __MemoryProblemList;
static pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;

void addTrack(void* addr_, size_t size_, const char* file_, int line_)
{
	pthread_mutex_lock(&__mutex);	

	TL_NewMemory *ptr = (TL_NewMemory*)malloc(sizeof(TL_NewMemory));

	ptr->_addr = addr_;
	ptr->_size = size_;
	strncpy(ptr->_file, file_, __fileSize-1);
	ptr->_line = line_;
	
	__MemoryProblemList.insert(ptr);
	
	pthread_mutex_unlock(&__mutex);	
}


void removeTrack(void* addr_)
{
	pthread_mutex_lock(&__mutex);	

	BadMemList::iterator iter = __MemoryProblemList.begin();
	for(; iter!=__MemoryProblemList.end(); ++iter)
	{
		if( (*iter)->_addr == addr_ )
		{
			__MemoryProblemList.erase( iter );
			break;
		}
	}
	pthread_mutex_unlock(&__mutex);	
}


void showUnfreedMemory()
{
	BadMemList::iterator iter = __MemoryProblemList.begin();
	size_t totalSize=0;

	cout << "---------------------------------------" << endl;
	cout << "MEMORY LEAK REPORT" << endl;
	for(; iter!=__MemoryProblemList.end(); ++iter)
	{
		cout << " File: " << (*iter)->_file <<
			" Line: " << (*iter)->_line <<
			" Size: " << (*iter)->_size << endl;
		totalSize += (*iter)->_size;
	}
	cout << "---------------------------------------" << endl;
	cout << "Tatal Size: " << totalSize << " Bytes" << endl;
}


#endif
