#include "image.h"
#include "output.h"
#include "global.h"
#include <stdio.h>
#include <string.h>


#define CAST_TO_BYTE_PTR(VOID_P) \
	((byte_t* )VOID_P)

#define RANGE_FROM_TO( VAL, SRC_MAX, DST_MAX ) \
	((VAL) / (SRC_MAX) * (DST_MAX))

#define GET_CHANNEL( VALUE, SIZE, POS ) \
	(((VALUE) & (((1 << (SIZE))-1) << (POS))) >> (POS))


#define RGB332_TYPE byte_t
#define RGB332_RED_SIZE 3
#define RGB332_RED_POS 5
#define RGB332_GREEN_SIZE 3
#define RGB332_GREEN_POS 2
#define RGB332_BLUE_SIZE 2
#define RGB332_BLUE_POS 0


#define RGB332_GET_RED(BYTE) \
	GET_CHANNEL( BYTE, 3, 5 )

#define RGB332_GET_GREEN(BYTE) \
	GET_CHANNEL( BYTE, 3, 2 )

#define RGB332_GET_BLUE(BYTE) \
	GET_CHANNEL( BYTE, 2, 0 )

#define CAT( a, ...) a ## __VA_ARGS__

#define GET_RED(FORMAT, PIXEL) \
	RANGE_FROM_TO( \
		CAT( FORMAT, _GET_RED )(PIXEL), \
		(1 << CAT( FORMAT, _RED_SIZE )) -1, \
		255 \
	)

#define GET_GREEN(FORMAT, PIXEL) \
	RANGE_FROM_TO( \
		CAT( FORMAT, _GET_GREEN )(PIXEL), \
		(1 << CAT( FORMAT, _GREEN_SIZE )) -1, \
		255 \
	)

#define GET_BLUE(FORMAT, PIXEL) \
	RANGE_FROM_TO( \
		CAT( FORMAT, _GET_BLUE )(PIXEL), \
		(1 << CAT( FORMAT, _BLUE_SIZE )) -1, \
		255 \
	)


ret_t image_save_ppm(
		const char* comment,
		const void* buffer,
		const size_t size,
		const img_format_t format,
		const char* filename
)
{
	if( format.pixelformat != V4L2_PIX_FMT_RGB332 ) {
		ERROR( "format not supported" );
		return RET_FAILURE;
	}
	FILE* fd = fopen( filename, "w+" );
	if( fd == NULL ) {
		return RET_FAILURE;
	}
	{
		fprintf( fd, "P6\n" );
		if( comment != NULL ) {
			fprintf( fd, "#%s\n", comment );
		}
		fprintf( fd, "%d %d 255\n",
				format.width,
				format.height
		);
		byte_t write_buffer[format.width * format.height * 3];
		const byte_t* input_buffer = CAST_TO_BYTE_PTR( buffer );
		for( uint i=0; i< size; i++ ) {
			const RGB332_TYPE current_pixel = input_buffer[i*sizeof(RGB332_TYPE)];
			const byte_t red =
				GET_RED( RGB332, current_pixel );
			const byte_t green =
				GET_GREEN( RGB332, current_pixel );
			const byte_t blue =
				GET_BLUE( RGB332, current_pixel );
			write_buffer[i*3+0] = red;
			write_buffer[i*3+1] = green;
			write_buffer[i*3+2] = blue;
		}
		for( uint i=0; i< format.width * format.height; i++ ) {
			size_t ret = fwrite( &write_buffer[i*3], 1, 3, fd );
			if( ret != 3 ) {
				fclose( fd );
				return RET_FAILURE;
			}
		}
	}
	if( -1 ==fclose( fd ) ) {
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}
