#include "allocation.h"

void * SzAlloc( void * p, size_t size )
{
	return malloc(size);
}

void SzFree( void * p, void * address )
{
	return free(address);
}