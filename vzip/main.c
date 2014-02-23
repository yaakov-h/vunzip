#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "LzmaEnc.h"
#include "allocation.h"
#include "crc32.h"
#include "vzipformat.h"

int main( int argc, const char * argv[] )
{
	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: %s <file to vzip>\n", argv[0] );
		return -1;
	}
	
	const char * pszInFileName = argv[1];
	FILE * pInFile = fopen( pszInFileName, "rb" );
	if ( !pInFile )
	{
		fprintf( stderr, "Error: unable to open \"%s\"\n", pszInFileName );
		return -1;
	}
	
	static const size_t uFileNameBufLen = PATH_MAX;
	char pszOutFileName[uFileNameBufLen];
	memcpy( pszOutFileName, pszInFileName, strlen( pszInFileName ) );
	strncat( pszOutFileName, ".vz", strlen( ".vz" ) );
	
	FILE * pOutFile = fopen( pszOutFileName, "wb" );
	if ( !pOutFile )
	{
		fprintf( stderr, "Error: unable to open \"%s\"\n", pszOutFileName );
		return -1;
	}
	
	fseek( pInFile, 0, SEEK_END );
	size_t uInFileLen = ftell( pInFile );
	fseek( pInFile, 0, SEEK_SET );
	
	fseek( pOutFile, 0, SEEK_SET );
	
	struct VZHeader header = { };
	header.V = 'V';
	header.Z = 'Z';
	header.version = 'a';
	
	time_t now;
	time( &now );
	header.rtime_created = (uint32_t)now;
	
	struct VZFooter footer = { };
	footer.size = (uint32_t)uInFileLen;
	footer.z = 'z';
	footer.v = 'v';
	
	char * pRawData = (char *)malloc( uInFileLen );
	fread( pRawData, 1, uInFileLen, pInFile );
	
	footer.crc = crc32(0, pRawData, uInFileLen );
	
	size_t uCompressedBufferSize = uInFileLen + ( uInFileLen / 3 ) + 128;
	char * pCompressedData = (char *)malloc( uCompressedBufferSize );
	
	SizeT uCompressedDataLen = uCompressedBufferSize - LZMA_PROPS_SIZE;
	SizeT uLzmaPropsLen = LZMA_PROPS_SIZE;
	
	ISzAlloc allocator = { SzAlloc, SzFree };
	CLzmaEncProps props;
	
	SRes result = LzmaEncode( (Byte *)pCompressedData + LZMA_PROPS_SIZE, &uCompressedDataLen, (Byte *)pRawData, uInFileLen, &props, (Byte *)pCompressedData, &uLzmaPropsLen, props.writeEndMark, NULL, &allocator, &allocator );

	int iRetVal = 0;
	if (result != SZ_OK)
	{
		fprintf(stderr, "Error compressing data: %d", result);
		iRetVal = -1;
	}
	
	fseek( pOutFile, 0, SEEK_SET );
	fwrite( &header, 1, sizeof( header ), pOutFile) ;
	fwrite( pCompressedData, 1, uLzmaPropsLen + uCompressedDataLen, pOutFile );
	fwrite( &footer, 1, sizeof( footer ), pOutFile );
	fflush( pOutFile );
	
	free( pCompressedData );
	free( pRawData );
	fclose( pInFile );
	fclose( pOutFile );
	return 0;
}