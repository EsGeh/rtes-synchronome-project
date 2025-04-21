#include "image.h"
#include "camera.h"
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

#define clamp(x,min,max) ((x)<(min) ? (min) : ((x)>(max) ? (max) : (x)))

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
 * YUYV
 *****************/

#define YUYV_SIZE 2

/*****************
 * Utils
 *****************/

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


#define CONVERT_PIXEL(FORMAT,SRC_BUFFER,DST_BUFFER,format) { \
	for( uint y=0; y<format.height; y++ ) { \
	for( uint x=0; x<format.width; x++ ) { \
		const byte_t* current_pixel = &SRC_BUFFER[y*format.bytesperline+x*GET_SIZE(FORMAT)]; \
		DST_BUFFER[(y*format.width+x)*3+0] = GET_RED(FORMAT,current_pixel); \
		DST_BUFFER[(y*format.width+x)*3+1] = GET_GREEN(FORMAT,current_pixel); \
		DST_BUFFER[(y*format.width+x)*3+2] = GET_BLUE(FORMAT,current_pixel); \
	} \
	} \
}

/************************
 * private utils decl
*************************/

uint format_pixel_size(img_format_t format);
ret_t check_format(
		const img_format_t format
);

/************************
 * API implementation
*************************/

ret_t image_save_ppm(
		const char* filename,
		const char* comment,
		const void* buffer,
		const uint buffer_size,
		const uint width,
		const uint height
)
{
	if( !(buffer_size >= image_rgb_size(width,height)) ) {
		log_error( "buffer size too small!\n" );
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
				width,
				height
		);
		for( uint i=0; i< width * height; i++ ) {
			size_t ret = fwrite( &((byte_t* )buffer)[i*3], 1, 3, fd );
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

ret_t image_convert_to_rgb(
		const img_format_t src_format,
		const void* src_buffer,
		const void* dst_buffer,
		const size_t dst_size
)
{
	if( RET_SUCCESS != check_format( src_format ) ) {
		return RET_FAILURE;
	}
	// check if dst size is sufficient:
	if( dst_size < image_rgb_size( src_format.width, src_format.height ) ) {
		log_error( "buffer size does not correspond to format size\n" );
		return RET_FAILURE;
	}
	const byte_t* input_buffer = CAST_TO_BYTE_PTR( src_buffer );
	byte_t* output_buffer = CAST_TO_BYTE_PTR( dst_buffer );
	if( src_format.pixelformat == V4L2_PIX_FMT_RGB332 ) {
		CONVERT_PIXEL(RGB332,input_buffer,output_buffer,src_format)
	}
	else if(
			src_format.pixelformat == V4L2_PIX_FMT_ARGB444
			|| src_format.pixelformat == V4L2_PIX_FMT_XRGB444
	) {
		CONVERT_PIXEL(XRGB444,input_buffer,output_buffer,src_format)
	}
	else if(
			src_format.pixelformat == V4L2_PIX_FMT_ARGB555
			|| src_format.pixelformat == V4L2_PIX_FMT_XRGB555
	) {
		CONVERT_PIXEL(XRGB555,input_buffer,output_buffer,src_format)
	}
	else if( src_format.pixelformat == V4L2_PIX_FMT_RGB565
	) {
		CONVERT_PIXEL(RGB565,input_buffer,output_buffer,src_format)
	}
	else if(
			src_format.pixelformat == V4L2_PIX_FMT_ARGB555X
			|| src_format.pixelformat == V4L2_PIX_FMT_XRGB555X
	) {
		CONVERT_PIXEL(XRGB555X,input_buffer,output_buffer,src_format)
	}
	else if( src_format.pixelformat == V4L2_PIX_FMT_RGB565X
	) {
		CONVERT_PIXEL(RGB565X,input_buffer,output_buffer,src_format)
	}
	else if( src_format.pixelformat == V4L2_PIX_FMT_BGR24 ) {
		CONVERT_PIXEL(BGR24,input_buffer,output_buffer,src_format)
	}
	else if( src_format.pixelformat == V4L2_PIX_FMT_RGB24 ) {
		CONVERT_PIXEL(RGB24,input_buffer,output_buffer,src_format)
	}
	else if( src_format.pixelformat == V4L2_PIX_FMT_BGR666 ) {
		CONVERT_PIXEL(BGR666,input_buffer,output_buffer,src_format)
	}
	else if(
			src_format.pixelformat == V4L2_PIX_FMT_ABGR32
			|| src_format.pixelformat == V4L2_PIX_FMT_XBGR32
	) {
		CONVERT_PIXEL(XBGR32,input_buffer,output_buffer,src_format)
	}
	else if(
			src_format.pixelformat == V4L2_PIX_FMT_ARGB32
			|| src_format.pixelformat == V4L2_PIX_FMT_XRGB32
	) {
		CONVERT_PIXEL(XRGB32,input_buffer,output_buffer,src_format)
	}
	else if(
			/* 16 YUV 4:2:2*/
			src_format.pixelformat == V4L2_PIX_FMT_YUYV
	) {
		// 4 bytes is 2 pixels (side by side):
		// with same U,V
		// but Y0/Y1 for left/right pixel
		//
		// | Y0 | U | Y1 | U |
		//
		// =>
		//
		// |Pixel 0|Pixel 1|
		// |Y0,U,V |Y1,U,V |
		for( uint y_pos=0; y_pos<src_format.height; y_pos++ ) {
		for( uint x_pos=0; x_pos<src_format.width/2; x_pos++ ) {
			const byte_t* read_pos = &input_buffer[
				y_pos*src_format.bytesperline
				+ x_pos*4
			];
			const uint write_pos = (
				(y_pos*src_format.width + x_pos*2) * 3
				);
			// transform:
			float y0 = read_pos[0];
			float u = read_pos[1];
			float y1 = read_pos[2];
			float v = read_pos[3];
			u -= 128;
			v -= 128;
			/*
			y0 -= 16;
			y1 -= 16;
			*/
			// left pixel:
			const float left_r = 1*y0 + 0*u + 1.402*v;
			const float left_g = 1*y0 - 0.344136*u - 0.714136*v;
			const float left_b = 1*y0 + 1.772*u + 0*v;
			// right pixel:
			const float right_r = 1*y1 + 0*u + 1.402*v;
			const float right_g = 1*y1 - 0.344136*u - 0.714136*v;
			const float right_b = 1*y1 + 1.772*u + 0*v;
			// write:
			output_buffer[write_pos+0] = clamp( left_r, 0,255.0 );
			output_buffer[write_pos+1] = clamp( left_g, 0,255.0 );
			output_buffer[write_pos+2] = clamp( left_b, 0,255.0 );
			output_buffer[write_pos+3] = clamp( right_r, 0,255.0 );
			output_buffer[write_pos+4] = clamp( right_g, 0,255.0 );
			output_buffer[write_pos+5] = clamp( right_b, 0,255.0 );
		}
		}
	}
	return RET_SUCCESS;
}

size_t image_rgb_size(
		const uint width,
		const uint height
)
{
	return width * height * 3;
}

/************************
 * private utils impl
*************************/

uint format_pixel_size(img_format_t format) {
	if( format.pixelformat == V4L2_PIX_FMT_RGB332 )
		return GET_SIZE( RGB332 );
	else if( format.pixelformat == V4L2_PIX_FMT_ARGB444 || format.pixelformat == V4L2_PIX_FMT_XRGB444 )
		return GET_SIZE( XRGB444 );
	else if( format.pixelformat == V4L2_PIX_FMT_ARGB555 || format.pixelformat == V4L2_PIX_FMT_XRGB555 )
		return GET_SIZE( XRGB555 );
	else if( format.pixelformat == V4L2_PIX_FMT_RGB565 )
		return GET_SIZE( RGB565 );
	else if( format.pixelformat == V4L2_PIX_FMT_ARGB555X || format.pixelformat == V4L2_PIX_FMT_XRGB555X )
		return GET_SIZE( XRGB555X );
	else if( format.pixelformat == V4L2_PIX_FMT_RGB565X )
		return GET_SIZE( RGB565X );
	else if( format.pixelformat == V4L2_PIX_FMT_BGR24 )
		return GET_SIZE( BGR24 );
	else if( format.pixelformat == V4L2_PIX_FMT_RGB24 )
		return GET_SIZE( RGB24 );
	else if( format.pixelformat == V4L2_PIX_FMT_BGR666 )
		return GET_SIZE( BGR666 );
	else if( format.pixelformat == V4L2_PIX_FMT_ABGR32 || format.pixelformat == V4L2_PIX_FMT_XBGR32 )
		return GET_SIZE( XBGR32 );
	else if( format.pixelformat == V4L2_PIX_FMT_ARGB32 || format.pixelformat == V4L2_PIX_FMT_XRGB32 )
		return GET_SIZE( XRGB32 );
	else if( format.pixelformat == V4L2_PIX_FMT_YUYV )
		return GET_SIZE( YUYV );
	return 0;
}

ret_t check_format(
		const img_format_t format
) {
	if( !(
			format.pixelformat == V4L2_PIX_FMT_RGB332
			|| format.pixelformat == V4L2_PIX_FMT_ARGB444 || format.pixelformat == V4L2_PIX_FMT_XRGB444
			|| format.pixelformat == V4L2_PIX_FMT_ARGB555 || format.pixelformat == V4L2_PIX_FMT_XRGB555
			|| format.pixelformat == V4L2_PIX_FMT_RGB565
			|| format.pixelformat == V4L2_PIX_FMT_ARGB555X || format.pixelformat == V4L2_PIX_FMT_XRGB555X
			|| format.pixelformat == V4L2_PIX_FMT_RGB565X
			|| format.pixelformat == V4L2_PIX_FMT_BGR24
			|| format.pixelformat == V4L2_PIX_FMT_RGB24
			|| format.pixelformat == V4L2_PIX_FMT_BGR666
			|| format.pixelformat == V4L2_PIX_FMT_ABGR32 || format.pixelformat == V4L2_PIX_FMT_XBGR32
			|| format.pixelformat == V4L2_PIX_FMT_ARGB32 || format.pixelformat == V4L2_PIX_FMT_XRGB32
			|| format.pixelformat == V4L2_PIX_FMT_YUYV
	) ) {
		log_error( "format not supported\n" );
		return RET_FAILURE;
	}
	if(
			!( format.bytesperline * format.height <= format.sizeimage )
	) {
		log_error( "condition not fulfilled: (format.bytesperline * format.height <= format.sizeimage)\n" );
		return RET_FAILURE;
	}
	if(
			!( format.width*format_pixel_size(format) <= format.bytesperline )
	) {
		log_error( "condition not fulfilled: (format.width*format_pixel_size(format) <= format.bytesperline)\n" );
		return RET_FAILURE;
	}
	/*
	if( format.xfer_func != V4L2_XFER_FUNC_DEFAULT ) {
		log_error( "xfer_func: non-standard color transfer functions not supported\n" );
		return RET_FAILURE;
	}
	if( format.ycbcr_enc != V4L2_YCBCR_ENC_DEFAULT ) {
		log_error( "ycbcr_enc: non-standard color encodings not supported\n" );
		return RET_FAILURE;
	}
	if( format.quantization != V4L2_QUANTIZATION_DEFAULT ) {
		log_error( "quantization: non-standard color quantizations not supported\n" );
		return RET_FAILURE;
	}
	*/
	return RET_SUCCESS;
}

