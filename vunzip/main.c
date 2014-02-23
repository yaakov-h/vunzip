#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "LzmaDec.h"
#include "allocation.h"
#include "crc32.h"
#include "vzipformat.h"

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
	
	// Assume the file path and name will be less than PATH_MAX chars.
	// If it's more, truncate indiscriminately.
	static const size_t uFileNameBufLen = PATH_MAX;
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
	
	SizeT srcLen = ulDataSize - LZMA_PROPS_SIZE;
	SizeT destLen = footer.size;
	ISzAlloc allocator = { SzAlloc, SzFree };
	
	ELzmaStatus status;
	SRes result = LzmaDecode( (Byte *)pUncompressedData, &destLen, (const Byte*)pCompressedData + LZMA_PROPS_SIZE, &srcLen, (Byte *)pCompressedData, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &allocator );
	
		
	if ( result != SZ_OK )
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

	fflush( pOutFile );
	free( pCompressedData );
	free( pUncompressedData );
	fclose( pInFile );
	fclose( pOutFile );
	return 0;
}
