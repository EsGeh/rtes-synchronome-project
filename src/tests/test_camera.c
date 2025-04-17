#include "lib/camera.h"
#include "lib/global.h"

#include <check.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>


#define CHECK_CAMERA_SUCCESS( CALL ) \
	if( CALL != RET_SUCCESS ) { \
		ck_abort_msg( "'" #CALL "' failed with: %s", camera_error() ); \
	}

#define CHECK_CAMERA_FAILURE( CALL ) { \
	ret_t ret = CALL; \
	ck_assert_int_eq( ret, RET_FAILURE ); \
	ck_assert_str_ne( camera_error(), "" ); \
}

/***********************
 * test case: init/exit
***********************/

const unsigned int buffer_count = 10;

// camera: uninitialized -> initialized
START_TEST(test_camera_init) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	{
		CHECK_CAMERA_SUCCESS( camera_init(
					&camera,
					dev_name
		) );
		ck_assert_str_eq( camera.dev_name, dev_name );
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_int_gt( camera.format.width, 0 );
		ck_assert_int_gt( camera.format.height, 0 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
	{
		CHECK_CAMERA_SUCCESS( camera_exit(&camera) );
		ck_assert_int_eq( camera.dev_file, -1 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
}
END_TEST

// camera: initialized -> ready
START_TEST(test_camera_init_buffer) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	const unsigned int buffer_count= 10;
	camera_init(
			&camera,
			dev_name
	);
	{
		ck_assert_ptr_null( camera.buffer_container.buffers );
		CHECK_CAMERA_SUCCESS( camera_init_buffer(
					&camera,
					buffer_count
		));
		ck_assert_ptr_nonnull( camera.buffer_container.buffers );
		ck_assert_int_ge( camera.buffer_container.count, buffer_count );
		for( unsigned int i=0; i<camera.buffer_container.count; i++ ) {
			ck_assert_ptr_ne( camera.buffer_container.buffers[i].data, NULL );
			ck_assert_int_ge( camera.buffer_container.buffers[i].size, 0 );
		}
	}
	camera_exit(&camera);
}

// negative tests:

START_TEST(test_camera_init_buffer_uninitialized) {
	const char dev_name[] = "/dev/video0";
	camera_t camera;
	camera_zero( &camera );
	camera_init(
			&camera,
			dev_name
	);
	{
		camera_exit(
				&camera
		);
		const unsigned int buffer_count= 10;
		CHECK_CAMERA_FAILURE( camera_init_buffer(
				&camera,
				buffer_count
		) );
	}
}
END_TEST

START_TEST(test_camera_exit_uninitialized) {
	const char dev_name[] = "/dev/video0";
	camera_t camera;
	camera_zero( &camera );
	camera_init(
			&camera,
			dev_name
	);
	{
		camera_exit(
				&camera
		);
		CHECK_CAMERA_FAILURE( camera_exit(
				&camera
		) );
	}
}
END_TEST

START_TEST(test_camera_init_non_existent_device) {
	camera_t camera;
	camera_zero( &camera );
	ret_t ret = camera_init(
			&camera,
			"/dev/nonexistent"
	);
	ck_assert_int_eq( ret, RET_FAILURE );
	ck_assert_str_ne( camera_error(), "" );
	// 
	ck_assert_int_eq( camera.dev_file, -1 );
	ck_assert_int_eq( camera.buffer_container.count, 0 );
	ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
}
END_TEST

START_TEST(test_camera_init_non_char_device) {
	camera_t camera;
	camera_zero( &camera );
	ret_t ret = camera_init(
			&camera,
			"/proc/cpuinfo"
	);
	ck_assert_int_eq( ret, RET_FAILURE );
	ck_assert_str_ne( camera_error(), "" );
	// 
	ck_assert_int_eq( camera.dev_file, -1 );
	ck_assert_int_eq( camera.buffer_container.count, 0 );
	ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
}
END_TEST

START_TEST(test_camera_init_wrong_device) {
	camera_t camera;
	camera_zero( &camera );
	ret_t ret = camera_init(
			&camera,
			"/dev/null"
	);
	ck_assert_int_eq( ret, RET_FAILURE );
	ck_assert_str_ne( camera_error(), "" );
	// 
	ck_assert_int_eq( camera.dev_file, -1 );
	ck_assert_int_eq( camera.buffer_container.count, 0 );
	ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
}
END_TEST

/***********************
 * test case: formats
***********************/

START_TEST(test_camera_list_formats) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		format_descriptions_t format_descriptions;
		CHECK_CAMERA_SUCCESS( camera_list_formats(
					&camera,
					&format_descriptions
		));
		ck_assert_int_gt( format_descriptions.count, 0 );
		// camera still initialized:
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
	camera_exit(&camera);
}

START_TEST(test_camera_set_format_default) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		CHECK_CAMERA_SUCCESS( camera_set_format(
					&camera,
					0, 0,
					NULL, false
		));
		// camera still initialized:
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
	camera_exit(&camera);
}

START_TEST(test_camera_set_format) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	format_descriptions_t format_descriptions;
	camera_list_formats(
				&camera,
				&format_descriptions
	);
	ck_assert_int_gt( format_descriptions.count, 0 );
	for( unsigned int i=0; i< format_descriptions.count; i++ ) {
		const struct v4l2_fmtdesc* format_descr = &format_descriptions.format_descrs[i];
		CHECK_CAMERA_SUCCESS( camera_set_format(
					&camera,
					0, 0,
					&(format_descr->pixelformat), true
		));
		ck_assert_int_eq( camera.format.pixelformat, format_descr->pixelformat );
		// camera still initialized:
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
	camera_exit(&camera);
}

// negative tests:

START_TEST(test_camera_list_formats_uninitialized) {
	camera_t camera;
	camera_zero( &camera );
	format_descriptions_t format_descriptions;
	CHECK_CAMERA_FAILURE( camera_list_formats(
			&camera,
			&format_descriptions
	) );
}
END_TEST

START_TEST(test_camera_set_format_uninitialized) {
	camera_t camera;
	camera_zero( &camera );
	{
		CHECK_CAMERA_FAILURE( camera_set_format(
				&camera,
				0, 0,
				NULL, false
		) );
	}
}
END_TEST

START_TEST(test_camera_set_format_too_large) {
	const char dev_name[] = "/dev/video0";
	camera_t camera;
	camera_zero( &camera );
	camera_init(
			&camera,
			dev_name
	);
	{
		CHECK_CAMERA_FAILURE( camera_set_format(
				&camera,
				100000, 100000,
				NULL, false
		) );
		// camera still initialized:
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_ptr_null( camera.buffer_container.buffers );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
	}
}
END_TEST

/***********************
 * test case: streaming
***********************/

// camera: ready -> capturing
START_TEST(test_camera_streaming) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	camera_init_buffer(
			&camera,
			10
	);
	{
		CHECK_CAMERA_SUCCESS( camera_stream_start(
					&camera
		));
		CHECK_CAMERA_SUCCESS( camera_stream_stop(
					&camera
		));
	}
	camera_exit(&camera);
}

START_TEST(test_camera_get_frame) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	camera_init_buffer(
			&camera,
			10
	);
	camera_stream_start( &camera );
	frame_buffer_t frame;
	{
		CHECK_CAMERA_SUCCESS( camera_get_frame( &camera, &frame ) );
		ck_assert_ptr_nonnull( frame.data );
		ck_assert_int_gt( frame.size, 0 );
	}
	{
		CHECK_CAMERA_SUCCESS( camera_return_frame( &camera, &frame ) );
		ck_assert_ptr_null( frame.data );
	}
	camera_stream_stop( &camera );
	camera_exit(&camera);
}
END_TEST

// negative tests:

START_TEST(test_camera_stream_start_uninitialized) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	camera_exit(
			&camera
	);
	{
		CHECK_CAMERA_FAILURE( camera_stream_start(
					&camera
		));
	}
}

START_TEST(test_camera_stream_start_not_ready) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		CHECK_CAMERA_FAILURE( camera_stream_start(
					&camera
		));
	}
}

START_TEST(test_camera_stream_stop_uninitialized) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		CHECK_CAMERA_FAILURE( camera_stream_stop(
					&camera
		));
	}
}

START_TEST(test_camera_stream_stop_not_ready) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	camera_exit(
			&camera
	);
	{
		CHECK_CAMERA_FAILURE( camera_stream_stop(
					&camera
		));
	}
}

START_TEST(test_camera_get_frame_uninitialized) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	camera_exit(
			&camera
	);
	{
		frame_buffer_t frame_buffer;
		CHECK_CAMERA_FAILURE( camera_get_frame(
					&camera,
					&frame_buffer
		));
	}
}

START_TEST(test_camera_get_frame_not_ready) {
	camera_t camera;
	camera_zero( &camera );
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		frame_buffer_t frame_buffer;
		CHECK_CAMERA_FAILURE( camera_get_frame(
					&camera,
					&frame_buffer
		));
	}
}

/***********************
 * test suite
***********************/

Suite* camera_suite() {
	Suite* suite = suite_create("camera");
	{
		TCase* test_case = tcase_create("init/exit");
		tcase_add_test(test_case, test_camera_init);
		tcase_add_test(test_case, test_camera_init_buffer);
		// negative:
		tcase_add_test(test_case, test_camera_init_buffer_uninitialized);
		tcase_add_test(test_case, test_camera_exit_uninitialized);
		tcase_add_test(test_case, test_camera_init_non_existent_device);
		tcase_add_test(test_case, test_camera_init_non_char_device);
		tcase_add_test(test_case, test_camera_init_wrong_device);
		suite_add_tcase(suite, test_case);
	}
	{
		TCase* test_case = tcase_create("formats");
		tcase_add_test(test_case, test_camera_list_formats);
		tcase_add_test(test_case, test_camera_set_format_default);
		tcase_add_test(test_case, test_camera_set_format);
		// negative:
		tcase_add_test(test_case, test_camera_list_formats_uninitialized);
		tcase_add_test(test_case, test_camera_set_format_uninitialized);
		tcase_add_test(test_case, test_camera_set_format_too_large);
		suite_add_tcase(suite, test_case);
	}
	{
		TCase* test_case = tcase_create("streaming");
		tcase_add_test(test_case, test_camera_streaming);
		tcase_add_test(test_case, test_camera_get_frame);
		// negative:
		tcase_add_test(test_case, test_camera_stream_start_uninitialized);
		tcase_add_test(test_case, test_camera_stream_start_not_ready);
		tcase_add_test(test_case, test_camera_stream_stop_uninitialized);
		tcase_add_test(test_case, test_camera_stream_stop_not_ready);
		tcase_add_test(test_case, test_camera_get_frame_uninitialized);
		tcase_add_test(test_case, test_camera_get_frame_not_ready);
		suite_add_tcase(suite, test_case);
	}
	return suite;
}
