#include "camera.h"
#include "global.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


/***********************
 * positve tests:
***********************/

START_TEST(test_camera_init) {
	camera_t camera;
	const char dev_name[] = "/dev/video0";
	{
		ret_t ret = camera_init(&camera, dev_name );
		ck_assert_int_eq( ret, RET_SUCCESS );
		ck_assert_str_eq( camera.dev_name, dev_name );
		ck_assert_int_ge( camera.dev_file, 0 );
		ck_assert_int_gt( camera.buffer_container.count, 0 );
		ck_assert_ptr_ne( camera.buffer_container.buffers, NULL );
		for( unsigned int i=0; i<camera.buffer_container.count; i++ ) {
			ck_assert_ptr_ne( camera.buffer_container.buffers[i].data, NULL );
			ck_assert_int_ge( camera.buffer_container.buffers[i].size, 0 );
		}
	}
	{
		ret_t ret = camera_exit(&camera);
		ck_assert_int_eq( ret, RET_SUCCESS );
		ck_assert_int_eq( camera.dev_file, -1 );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
		ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
	}
}
END_TEST

/***********************
 * negative tests:
***********************/

START_TEST(test_camera_init_null_fails) {
	{
		ret_t ret = camera_init(NULL, "/dev/video0");
		ck_assert_int_eq( ret, RET_FAILURE );
		ck_assert_str_ne( camera_error(), "" );
	}
}
END_TEST

START_TEST(test_camera_exit_null_fails) {
	{
		ret_t ret = camera_exit(NULL);
		ck_assert_int_eq( ret, RET_FAILURE );
		ck_assert_str_ne( camera_error(), "" );
	}
}
END_TEST

START_TEST(test_camera_init_non_existent_device) {
	camera_t camera;
	{
		ret_t ret = camera_init(&camera, "/dev/nonexistent" );
		ck_assert_int_eq( ret, RET_FAILURE );
		ck_assert_str_ne( camera_error(), "" );
		// 
		ck_assert_int_eq( camera.dev_file, -1 );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
		ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
	}
}
END_TEST

START_TEST(test_camera_init_wrong_device) {
	camera_t camera;
	{
		ret_t ret = camera_init(&camera, "/dev/null" );
		ck_assert_int_eq( ret, RET_FAILURE );
		ck_assert_str_ne( camera_error(), "" );
		// 
		ck_assert_int_eq( camera.dev_file, -1 );
		ck_assert_int_eq( camera.buffer_container.count, 0 );
		ck_assert_ptr_eq( camera.buffer_container.buffers, NULL );
	}
}
END_TEST

/***********************
 * test suite
***********************/

Suite* camera_suite() {
	Suite* suite = suite_create("camera");
	{
		TCase* test_case = tcase_create("init/exit");
		tcase_add_test(test_case, test_camera_init);
		tcase_add_test(test_case, test_camera_init_null_fails);
		tcase_add_test(test_case, test_camera_exit_null_fails);
		tcase_add_test(test_case, test_camera_init_non_existent_device);
		tcase_add_test(test_case, test_camera_init_wrong_device);
		suite_add_tcase(suite, test_case);
	}
	return suite;
}
