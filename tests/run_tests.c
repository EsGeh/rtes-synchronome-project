#include "test_camera.c"
#include "global.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


const char short_options[] = "h";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0,0,0,0 },
};


void print_cmd_line_info(
		int argc,
		char* argv[]
);
int parse_cmd_line_args(
		int argc,
		char* argv[],
		char** suite_name,
		char** case_name
);

int main(
		int argc,
		char* argv[]
) {
	// init:
	SRunner* runner = srunner_create(NULL);
	// test suites:
	{
		srunner_add_suite( runner, camera_suite() );
	}
	char* suite_name = NULL;
	char* case_name = NULL;
	{
		int ret = parse_cmd_line_args(
				argc, argv,
				&suite_name,
				&case_name
		);
		// --help:
		if( ret == -1 ) {
			print_cmd_line_info( argc, argv );
			return EXIT_SUCCESS;
		}
		// error parsing cmd line args:
		else if( ret != 0 ) {
			print_cmd_line_info( argc, argv );
			return EXIT_FAILURE;
		}
	}
	/*
	if( !suite_name ) {
		printf( "SUITE: all\n" );
	}
	else {
		printf( "SUITE: %s\n", suite_name );
	}
	if( !case_name ) {
		printf( "CASE: all\n" );
	}
	else {
		printf( "CASE: %s\n", case_name );
	}
	*/

	// run tests:
	srunner_run_tagged(runner,
			suite_name,
			case_name,
			NULL,
			NULL,
			CK_NORMAL
	);
	int number_failed = srunner_ntests_failed(runner);
	// exit:
	srunner_free(runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void print_cmd_line_info(
		int argc,
		char* argv[]
)
{
	printf( "usage: %s [TEST_SUITE] [TEST_CASE]\n", argv[0] );
	printf( "\n" );
	printf( "ARGS: can be 'all' or the name of a test suite / case\n" );
}

int parse_cmd_line_args(
		int argc,
		char* argv[],
		char** suite_name,
		char** case_name
)
{
	// parse options:
	while( true ) {
		int option_index = 0;
		int c = getopt_long(
				argc, argv,
				short_options,
				long_options,
				&option_index
		);
		if( c == -1 ) { break; }
		switch( c ) {
			case 'h':
				return -1;
			break;
		}
	}
	// parse arguments:
	int index = 1;
	static char suite_name_buf[STR_BUFFER_SIZE] = "";
	static char case_name_buf[STR_BUFFER_SIZE] = "";
	(*suite_name) = NULL;
	(*case_name) = NULL;
	if( index < argc ) {
		if( strcmp(argv[index], "all" ) ) {
			strncpy( suite_name_buf, argv[index], STR_BUFFER_SIZE );
			(*suite_name) = suite_name_buf;
		}
		++index;
	}
	if( index < argc ) {
		if( strcmp(argv[index], "all" ) ) {
			strncpy( case_name_buf, argv[index], STR_BUFFER_SIZE );
			(*case_name) = case_name_buf;
		}
		++index;
	}
	return 0;
}
