#include "image.h"
#include "global.h"

#include <check.h>
#include <linux/videodev2.h>


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

START_TEST(test_image_save_ppm_RGB332) {
	const char* filename = "/tmp/test.ppm";
	const uint format_size = 1;
	const byte_t RED = 7 << 5; // 0b11100000
	const byte_t GREEN = 7 << 2; // 0b00011100
	const byte_t BLUE = 3; // 0b00000011
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_RGB332,
	};
	const byte_t buffer[] = {
		RED, GREEN, BLUE,
		RED|GREEN, RED|GREEN|BLUE, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_XRGB444) {
	const char* filename = "/tmp/test_xrgb444.ppm";
	const uint format_size = 2;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_XRGB444,
	};
	const byte_t HIGH = 0xF0;
	const byte_t LOW = 0x0F;
	// | byte0 | byte1 |
	// | g, b  | _ r   |
	const byte_t buffer[] = {
		0, LOW, /**/ HIGH, 0, /**/ LOW, 0,
		HIGH, LOW, /**/ HIGH|LOW, LOW, /**/ 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	ck_assert_int_lt( file_size, 100 );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_XRGB555) {
	const char* filename = "/tmp/test_xrgb555.ppm";
	const uint format_size = 2;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_XRGB555,
	};
	// | byte0 | byte1 |
	// | gggbbbbb  | _rrrrrgg   |
	const byte_t buffer[] = {
		0, 0b01111100, /**/ 0b11100000, 0b00000011, /**/ 0b00011111, 0,
		0b11100000, 0b01111111, /**/ 0xFF, 0b01111111, /**/ 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	ck_assert_int_lt( file_size, 100 );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_RGB565) {
	const char* filename = "/tmp/test_rgb565.ppm";
	const uint format_size = 2;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_RGB565,
	};
	// | byte0 | byte1 |
	// | gggbbbbb | rrrrrggg |
	const byte_t buffer[] = {
		0, 0b11111000, /**/ 0b11100000, 0b00000111, /**/ 0b00011111, 0,
		0b11100000, 0b11111111, /**/ 0xFF, 0xFF, /**/ 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	ck_assert_int_lt( file_size, 100 );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_XRGB555X) {
	const char* filename = "/tmp/test_XRGB555X.ppm";
	const uint format_size = 2;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_XRGB555X,
	};
	// | byte0 | byte1 |
	// | _rrrrrgg | gggbbbbb |
	const byte_t buffer[] = {
		0b01111100, 0, /**/ 0b000000011, 0b11100000, /**/ 0, 0b00011111,
		0b01111111, 0b11100000, /**/ 0xFF, 0xFF, /**/ 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	ck_assert_int_lt( file_size, 100 );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_RGB565X) {
	const char* filename = "/tmp/test_RGB565X.ppm";
	const uint format_size = 2;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_RGB565X,
	};
	// | byte0 | byte1 |
	// | rrrrrggg | gggbbbbb |
	const byte_t buffer[] = {
		0b11111000, 0, /**/ 0b000000111, 0b11100000, /**/ 0, 0b00011111,
		0b11111111, 0b11100000, /**/ 0xFF, 0xFF, /**/ 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	ck_assert_int_lt( file_size, 100 );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_BGR24) {
	const char* filename = "/tmp/test_BGR24.ppm";
	const uint format_size = 3;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_BGR24,
	};
	const byte_t buffer[] = {
		0, 0, 0xFF, /**/ 0, 0xFF, 0, /**/ 0xFF, 0, 0, 
		0, 0xFF, 0xFF, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_RGB24) {
	const char* filename = "/tmp/test_rgb24.ppm";
	const uint format_size = 3;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_RGB24,
	};
	const byte_t buffer[] = {
		0xFF, 0, 0, /**/ 0, 0xFF, 0, /**/ 0, 0, 0xFF, 
		0xFF, 0xFF, 0, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_BGR666) {
	const char* filename = "/tmp/test_BGR666.ppm";
	const uint format_size = 3;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_BGR666,
	};
	// |byte 0|byte 1|byte 2|
	// |bbbbbbgg|ggggrrrr|rr______|
	const byte_t buffer[] = {
		0, 0b00001111, 0b11000000, /**/ 0b00000011, 0b11110000, 0, /**/ 0b11111100, 0, 0, 
		0b00000011, 0xFF, 0b11000000, /**/ 0xFF, 0xFF, 0xFF, /**/ 0, 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_XBGR32) {
	const char* filename = "/tmp/test_XBGR32.ppm";
	const uint format_size = 4;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_XBGR32,
	};
	// |byte 0|byte 1|byte 2|byte3|
	// |b|g|r|x|
	const byte_t buffer[] = {
		0,0,0xFF,0, /**/ 0,0xFF,0,0, /**/ 0xFF,0,0,0,
		0,0xFF,0xFF,0, /**/ 0xFF, 0xFF, 0xFF, 0, /**/ 0, 0, 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

START_TEST(test_image_save_ppm_XRGB32) {
	const char* filename = "/tmp/test_XRGB32.ppm";
	const uint format_size = 4;
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_XRGB32,
	};
	// |byte 0|byte 1|byte 2|byte3|
	// |x|r|g|b|
	const byte_t buffer[] = {
		0,0xFF,0,0, /**/ 0,0,0xFF,0, /**/ 0,0,0,0xFF,
		0,0xFF,0xFF,0, /**/ 0,0xFF,0xFF,0xFF, /**/ 0, 0, 0, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6*format_size,
			format,
			filename
	);
	FILE* fp = fopen( filename, "r" );
	ck_assert_msg( fp != NULL, "file does not exist" );
	fseek( fp, 0, SEEK_END );
	int file_size = ftell( fp );
	ck_assert_msg( file_size > 0, "file has size 0" );
	fseek( fp, 0, SEEK_SET );
	byte_t expected[] =
		"P6\n"
		"#hello world\n"
		"3 2 255\n"
		"\xff\x0\x0" "\x0\xff\x0" "\x0\x0\xff"
		"\xff\xff\x0" "\xff\xff\xff" "\x0\x0\x0"
	;
	byte_t read_buffer[file_size];
	fread( read_buffer, 1, file_size, fp );
	ck_assert_msg( !memcmp( read_buffer, expected, file_size ),
			"file content not matching"
	);
	fclose( fp );
}
END_TEST

/***********************
 * test suite
***********************/

Suite* image_suite() {
	Suite* suite = suite_create("image");
	{
		TCase* test_case = tcase_create("save");
		tcase_add_test(test_case, test_image_save_ppm_RGB332);
		tcase_add_test(test_case, test_image_save_ppm_XRGB444);
		tcase_add_test(test_case, test_image_save_ppm_XRGB555);
		tcase_add_test(test_case, test_image_save_ppm_RGB565);
		tcase_add_test(test_case, test_image_save_ppm_XRGB555X);
		tcase_add_test(test_case, test_image_save_ppm_RGB565X);
		tcase_add_test(test_case, test_image_save_ppm_BGR24);
		tcase_add_test(test_case, test_image_save_ppm_RGB24);
		tcase_add_test(test_case, test_image_save_ppm_BGR666);
		tcase_add_test(test_case, test_image_save_ppm_XBGR32);
		tcase_add_test(test_case, test_image_save_ppm_XRGB32);
		suite_add_tcase(suite, test_case);
	}
	return suite;
}
