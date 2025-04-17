#include "lib/image.h"
#include "lib/global.h"

#include <check.h>
#include <linux/videodev2.h>
#include <string.h>

#define CHECK_IMAGE_SUCCESS( CALL ) \
	if( CALL != RET_SUCCESS ) { \
		ck_abort_msg( "'" #CALL "' failed" ); \
	}

#define CHECK_IMAGE_FAILURE( CALL ) { \
	ret_t ret = CALL; \
	ck_assert_int_eq( ret, RET_FAILURE ); \
}

/***********************
 * test case
***********************/

const byte_t expected[] =
	"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
	"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
;

typedef struct {
	img_format_t format;
	byte_t src_buffer[256];
	size_t src_size;
} test_args_t;

test_args_t test_args_RGB332() {
	const byte_t RED = 7 << 5; // 0b11100000
	const byte_t GREEN = 7 << 2; // 0b00011100
	const byte_t BLUE = 3; // 0b00000011
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_RGB332,
			.sizeimage = 3*2,
			.bytesperline = 3
		},
			.src_buffer = {
				RED, GREEN, BLUE,
				RED|GREEN, RED|GREEN|BLUE, 0
			},
	};
}

test_args_t test_args_XRGB444() {
	const uint format_size = 2;
	const byte_t HIGH = 0xF0;
	const byte_t LOW = 0x0F;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_XRGB444,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
			.src_buffer = {
				0, LOW, /**/ HIGH, 0, /**/ LOW, 0,
				HIGH, LOW, /**/ HIGH|LOW, LOW, /**/ 0, 0
			},
	};
}

test_args_t test_args_XRGB555() {
	const uint format_size = 2;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_XRGB555,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
// | byte0 | byte1 |
// | gggbbbbb  | _rrrrrgg   |
			.src_buffer = {
				0, 0b01111100, /**/ 0b11100000, 0b00000011, /**/ 0b00011111, 0,
				0b11100000, 0b01111111, /**/ 0xFF, 0b01111111, /**/ 0, 0
			},
	};
}

test_args_t test_args_RGB565() {
	const uint format_size = 2;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_RGB565,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
// | byte0 | byte1 |
// | gggbbbbb | rrrrrggg |
			.src_buffer = {
				0, 0b11111000, /**/ 0b11100000, 0b00000111, /**/ 0b00011111, 0,
				0b11100000, 0b11111111, /**/ 0xFF, 0xFF, /**/ 0, 0
			},
	};
}

test_args_t test_args_XRGB555X() {
	const uint format_size = 2;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_XRGB555X,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
// | byte0 | byte1 |
// | xrrrrrgg | gggbbbbb |
			.src_buffer = {
				0b011111100, 0, /**/ 0b00000011, 0b11100000, /**/ 0, 0b00011111,
				0b01111111, 0b11100000, /**/ 0b01111111, 0xFF, /**/ 0, 0
			},
	};
}

test_args_t test_args_RGB565X() {
	const uint format_size = 2;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_RGB565X,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
// | byte0 | byte1 |
// | rrrrrggg | gggbbbbb |
			.src_buffer = {
				0b11111000, 0, /**/ 0b00000111, 0b11100000, /**/ 0, 0b00011111,
				0b11111111, 0b11100000, /**/ 0xFF, 0xFF, /**/ 0, 0
			},
	};
}

test_args_t test_args_BGR24() {
	const uint format_size = 3;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_BGR24,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
			.src_buffer = {
				0, 0, 0xFF, /**/ 0, 0xFF, 0, /**/ 0xFF, 0, 0, 
				0, 0xFF, 0xFF, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
			},
	};
}

test_args_t test_args_RGB24() {
	const uint format_size = 3;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_RGB24,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
			.src_buffer = {
				0xFF, 0, 0, /**/ 0, 0xFF, 0, /**/ 0, 0, 0xFF, 
				0xFF, 0xFF, 0, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
			},
	};
}

test_args_t test_args_BGR666() {
	const uint format_size = 3;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_BGR666,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
	// |byte 0|byte 1|byte 2|
	// |bbbbbbgg|ggggrrrr|rr______|
		.src_buffer = {
			0, 0b00001111, 0b11000000, /**/ 0b00000011, 0b11110000, 0, /**/ 0b11111100, 0, 0, 
			0b00000011, 0xFF, 0b11000000, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
		},
	};
}

test_args_t test_args_XBGR32() {
	const uint format_size = 4;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_XBGR32,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
	// |byte 0|byte 1|byte 2|byte3|
	// |b|g|r|x|
		.src_buffer = {
			0,0,0xFF,0, /**/ 0,0xFF,0,0, /**/ 0xFF,0,0,0,
			0,0xFF,0xFF,0, /**/ 0xFF, 0xFF, 0xFF, 0, /**/ 0, 0, 0, 0
		},
	};
}

test_args_t test_args_XRGB32() {
	const uint format_size = 4;
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_XRGB32,
			.sizeimage = 3*2*format_size,
			.bytesperline = 3*format_size,
		},
	// |byte 0|byte 1|byte 2|byte3|
	// |x|r|g|b|
		.src_buffer = {
			0,0xFF,0,0, /**/ 0,0,0xFF,0, /**/ 0,0,0,0xFF,
			0,0xFF,0xFF,0, /**/ 0,0xFF,0xFF,0xFF, /**/ 0, 0, 0, 0
		},
	};
}

const byte_t expected_YUYV[] = {
	0xFF,0,0, 0xFF,0,0, 0,0xFF,0, 0,0xFF,0,
	0,0,0xFF, 0,0,0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF
	// 0,0,0, 0,0,0, 0,0,0, 0,0,0
};

test_args_t test_args_YUYV() {
	return (test_args_t){
		.format = {
			.width = 4, .height = 2,
			.pixelformat = V4L2_PIX_FMT_YUYV,
			.sizeimage = 16,
			.bytesperline = 8 // 
		},
		.src_buffer = {
			76, 84, 76, 255, /**/ 149, 43, 149, 21,
			29, 255, 29, 107, /**/ 255, 128, 255, 128
		},
	};
}

test_args_t test_args_RGB332_padding() {
	const byte_t RED = 7 << 5; // 0b11100000
	const byte_t GREEN = 7 << 2; // 0b00011100
	const byte_t BLUE = 3; // 0b00000011
	return (test_args_t){
		.format = {
			.width = 3, .height = 2,
			.pixelformat = V4L2_PIX_FMT_RGB332,
			.sizeimage = 4*2,
			.bytesperline = 4,
		},
			.src_buffer = {
				RED, GREEN, BLUE, 0,
				RED|GREEN, RED|GREEN|BLUE, 0, 0
			},
	};
}

START_TEST(test_image_convert_RGB332) {
	const test_args_t args = test_args_RGB332();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_XRGB444) {
	const test_args_t args = test_args_XRGB444();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_XRGB555) {
	const test_args_t args = test_args_XRGB555();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_RGB565) {
	const test_args_t args = test_args_RGB565();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_XRGB555X) {
	const test_args_t args = test_args_XRGB555X();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_RGB565X) {
	const test_args_t args = test_args_RGB565X();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_BGR24) {
	const test_args_t args = test_args_BGR24();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_RGB24) {
	const test_args_t args = test_args_RGB24();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_BGR666) {
	const test_args_t args = test_args_BGR666();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_XBGR32) {
	const test_args_t args = test_args_XBGR32();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_XRGB32) {
	const test_args_t args = test_args_XRGB32();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_YUYV) {
	const test_args_t args = test_args_YUYV();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	for( uint i=0; i<sizeof(dst_buffer); ++i ) {
		if( abs((int )expected_YUYV[i] - (int )dst_buffer[i]) > 1 ) {
			char str_buf[STR_BUFFER_SIZE] = "";
			for( uint j=0; j<sizeof(dst_buffer); ++j ) {
				sprintf( &str_buf[strlen(str_buf)], "%x", dst_buffer[j] );
			}
			sprintf( &str_buf[strlen(str_buf)], " = " );
			for( uint j=0; j<sizeof(expected_YUYV); ++j ) {
				sprintf( &str_buf[strlen(str_buf)], "%x", expected_YUYV[j] );
			}
			ck_abort_msg(
				"failed '%s' - differs at pos %d: %x != %x",
				str_buf,
				i,
				dst_buffer[i],
				expected_YUYV[i]
		);
		}
	}
}
END_TEST


START_TEST(test_image_convert_1byte_padding) {
	const test_args_t args = test_args_RGB332_padding();
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_1byte_oversize) {
	test_args_t args = test_args_RGB332();
		args.format.sizeimage += 1;
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_SUCCESS );
	}
	ck_assert_mem_eq( dst_buffer, expected, sizeof(dst_buffer) );
}
END_TEST

START_TEST(test_image_convert_1byte_undersized) {
	test_args_t args = test_args_RGB332();
		args.format.sizeimage -= 1;
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_FAILURE );
	}
}
END_TEST

START_TEST(test_image_convert_1byte_padding_undersized) {
	test_args_t args = test_args_RGB332_padding();
		args.format.sizeimage -= 1;
	byte_t dst_buffer[args.format.width * args.format.height * 3];
	memset(dst_buffer, 0, sizeof(dst_buffer));
	{
		ret_t ret = image_convert_to_rgb(
				args.format,
				args.src_buffer,
				dst_buffer, sizeof(dst_buffer)
		);
		ck_assert( ret == RET_FAILURE );
	}
}
END_TEST

/***********************
 * test suite
***********************/

Suite* image_suite() {
	Suite* suite = suite_create("image");
	{
		TCase* test_case = tcase_create("convert");
		tcase_add_test(test_case, test_image_convert_RGB332);
		tcase_add_test(test_case, test_image_convert_XRGB444);
		tcase_add_test(test_case, test_image_convert_XRGB555);
		tcase_add_test(test_case, test_image_convert_RGB565);
		tcase_add_test(test_case, test_image_convert_XRGB555X);
		tcase_add_test(test_case, test_image_convert_RGB565X);
		tcase_add_test(test_case, test_image_convert_BGR24);
		tcase_add_test(test_case, test_image_convert_RGB24);
		tcase_add_test(test_case, test_image_convert_BGR666);
		tcase_add_test(test_case, test_image_convert_XBGR32);
		tcase_add_test(test_case, test_image_convert_XRGB32);

		tcase_add_test(test_case, test_image_convert_YUYV);

		tcase_add_test(test_case, test_image_convert_1byte_padding);
		tcase_add_test(test_case, test_image_convert_1byte_oversize);
		tcase_add_test(test_case, test_image_convert_1byte_undersized);
		tcase_add_test(test_case, test_image_convert_1byte_padding_undersized);

		suite_add_tcase(suite, test_case);
	}
	/*
	{
		TCase* test_case = tcase_create("save");
		tcase_add_test(test_case, test_image_convert_1byte_save);
	}
	*/
	return suite;
}
