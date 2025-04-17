#include "lib/camera.h"
#include "lib/global.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h> // <- work with files


const char short_options[] = "h";
const  struct option long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ 0,0,0,0 },
};


void print_cmd_line_info(
		// int argc,
		char* argv[]
);
int parse_cmd_line_args(
		int argc,
		char* argv[]
);

int main(
		int argc,
		char* argv[]
) {
	{
		int ret = parse_cmd_line_args(
				argc, argv
		);
		// --help:
		if( ret == -1 ) {
			print_cmd_line_info( /*argc,*/ argv );
			return EXIT_SUCCESS;
		}
		// error parsing cmd line args:
		else if( ret != 0 ) {
			print_cmd_line_info( /*argc,*/ argv );
			return EXIT_FAILURE;
		}
	}
	// print output of '$ uname -a'
	{
		char cmd[] = "uname -a";
		char str[STR_BUFFER_SIZE];
		FILE* file = popen( cmd, "r" );
		if( !file ) {
			perror( "popen" );
			return EXIT_FAILURE;
		}
		fgets(str, STR_BUFFER_SIZE, file);
		printf("$ %s\n", cmd );
		printf("%s", str);
		if( -1 == pclose( file ) ) {
			perror( "pclose" );
		}
	}
	camera_t camera;
	const char dev_name[] = "/dev/video0";
	camera_init(
			&camera,
			dev_name
	);
	{
		format_descriptions_t format_descriptions;
		if( RET_SUCCESS != camera_list_formats(
					&camera,
					&format_descriptions
		) ) {
			fprintf(stderr, "%s\n", camera_error() );
			return EXIT_FAILURE;
		}
		printf( "camera video formats:\n" );
		for( unsigned int i=0; i< format_descriptions.count; i++ ) {
			printf( "- %s\n", format_descriptions.format_descrs[i].description );
		}
	}
	camera_exit(&camera);

	return EXIT_SUCCESS;
}

void print_cmd_line_info(
		// int argc,
		char* argv[]
)
{
	printf( "usage: %s\n", argv[0] );
}

int parse_cmd_line_args(
		int argc,
		char* argv[]
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
	return 0;
}
