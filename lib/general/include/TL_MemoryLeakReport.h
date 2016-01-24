#ifndef __TL_MEMORYLEAKREPORT__
#define __TL_MEMORYLEAKREPORT__


#if defined(DEBUG) && defined(MEMORYLEAKREPORT)


#include<new>


void addTrack(void* addr_, size_t size_, const char* file_, int line_);
void removeTrack(void* addr_);
void showUnfreedMemory();


inline void * operator new(size_t size, const char* file, int line)
{
	void *ptr = (void *)malloc(size);
	addTrack(ptr, size, file, line);
	return(ptr);
};


inline void operator delete(void *addr_)
{
	removeTrack(addr_);
	free(addr_);
};


inline void * operator new[](size_t size, const char* file, int line)
{
	void *ptr = (void *)malloc(size);
	addTrack(ptr, size, file, line);
	return(ptr);
};


inline void operator delete[](void *addr_)
{
	removeTrack(addr_);
	free(addr_);
};

#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW


#endif
#endif
