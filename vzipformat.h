#pragma once
#include <stdint.h>

#pragma pack( push )
#pragma pack( 1 )
struct VZHeader
{
	char V;
	char Z;
	char version;
	uint32_t rtime_created;
};

struct VZFooter
{
	uint32_t crc;
	uint32_t size;
	char z;
	char v;
};
#pragma pack( pop )