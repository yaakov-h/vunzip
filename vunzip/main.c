#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "LzmaDec.h"
#include "crc32.h"

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

void * SzAlloc( void * p, size_t size ) { return malloc(size); }
void SzFree( void * p, void * address ) { return free(address); }

int main( int argc, const char * argv[] )
{
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: %s <file to vunzip>\n", argv[0] );
		return -1;
	}
	
	const char * pszInFileName = argv[1];
	FILE * pInFile = fopen( pszInFileName, "rb" );
	if ( !pInFile )
	{
		fprintf( stderr, "Error: unable to open \"%s\"\n", pszInFileName );
		return -1;
	}
	
	// Assume the input filename is <output filename>.vz<anything here is ignored>
	// Valve use <output filename>.vz.<sha-1>_<num bytes>
	char * vzLoc = strstr( pszInFileName, ".vz" );
	if ( vzLoc == NULL )
	{
		fprintf( stderr, "Unable to determine output filename. Expected to be <file> where input file name is <file>.vz or <file>.vz.<blah>\n" );
		return -1;
	}
	
	// Assume the file path and name will be less than 1024 chars.
	// If it's more, truncate indiscriminately.
	static const size_t uFileNameBufLen = 1024;
	char pszOutFileName[uFileNameBufLen];
	memset( pszOutFileName, 0, uFileNameBufLen - 1 );
	size_t uNameLen = vzLoc - pszInFileName;
	memcpy( pszOutFileName, pszInFileName, uNameLen < uFileNameBufLen ? uNameLen : uFileNameBufLen );
	pszOutFileName[uFileNameBufLen - 1] = '\0';
	
	FILE * pOutFile = fopen( pszOutFileName, "wb" );
	if ( !pOutFile )
	{
		fprintf( stderr, "Error: unable to open \"%s\"\n", pszOutFileName );
		return -1;
	}
	
	fseek( pInFile, 0, SEEK_SET );
	fseek( pOutFile, 0, SEEK_SET );
	
	struct VZHeader header = { };
	fread( &header, 1, sizeof( header ), pInFile );
	if ( header.V != 'V' || header.Z != 'Z' )
	{
		fprintf( stderr, "Invalid vzip file: Header was '%c%c', expecting 'VZ'\n", header.V, header.Z );
		return -1;
	}
	
	if ( header.version != 'a' )
	{
		fprintf( stderr, "Unknown vzip type. Version was '%c', but only 'a' is supported", header.version );
		return -1;
	}

	// time_t rtime_created = header.rtime_created;
	// char szTime[17]; // 1234-67-90 23:56 <-- 16 chars + \0
	// struct tm * time = localtime( &rtime_created );
	// strftime( szTime, sizeof(szTime), "%F %R", time );
	
	// fprintf( stdout, "Found vzip file. Version: '%c', timestamp: %s\n", header.version, szTime );
	
	struct VZFooter footer = { };
	fseek( pInFile, -sizeof( footer ), SEEK_END );
	fread( &footer, 1, sizeof( footer ), pInFile );
	if ( footer.z != 'z' && footer.v != 'v' )
	{
		fprintf( stderr, "Invalid vzip file: Footer was '%c%c', expecting 'zv'\n", footer.z, footer.v );
	}
	long ulFileLength = ftell( pInFile );
	
	fseek( pInFile, sizeof( struct VZHeader ), SEEK_SET );

	size_t ulDataSize = ulFileLength - sizeof( struct VZHeader ) - sizeof( struct VZFooter );
	
	char * pCompressedData = (char *)malloc( ulDataSize );
	size_t read = fread( pCompressedData, 1, ulDataSize, pInFile );
	assert( read == ulDataSize );
	assert( ftell(pInFile) == ulFileLength - sizeof(struct VZFooter) );
	
	char * pUncompressedData = (char *)malloc( footer.size );
	memset( pUncompressedData, 0, footer.size );
	
	SizeT srcLen, available = ulDataSize - LZMA_PROPS_SIZE;
	SizeT destLen = footer.size;
	ISzAlloc allocator = { SzAlloc, SzFree };
	
	CLzmaDec state;
	LzmaDec_Construct(&state);
	SRes result = LzmaDec_Allocate( &state, (const Byte *)pCompressedData, LZMA_PROPS_SIZE, &allocator );
	
	static const size_t uBufferSize = 1024;
	
	if ( result != SZ_OK )
	{
		fprintf( stderr, "Invalid LZMA properties: %d\n", result );
	}
	else
	{
		char * pLzmaData = pCompressedData + LZMA_PROPS_SIZE;
		char * pDecompressedData = pUncompressedData;
		LzmaDec_Init( &state );
		
		ELzmaStatus status;
		
		while ( true )
		{
			
			srcLen = available;
			destLen = (SizeT)uBufferSize;
			
			result = LzmaDec_DecodeToBuf( &state, (Byte *)pDecompressedData, &destLen, (Byte *)pLzmaData, &srcLen, LZMA_FINISH_ANY, &status );
			pLzmaData += srcLen;
			available -= srcLen;
			pDecompressedData += destLen;
			
			if ( result != SZ_OK || status == LZMA_STATUS_FINISHED_WITH_MARK || status == LZMA_STATUS_NEEDS_MORE_INPUT )
			{
				break;
			}
		}
		
		if ( status == LZMA_STATUS_NEEDS_MORE_INPUT )
		{
			fprintf( stderr, "Data error during decompression!\n" );
		}
		else if ( result != SZ_OK )
		{
			fprintf( stderr, "Error while decompressing: SRes result %d\n", result );
		}
		else
		{
			uint32_t uCalculatedChecksum = crc32( 0, pUncompressedData, footer.size );
			
 			if ( uCalculatedChecksum != footer.crc )
			{
				fprintf(stderr, "Checksum mismatch. The data may be corrupt.\n");
			}
			else
			{
				fwrite( pUncompressedData, 1, footer.size, pOutFile );
			}
		}
	}
	
	fflush( pOutFile );
	free( pCompressedData );
	free( pUncompressedData );
	fclose( pInFile );
	fclose( pOutFile );
	return 0;
}
