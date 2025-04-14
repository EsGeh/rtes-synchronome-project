#include "image.h"
#include "global.h"

#include <check.h>


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

START_TEST(test_image_save_ppm) {
	const char* filename = "/tmp/test.ppm";
	const byte_t RED = 7 << 5; // 0b11100000
	const byte_t GREEN = 7 << 2; // 0b00011100
	const byte_t BLUE = 3; // 0b00000011
	const img_format_t format = {
		.width = 3,
		.height = 2,
		.pixelformat = V4L2_PIX_FMT_RGB332,
	};
	const unsigned char buffer[] = {
		RED, GREEN, BLUE,
		RED|GREEN, RED|GREEN|BLUE, 0
	};
	image_save_ppm(
			"hello world",
			buffer,
			6,
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
		tcase_add_test(test_case, test_image_save_ppm);
		suite_add_tcase(suite, test_case);
	}
	return suite;
}
