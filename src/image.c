#include "image.h"
#include "output.h"
#include "global.h"
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>


#define CAST_TO_BYTE_PTR(VOID_P) \
	((byte_t* )VOID_P)

#define RANGE_FROM_TO( VAL, SRC_MAX, DST_MAX ) \
	((VAL) / (SRC_MAX) * (DST_MAX))

#define GET_CHANNEL( VALUE, SIZE, POS ) \
	(((VALUE) & (((1 << (SIZE))-1) << (POS))) >> (POS))

#define GET_CHANNEL_SIMPLE(FORMAT,CHANNEL,PIXEL) \
	GET_CHANNEL(*(PIXEL+CAT2(FORMAT,CHANNEL,_BYTE)),CAT2(FORMAT,CHANNEL,_SIZE),CAT2(FORMAT,CHANNEL,_POS))

/*****************
 * RGB332
 *****************/

#define RGB332_SIZE 1
#define RGB332_RED_BYTE 0
#define RGB332_RED_SIZE 3
#define RGB332_RED_POS 5

#define RGB332_GREEN_BYTE 0
#define RGB332_GREEN_SIZE 3
#define RGB332_GREEN_POS 2

#define RGB332_BLUE_BYTE 0
#define RGB332_BLUE_SIZE 2
#define RGB332_BLUE_POS 0

#define RGB332_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(RGB332,_RED,PIXEL)
#define RGB332_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(RGB332,_GREEN,PIXEL)
#define RGB332_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(RGB332,_BLUE,PIXEL)

/*****************
 * ARGB444 or XRGB444 
 *****************/

#define XRGB444_SIZE 2

#define XRGB444_RED_BYTE 1
#define XRGB444_RED_SIZE 4
#define XRGB444_RED_POS 0

#define XRGB444_GREEN_BYTE 0
#define XRGB444_GREEN_SIZE 4
#define XRGB444_GREEN_POS 4

#define XRGB444_BLUE_BYTE 0
#define XRGB444_BLUE_SIZE 4
#define XRGB444_BLUE_POS 0

#define XRGB444_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(XRGB444,_RED,PIXEL)
#define XRGB444_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(XRGB444,_GREEN,PIXEL)
#define XRGB444_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(XRGB444,_BLUE,PIXEL)

/*****************
 * ARGB555 or XRGB555
 *****************/

#define XRGB555_SIZE 2

#define XRGB555_RED_BYTE 1
#define XRGB555_RED_SIZE 5
#define XRGB555_RED_POS 2

#define XRGB555_GREEN_SIZE 5

#define XRGB555_BLUE_BYTE 0
#define XRGB555_BLUE_SIZE 5
#define XRGB555_BLUE_POS 0

#define XRGB555_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(XRGB555,_RED,PIXEL)
#define XRGB555_GET_GREEN(PIXEL) \
	GET_CHANNEL(PIXEL[0],3,5) \
	| (GET_CHANNEL(PIXEL[1],2,0) << 3)
#define XRGB555_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(XRGB555,_BLUE,PIXEL)

/*****************
 * RGB565 
 *****************/

#define RGB565_SIZE 2

#define RGB565_RED_BYTE 1
#define RGB565_RED_SIZE 5
#define RGB565_RED_POS 3

#define RGB565_GREEN_SIZE 6

#define RGB565_BLUE_BYTE 0
#define RGB565_BLUE_SIZE 5
#define RGB565_BLUE_POS 0

#define RGB565_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(RGB565,_RED,PIXEL)
#define RGB565_GET_GREEN(PIXEL) \
	GET_CHANNEL(PIXEL[0],3,5) \
	| (GET_CHANNEL(PIXEL[1],3,0) << 3)
#define RGB565_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(RGB565,_BLUE,PIXEL)

/*****************
 * XRGB555X or ARGB555X 
 *****************/

#define XRGB555X_SIZE 2

#define XRGB555X_RED_BYTE 0
#define XRGB555X_RED_SIZE 5
#define XRGB555X_RED_POS 2

#define XRGB555X_GREEN_SIZE 5

#define XRGB555X_BLUE_BYTE 1
#define XRGB555X_BLUE_SIZE 5
#define XRGB555X_BLUE_POS 0

#define XRGB555X_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(XRGB555X,_RED,PIXEL)
#define XRGB555X_GET_GREEN(PIXEL) \
	(GET_CHANNEL(PIXEL[0],2,0) << 3) \
	| GET_CHANNEL(PIXEL[1],3,5)
#define XRGB555X_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(XRGB555X,_BLUE,PIXEL)

/*****************
 * RGB565X 
 *****************/

#define RGB565X_SIZE 2

#define RGB565X_RED_BYTE 0
#define RGB565X_RED_SIZE 5
#define RGB565X_RED_POS 3

#define RGB565X_GREEN_SIZE 6

#define RGB565X_BLUE_BYTE 1
#define RGB565X_BLUE_SIZE 5
#define RGB565X_BLUE_POS 0

#define RGB565X_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(RGB565X,_RED,PIXEL)
#define RGB565X_GET_GREEN(PIXEL) \
	(GET_CHANNEL(PIXEL[0],3,0) << 3) \
	| GET_CHANNEL(PIXEL[1],3,5)
#define RGB565X_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(RGB565X,_BLUE,PIXEL)

/*****************
 * BGR24
 *****************/

#define BGR24_SIZE 3
#define BGR24_RED_BYTE 2
#define BGR24_RED_SIZE 8
#define BGR24_RED_POS 0

#define BGR24_GREEN_BYTE 1
#define BGR24_GREEN_SIZE 8
#define BGR24_GREEN_POS 0

#define BGR24_BLUE_BYTE 0
#define BGR24_BLUE_SIZE 8
#define BGR24_BLUE_POS 0

#define BGR24_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(BGR24,_RED,PIXEL)
#define BGR24_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(BGR24,_GREEN,PIXEL)
#define BGR24_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(BGR24,_BLUE,PIXEL)

/*****************
 * RGB24
 *****************/

#define RGB24_SIZE 3
#define RGB24_RED_BYTE 0
#define RGB24_RED_SIZE 8
#define RGB24_RED_POS 0

#define RGB24_GREEN_BYTE 1
#define RGB24_GREEN_SIZE 8
#define RGB24_GREEN_POS 0

#define RGB24_BLUE_BYTE 2
#define RGB24_BLUE_SIZE 8
#define RGB24_BLUE_POS 0

#define RGB24_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(RGB24,_RED,PIXEL)
#define RGB24_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(RGB24,_GREEN,PIXEL)
#define RGB24_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(RGB24,_BLUE,PIXEL)

/*****************
 * BGR666
 *****************/
// |byte 0|byte 1|byte 2|
// |bbbbbbgg|ggggrrrr|rr______|

#define BGR666_SIZE 3

#define BGR666_RED_SIZE 6
#define BGR666_GREEN_SIZE 6

#define BGR666_BLUE_BYTE 0
#define BGR666_BLUE_SIZE 6
#define BGR666_BLUE_POS 2

#define BGR666_GET_RED(PIXEL) \
	(GET_CHANNEL(PIXEL[1],4,0) << 2) \
	| GET_CHANNEL(PIXEL[2],2,6)
#define BGR666_GET_GREEN(PIXEL) \
	(GET_CHANNEL(PIXEL[0],2,0) << 4) \
	| GET_CHANNEL(PIXEL[1],4,4)
#define BGR666_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(BGR666,_BLUE,PIXEL)

/*****************
 * ABGR32 or XBGR32
 *****************/
// |byte 0|byte 1|byte 2|byte3|
// |b|g|r|x|

#define XBGR32_SIZE 4

#define XBGR32_RED_BYTE 2
#define XBGR32_RED_SIZE 8
#define XBGR32_RED_POS 0

#define XBGR32_GREEN_BYTE 1
#define XBGR32_GREEN_SIZE 8
#define XBGR32_GREEN_POS 0

#define XBGR32_BLUE_BYTE 0
#define XBGR32_BLUE_SIZE 8
#define XBGR32_BLUE_POS 0

#define XBGR32_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(XBGR32,_RED,PIXEL)
#define XBGR32_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(XBGR32,_GREEN,PIXEL)
#define XBGR32_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(XBGR32,_BLUE,PIXEL)

/*****************
 * ARGB32 or XRGB32
 *****************/
// |byte 0|byte 1|byte 2|byte3|
// |x|b|g|r|

#define XRGB32_SIZE 4

#define XRGB32_RED_BYTE 1
#define XRGB32_RED_SIZE 8
#define XRGB32_RED_POS 0

#define XRGB32_GREEN_BYTE 2
#define XRGB32_GREEN_SIZE 8
#define XRGB32_GREEN_POS 0

#define XRGB32_BLUE_BYTE 3
#define XRGB32_BLUE_SIZE 8
#define XRGB32_BLUE_POS 0

#define XRGB32_GET_RED(PIXEL) GET_CHANNEL_SIMPLE(XRGB32,_RED,PIXEL)
#define XRGB32_GET_GREEN(PIXEL) GET_CHANNEL_SIMPLE(XRGB32,_GREEN,PIXEL)
#define XRGB32_GET_BLUE(PIXEL) GET_CHANNEL_SIMPLE(XRGB32,_BLUE,PIXEL)

/*****************
 * Utils
 *****************/

#define CAT(a, ...) a ## __VA_ARGS__
#define CAT2(a,b, ...) a ## b ## __VA_ARGS__
#define CAT3(a,b,c ...) a ## b ## c ## __VA_ARGS__

#define GET_SIZE(FORMAT) \
	CAT(FORMAT,_SIZE)

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


#define CONVERT_PIXEL(FORMAT,SRC_BUFFER,DST_BUFFER) { \
	for( uint i=0; i< size/GET_SIZE(FORMAT); i++ ) { \
		const byte_t* current_pixel = &SRC_BUFFER[i*GET_SIZE(FORMAT)]; \
		DST_BUFFER[i*3+0] = GET_RED(FORMAT,current_pixel); \
		DST_BUFFER[i*3+1] = GET_GREEN(FORMAT,current_pixel); \
		DST_BUFFER[i*3+2] = GET_BLUE(FORMAT,current_pixel); \
	} \
}

ret_t image_save_ppm(
		const char* comment,
		const void* buffer,
		const size_t size,
		const img_format_t format,
		const char* filename
)
{
	if( !(
			format.pixelformat == V4L2_PIX_FMT_RGB332
			|| format.pixelformat == V4L2_PIX_FMT_ARGB444
			|| format.pixelformat == V4L2_PIX_FMT_XRGB444
			|| format.pixelformat == V4L2_PIX_FMT_ARGB555
			|| format.pixelformat == V4L2_PIX_FMT_XRGB555
			|| format.pixelformat == V4L2_PIX_FMT_RGB565
			|| format.pixelformat == V4L2_PIX_FMT_ARGB555X
			|| format.pixelformat == V4L2_PIX_FMT_XRGB555X
			|| format.pixelformat == V4L2_PIX_FMT_RGB565X
			|| format.pixelformat == V4L2_PIX_FMT_BGR24
			|| format.pixelformat == V4L2_PIX_FMT_RGB24
			|| format.pixelformat == V4L2_PIX_FMT_BGR666
			|| format.pixelformat == V4L2_PIX_FMT_ABGR32
			|| format.pixelformat == V4L2_PIX_FMT_XBGR32
			|| format.pixelformat == V4L2_PIX_FMT_ARGB32
			|| format.pixelformat == V4L2_PIX_FMT_XRGB32
	) ) {
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
		if( format.pixelformat == V4L2_PIX_FMT_RGB332 ) {
			CONVERT_PIXEL(RGB332,input_buffer,write_buffer)
		}
		else if(
				format.pixelformat == V4L2_PIX_FMT_ARGB444
				|| format.pixelformat == V4L2_PIX_FMT_XRGB444
		) {
			CONVERT_PIXEL(XRGB444,input_buffer,write_buffer)
		}
		else if(
				format.pixelformat == V4L2_PIX_FMT_ARGB555
				|| format.pixelformat == V4L2_PIX_FMT_XRGB555
		) {
			CONVERT_PIXEL(XRGB555,input_buffer,write_buffer)
		}
		else if( format.pixelformat == V4L2_PIX_FMT_RGB565
		) {
			CONVERT_PIXEL(RGB565,input_buffer,write_buffer)
		}
		else if(
				format.pixelformat == V4L2_PIX_FMT_ARGB555X
				|| format.pixelformat == V4L2_PIX_FMT_XRGB555X
		) {
			CONVERT_PIXEL(XRGB555X,input_buffer,write_buffer)
		}
		else if( format.pixelformat == V4L2_PIX_FMT_RGB565X
		) {
			CONVERT_PIXEL(RGB565X,input_buffer,write_buffer)
		}
		else if( format.pixelformat == V4L2_PIX_FMT_BGR24 ) {
			CONVERT_PIXEL(BGR24,input_buffer,write_buffer)
		}
		else if( format.pixelformat == V4L2_PIX_FMT_RGB24 ) {
			CONVERT_PIXEL(RGB24,input_buffer,write_buffer)
		}
		else if( format.pixelformat == V4L2_PIX_FMT_BGR666 ) {
			CONVERT_PIXEL(BGR666,input_buffer,write_buffer)
		}
		else if(
				format.pixelformat == V4L2_PIX_FMT_ABGR32
				|| format.pixelformat == V4L2_PIX_FMT_XBGR32
		) {
			CONVERT_PIXEL(XBGR32,input_buffer,write_buffer)
		}
		else if(
				format.pixelformat == V4L2_PIX_FMT_ARGB32
				|| format.pixelformat == V4L2_PIX_FMT_XRGB32
		) {
			CONVERT_PIXEL(XRGB32,input_buffer,write_buffer)
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
